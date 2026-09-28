#pragma once

// Versioned, human-diffable world serialisation ("ASTRAL_WORLD 1").
//
//   ASTRAL_WORLD 1
//   entity 0
//   Name "Lincoln Memorial"
//   LocalTransform -8 0 18 0 0 0 1 1 1 1
//   end
//   entity 1
//   Parent 0
//   ...
//
// Loading is transactional: the destination registry is only modified when the
// whole document parses and validates (ids, references, cycles, finite numbers,
// resource limits). Games register their own component serialisers by name.

#include "Engine/World/Registry.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Astral::World {

struct WorldLoadLimits {
    std::size_t maxBytes{64u * 1024u * 1024u};
    std::size_t maxEntities{1000000u};
    std::size_t maxLineLength{4096u};
};

class WorldSerializer {
public:
    // Writes the component body after its name (no newline). Return false to skip.
    using WriteFn = std::function<bool(const Registry&, Entity, std::string&)>;
    // Parses the tokens after the name onto the entity. Return false on error.
    using ReadFn = std::function<bool(Registry&, Entity, const std::vector<std::string>&, std::string&)>;

    WorldSerializer(); // registers Name, LocalTransform (Parent is built in)
    void Register(const std::string& name, WriteFn write, ReadFn read);

    std::string Save(const Registry& registry) const;
    // On failure `registry` is untouched and `error` explains the first problem.
    bool Load(const std::string& text, Registry& registry, std::string& error,
        const WorldLoadLimits& limits = {}) const;

    bool strictUnknownComponents{true};

private:
    struct Entry {
        WriteFn write;
        ReadFn read;
    };
    std::map<std::string, Entry> components_;
};

// Tokeniser shared with component readers: whitespace separated, double-quoted
// strings with \" and \\ escapes. Returns false for an unterminated quote.
bool TokenizeLine(const std::string& line, std::vector<std::string>& tokens);
std::string QuoteString(const std::string& value);
bool ParseFloat(const std::string& token, float& value); // finite only

} // namespace Astral::World
