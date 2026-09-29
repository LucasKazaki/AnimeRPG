#include "Engine/Core/Reflection.h"

namespace Astral::Core {

const FieldInfo* TypeInfo::Field(std::string_view fieldName) const {
    for (const FieldInfo& field : fields) {
        if (field.name == fieldName) return &field;
    }
    return nullptr;
}

JsonValue TypeInfo::ToJson(const void* object) const {
    JsonValue json = JsonValue::MakeObject();
    for (const FieldInfo& field : fields) json.Set(field.name, field.read(object));
    return json;
}

bool TypeInfo::FromJson(void* object, const JsonValue& json, std::string& error, bool strict) const {
    if (!json.IsObject()) {
        error = name + " expects an object";
        return false;
    }
    // Validate on a scratch copy first so a bad field never half-applies.
    void* scratch = create ? create() : nullptr;
    const auto discard = [&] {
        if (scratch && destroy) destroy(scratch);
    };
    std::string unknown;
    for (std::size_t i = 0; i < json.Keys().size(); ++i) {
        const FieldInfo* field = Field(json.Keys()[i]);
        if (!field) {
            unknown += (unknown.empty() ? "" : ", ") + json.Keys()[i];
            continue;
        }
        if (scratch && !field->write(scratch, json.Values()[i], *field, error)) {
            error = name + ": " + error;
            discard();
            return false;
        }
    }
    discard();
    if (!unknown.empty()) {
        error = name + ": unknown field(s) " + unknown;
        if (strict) return false;
    } else {
        error.clear();
    }
    std::string ignored;
    for (std::size_t i = 0; i < json.Keys().size(); ++i) {
        if (const FieldInfo* field = Field(json.Keys()[i])) field->write(object, json.Values()[i], *field, ignored);
    }
    return true;
}

bool TypeInfo::SetField(void* object, std::string_view fieldName, const JsonValue& value, std::string& error) const {
    const FieldInfo* field = Field(fieldName);
    if (!field) {
        error = name + " has no field '" + std::string(fieldName) + "'";
        return false;
    }
    return field->write(object, value, *field, error);
}

JsonValue TypeInfo::GetField(const void* object, std::string_view fieldName) const {
    const FieldInfo* field = Field(fieldName);
    return field ? field->read(object) : JsonValue();
}

TypeRegistry& TypeRegistry::Global() {
    static TypeRegistry registry;
    return registry;
}

const TypeInfo* TypeRegistry::Find(std::string_view name) const {
    const auto it = byName_.find(name);
    return it == byName_.end() ? nullptr : it->second.get();
}

std::vector<std::string> TypeRegistry::Names() const {
    std::vector<std::string> names;
    for (const auto& entry : byName_) names.push_back(entry.first);
    return names;
}

} // namespace Astral::Core
