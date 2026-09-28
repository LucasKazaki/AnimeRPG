#include "Engine/World/Serialization.h"

#include "Engine/World/Components.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <set>
#include <sstream>

namespace Astral::World {

bool TokenizeLine(const std::string& line, std::vector<std::string>& tokens) {
    tokens.clear();
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
        if (i >= line.size()) break;
        std::string token;
        if (line[i] == '"') {
            ++i;
            bool closed = false;
            while (i < line.size()) {
                const char c = line[i++];
                if (c == '\\' && i < line.size()) {
                    token.push_back(line[i++]);
                } else if (c == '"') {
                    closed = true;
                    break;
                } else {
                    token.push_back(c);
                }
            }
            if (!closed) return false;
        } else {
            while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r') token.push_back(line[i++]);
        }
        tokens.push_back(token);
    }
    return true;
}

std::string QuoteString(const std::string& value) {
    std::string out = "\"";
    for (char c : value) {
        if (c == '"' || c == '\\') out.push_back('\\');
        if (c == '\n' || c == '\r') {
            out.push_back(' ');
            continue;
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

bool ParseFloat(const std::string& token, float& value) {
    if (token.empty() || token.size() > 64) return false;
    errno = 0;
    char* end = nullptr;
    const double parsed = std::strtod(token.c_str(), &end);
    if (errno != 0 || end != token.c_str() + token.size() || !std::isfinite(parsed)
        || std::fabs(parsed) > 3.0e38) {
        return false;
    }
    value = static_cast<float>(parsed);
    return true;
}

namespace {
bool ParseIndex(const std::string& token, std::uint64_t& value) {
    if (token.empty() || token.size() > 12) return false;
    for (char c : token)
        if (c < '0' || c > '9') return false;
    value = std::strtoull(token.c_str(), nullptr, 10);
    return true;
}

std::string FormatFloat(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
    return buffer;
}
} // namespace

WorldSerializer::WorldSerializer() {
    Register("Name",
        [](const Registry& r, Entity e, std::string& out) {
            const Name* name = r.Get<Name>(e);
            if (!name) return false;
            out = QuoteString(name->value);
            return true;
        },
        [](Registry& r, Entity e, const std::vector<std::string>& args, std::string& error) {
            if (args.size() != 1) {
                error = "Name expects one string";
                return false;
            }
            r.Add<Name>(e, Name{args[0]});
            return true;
        });
    Register("LocalTransform",
        [](const Registry& r, Entity e, std::string& out) {
            const LocalTransform* t = r.Get<LocalTransform>(e);
            if (!t) return false;
            const Math::TRS& trs = t->trs;
            const float values[10] = {trs.translation.x, trs.translation.y, trs.translation.z, trs.rotation.x,
                trs.rotation.y, trs.rotation.z, trs.rotation.w, trs.scale.x, trs.scale.y, trs.scale.z};
            out.clear();
            for (int i = 0; i < 10; ++i) {
                if (i) out.push_back(' ');
                out += FormatFloat(values[i]);
            }
            return true;
        },
        [](Registry& r, Entity e, const std::vector<std::string>& args, std::string& error) {
            if (args.size() != 10) {
                error = "LocalTransform expects 10 numbers";
                return false;
            }
            float v[10];
            for (int i = 0; i < 10; ++i) {
                if (!ParseFloat(args[static_cast<std::size_t>(i)], v[i])) {
                    error = "LocalTransform has a non-finite or malformed number";
                    return false;
                }
            }
            LocalTransform t;
            t.trs.translation = {v[0], v[1], v[2]};
            t.trs.rotation = Math::Normalize(Math::Quat{v[3], v[4], v[5], v[6]});
            t.trs.scale = {v[7], v[8], v[9]};
            r.Add<LocalTransform>(e, t);
            return true;
        });
}

void WorldSerializer::Register(const std::string& name, WriteFn write, ReadFn read) {
    components_[name] = {std::move(write), std::move(read)};
}

std::string WorldSerializer::Save(const Registry& registry) const {
    std::vector<Entity> entities;
    registry.ForEachEntity([&entities](Entity e) { entities.push_back(e); });
    std::map<Entity, std::size_t> ids;
    for (std::size_t i = 0; i < entities.size(); ++i) ids[entities[i]] = i;
    std::string out = "ASTRAL_WORLD 1\n";
    std::string body;
    for (std::size_t i = 0; i < entities.size(); ++i) {
        const Entity e = entities[i];
        out += "entity " + std::to_string(i) + "\n";
        const Entity parent = GetParent(registry, e);
        if (!parent.IsNull()) out += "Parent " + std::to_string(ids[parent]) + "\n";
        for (const auto& component : components_) {
            if (component.second.write(registry, e, body)) out += component.first + (body.empty() ? "" : " " + body) + "\n";
        }
        out += "end\n";
    }
    return out;
}

bool WorldSerializer::Load(const std::string& text, Registry& destination, std::string& error,
    const WorldLoadLimits& limits) const {
    if (text.size() > limits.maxBytes) {
        error = "world file exceeds size limit";
        return false;
    }
    struct PendingComponent {
        std::string name;
        std::vector<std::string> args;
        std::size_t line;
    };
    struct PendingEntity {
        std::uint64_t id;
        long long parent{-1};
        std::vector<PendingComponent> components;
    };
    std::vector<PendingEntity> entities;
    std::set<std::uint64_t> seen;
    std::istringstream stream(text);
    std::string line;
    std::vector<std::string> tokens;
    std::size_t lineNumber = 0;
    bool header = false;
    bool inEntity = false;
    auto fail = [&](const std::string& message) {
        error = "line " + std::to_string(lineNumber) + ": " + message;
        return false;
    };
    while (std::getline(stream, line)) {
        ++lineNumber;
        if (line.size() > limits.maxLineLength) return fail("line too long");
        if (!TokenizeLine(line, tokens)) return fail("unterminated string");
        if (tokens.empty() || tokens[0][0] == '#') continue;
        if (!header) {
            if (tokens.size() != 2 || tokens[0] != "ASTRAL_WORLD") return fail("missing ASTRAL_WORLD header");
            if (tokens[1] != "1") return fail("unsupported world version " + tokens[1]);
            header = true;
            continue;
        }
        if (tokens[0] == "entity") {
            if (inEntity) return fail("nested entity");
            std::uint64_t id = 0;
            if (tokens.size() != 2 || !ParseIndex(tokens[1], id)) return fail("bad entity id");
            if (!seen.insert(id).second) return fail("duplicate entity id " + tokens[1]);
            if (entities.size() >= limits.maxEntities) return fail("entity limit exceeded");
            entities.push_back({id, -1, {}});
            inEntity = true;
        } else if (tokens[0] == "end") {
            if (!inEntity || tokens.size() != 1) return fail("unexpected end");
            inEntity = false;
        } else {
            if (!inEntity) return fail("component outside an entity");
            if (tokens[0] == "Parent") {
                std::uint64_t parent = 0;
                if (tokens.size() != 2 || !ParseIndex(tokens[1], parent)) return fail("bad Parent reference");
                if (entities.back().parent >= 0) return fail("duplicate Parent");
                entities.back().parent = static_cast<long long>(parent);
            } else if (components_.count(tokens[0])) {
                entities.back().components.push_back({tokens[0], {tokens.begin() + 1, tokens.end()}, lineNumber});
            } else if (strictUnknownComponents) {
                return fail("unknown component " + tokens[0]);
            }
        }
    }
    if (!header) {
        error = "empty world document";
        return false;
    }
    if (inEntity) {
        error = "unterminated entity";
        return false;
    }
    std::map<std::uint64_t, std::size_t> index;
    for (std::size_t i = 0; i < entities.size(); ++i) index[entities[i].id] = i;
    for (const PendingEntity& e : entities) {
        if (e.parent >= 0 && !index.count(static_cast<std::uint64_t>(e.parent))) {
            error = "entity " + std::to_string(e.id) + " references missing parent " + std::to_string(e.parent);
            return false;
        }
    }
    // Reject cycles before touching any registry.
    for (std::size_t i = 0; i < entities.size(); ++i) {
        std::size_t steps = 0;
        long long current = entities[i].parent;
        while (current >= 0) {
            if (static_cast<std::uint64_t>(current) == entities[i].id || ++steps > entities.size()) {
                error = "parent cycle through entity " + std::to_string(entities[i].id);
                return false;
            }
            current = entities[index[static_cast<std::uint64_t>(current)]].parent;
        }
    }
    // Build into a scratch registry; swap only on success (transactional load).
    Registry built;
    std::vector<Entity> handles;
    handles.reserve(entities.size());
    for (std::size_t i = 0; i < entities.size(); ++i) handles.push_back(built.Create());
    for (std::size_t i = 0; i < entities.size(); ++i) {
        for (const PendingComponent& component : entities[i].components) {
            std::string componentError;
            if (!components_.at(component.name).read(built, handles[i], component.args, componentError)) {
                error = "line " + std::to_string(component.line) + ": " + componentError;
                return false;
            }
        }
    }
    for (std::size_t i = 0; i < entities.size(); ++i) {
        if (entities[i].parent < 0) continue;
        if (!SetParent(built, handles[i], handles[index[static_cast<std::uint64_t>(entities[i].parent)]])) {
            error = "invalid parent assignment for entity " + std::to_string(entities[i].id);
            return false;
        }
    }
    destination = std::move(built);
    return true;
}

} // namespace Astral::World
