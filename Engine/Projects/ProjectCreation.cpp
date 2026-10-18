#include "Engine/Projects/ProjectCreation.h"

#include "Engine/Core/Json.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Framework/Scene.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Astral::Projects {
namespace {
namespace fs = std::filesystem;
using Core::JsonValue;

bool Fail(std::string& error, std::string message) { error = std::move(message); return false; }

bool ControlOrNul(const std::string& text) {
    return std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 32 || c == 127; });
}

bool ValidDirectoryName(const std::string& name) {
    if (name.empty() || name == "." || name == ".." || ControlOrNul(name)
        || name.find_first_of("/\\:<>\"|?*") != std::string::npos || name.back() == '.' || name.back() == ' ') return false;
    std::string base = name.substr(0, name.find('.'));
    while (!base.empty() && base.back() == ' ') base.pop_back();
    for (char& c : base) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL") return false;
    return !(base.size() == 4 && (base.compare(0, 3, "COM") == 0 || base.compare(0, 3, "LPT") == 0)
        && base[3] >= '1' && base[3] <= '9');
}

bool Absent(const fs::path& path, std::string& error) {
    std::error_code ec;
    const auto status = fs::symlink_status(path, ec);
    if (ec && ec != std::errc::no_such_file_or_directory) return Fail(error, "cannot inspect destination: " + ec.message());
    if (status.type() != fs::file_type::not_found) return Fail(error, "destination already exists: " + path.string());
    return true;
}

bool Redirected(const fs::path& path, std::string& error) {
#if defined(_WIN32)
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        error = "cannot inspect project entry: " + std::error_code(GetLastError(), std::system_category()).message();
        return true;
    }
    if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) { error = "project entry is a reparse point"; return true; }
#else
    std::error_code ec;
    if (fs::is_symlink(fs::symlink_status(path, ec))) { error = "project entry is a symbolic link"; return true; }
    if (ec) { error = "cannot inspect project entry: " + ec.message(); return true; }
#endif
    return false;
}

bool Within(const fs::path& root, const fs::path& path) {
    auto part = path.begin();
    fs::path prefix;
    for (auto r = root.begin(); r != root.end() && part != path.end(); ++r) prefix /= *part++;
    std::error_code ec;
    return fs::equivalent(root, prefix, ec) && !ec;
}

struct Writer {
    fs::path parent, root;
    std::vector<fs::path> directories, files;

    bool CheckDirectories(const fs::path& last, std::string& error) const {
        // All relative parts are fixed internal names; no caller-supplied path
        // is interpreted below the exclusively reserved root.
        const fs::path relative = last.lexically_relative(root);
        if (relative.empty() || relative.is_absolute()) return Fail(error, "invalid owned project directory");
        fs::path cursor = root;
        const auto check = [&]() {
            if (Redirected(cursor, error)) return false;
            std::error_code ec;
            const fs::path actual = fs::canonical(cursor, ec);
            if (ec || !fs::is_directory(actual, ec) || ec || !Within(parent, actual) || !Within(root, actual))
                return Fail(error, "project directory escaped its approved root");
            return true;
        };
        if (!check()) return false;
        for (const auto& part : relative) {
            if (part == ".") continue;
            if (part == "..") return Fail(error, "invalid owned project parent segment");
            cursor /= part;
            if (!check()) return false;
        }
        return true;
    }

    bool Reserve(std::string& error) {
        directories.reserve(3);
        files.reserve(6);
        if (!Absent(root, error)) return false;
        std::error_code ec;
        if (!fs::create_directory(root, ec)) return Fail(error, "cannot reserve new project directory: " + ec.message());
        directories.push_back(root);
        return CheckDirectories(root, error);
    }

    bool Directory(const fs::path& path, std::string& error) {
        if (!CheckDirectories(path.parent_path(), error) || !Absent(path, error)) return false;
        std::error_code ec;
        if (!fs::create_directory(path, ec)) return Fail(error, "cannot create project directory: " + ec.message());
        directories.push_back(path);
        return CheckDirectories(path, error);
    }

    bool Write(const fs::path& path, const JsonValue* json, const std::string& text, std::string& error) {
        const fs::path temporary(path.string() + ".tmp");
        if (!CheckDirectories(path.parent_path(), error) || !Absent(path, error) || !Absent(temporary, error)) return false;
        files.push_back(temporary); // known absent; our write may leave a partial temporary
        if (json) {
            if (!Core::WriteJsonFile(path.string(), *json, error)) return false;
            files.push_back(path);
            JsonValue written;
            if (!Core::ReadJsonFile(path.string(), written, error)) return false;
            if (written != *json) return Fail(error, "written project JSON differs from the validated document");
        } else {
            {
                std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
                if (!file) return Fail(error, "cannot write project README");
                file.write(text.data(), static_cast<std::streamsize>(text.size()));
                if (!file) return Fail(error, "short write to project README");
                file.close();
                if (!file) return Fail(error, "cannot close project README");
            }
            std::error_code ec;
            fs::rename(temporary, path, ec);
            if (ec) return Fail(error, "cannot publish project README: " + ec.message());
            files.push_back(path);
        }
        return true;
    }

    void Rollback(std::string& error) const {
        const auto retain = [&](const fs::path& path) { error += "; rollback retained " + path.string(); };
        for (auto it = files.rbegin(); it != files.rend(); ++it) {
            std::error_code ec;
            const auto status = fs::symlink_status(*it, ec);
            if (status.type() == fs::file_type::not_found && (!ec || ec == std::errc::no_such_file_or_directory)) continue;
            std::string ignored;
            if (ec || !fs::is_regular_file(status) || !CheckDirectories(it->parent_path(), ignored)
                || Redirected(*it, ignored)) { retain(*it); continue; }
            const auto links = fs::hard_link_count(*it, ec);
            if (ec || links != 1 || !fs::remove(*it, ec) || ec) retain(*it);
        }
        for (auto it = directories.rbegin(); it != directories.rend(); ++it) {
            std::error_code ec;
            const auto status = fs::symlink_status(*it, ec);
            if (status.type() == fs::file_type::not_found && (!ec || ec == std::errc::no_such_file_or_directory)) continue;
            std::string ignored;
            if (ec || !CheckDirectories(*it, ignored) || !fs::remove(*it, ec) || ec) retain(*it);
        }
    }
};

bool Documents(const ProjectCreationRequest& request, JsonValue& project, JsonValue& scene, std::string& error) {
    if (request.templateKind != ProjectTemplate::Blank3D && request.templateKind != ProjectTemplate::Basic3D)
        return Fail(error, "unknown project template");
    const char* blank = R"({"format":"astral-scene","version":1,"environment":{},"entities":[
        {"name":"Main Camera","transform":{"position":[0,3,-8],"rotation":{"euler":[14,0,0]}},
         "components":{"Camera":{"fieldOfView":60,"nearPlane":0.1,"farPlane":100},"AudioListener":{}}}]})";
    const char* basic = R"({"format":"astral-scene","version":1,"environment":{},
        "materials":{"neutral":{"shading":"Toon","baseColor":[0.55,0.58,0.62]}},"entities":[
        {"name":"Main Camera","transform":{"position":[0,3,-8],"rotation":{"euler":[14,0,0]}},
         "components":{"Camera":{"fieldOfView":60,"nearPlane":0.1,"farPlane":100},"AudioListener":{}}},
        {"name":"Sun","transform":{"rotation":{"euler":[50,30,0]}},
         "components":{"Light":{"type":"Directional","intensity":2.5}}},
        {"name":"Ground","transform":{"scale":[10,1,10]},
         "components":{"MeshRenderer":{"mesh":"primitive:plane","material":"neutral"},"Collider":{"shape":"Mesh"}}},
        {"name":"Box","transform":{"position":[0,2,0]},"components":{
         "MeshRenderer":{"mesh":"primitive:cube","material":"neutral"},
         "Collider":{"shape":"Box","halfExtents":[0.5,0.5,0.5]},"RigidBody":{"type":"Dynamic","mass":1}}}]})";
    if (!Core::ParseJson(request.templateKind == ProjectTemplate::Blank3D ? blank : basic, scene, error)
        || !Core::ParseJson(R"({"format":"astral-project","version":1,"name":"", "startupScene":"scenes/main.scene.json",
            "input":{"contexts":[]},"config":{"r.Quality":2}})", project, error)) return false;
    project.Set("name", request.displayName.empty() ? request.directoryName : request.displayName);
    Framework::ProjectDesc parsedProject;
    Framework::SceneDocument parsedScene;
    return Framework::ParseProject(project, parsedProject, error)
        && Framework::SceneSerializer().Parse(scene, parsedScene, error);
}
} // namespace

bool CreateProject(const ProjectCreationRequest& request, CreatedProject& out, std::string& error) {
    Writer writer;
    try {
        if (request.approvedParent.empty() || request.approvedParent.find('\0') != std::string::npos)
            return Fail(error, "an explicit existing approved parent directory is required");
        if (!ValidDirectoryName(request.directoryName)) return Fail(error, "project directory must be one valid new name");
        if (ControlOrNul(request.displayName)) return Fail(error, "project display name cannot contain control characters");
        JsonValue project, scene;
        if (!Documents(request, project, scene, error)) return false;
        std::error_code ec;
        writer.parent = fs::canonical(fs::u8path(request.approvedParent), ec);
        if (ec || !fs::is_directory(writer.parent, ec) || ec) return Fail(error, "cannot resolve approved parent directory");
        const auto name = fs::u8path(request.directoryName);
        if (name.has_root_path() || name.filename() != name) return Fail(error, "project directory must be one relative name");
        writer.root = writer.parent / name;
        const fs::path content = writer.root / "Content";
        const fs::path sceneDirectory = content / "scenes";
        CreatedProject result{writer.root.string(), content.string(), (content / "project.json").string()};
        if (fs::path(result.root) != writer.root || fs::path(result.contentRoot) != content
            || fs::path(result.projectFile) != content / "project.json")
            return Fail(error, "project path cannot round-trip through the current runtime path encoding");
        const std::string readme = "# " + project.String("name") + "\n\n"
            "Original neutral Astral engine verification project; no game sample registration is required.\n"
            "Use the built AstralEnginePlayer executable with this Content directory and project.json:\n\n"
            "AstralEnginePlayer \"" + result.contentRoot + "\" project.json --frames 120 --size 320x180\n\n"
            "Blank3D is a perspective camera; Basic3D adds original primitives, lighting and physics.\n"
            "There are no scripted controls, 2D template, native project wizard or packaged runtime here.\n"
            "Preserve the Astral source ownership and applicable runtime notices; no new public license is granted.\n";
        if (!writer.Reserve(error) || !writer.Directory(content, error) || !writer.Directory(sceneDirectory, error)
            || !writer.Write(sceneDirectory / "main.scene.json", &scene, {}, error)
            || !writer.Write(writer.root / "README.md", nullptr, readme, error)
            || !writer.Write(content / "project.json", &project, {}, error)) {
            writer.Rollback(error);
            return false;
        }
        out = std::move(result);
        error.clear();
        return true;
    } catch (const std::system_error& e) {
        error = "cannot convert or create project path: " + e.code().message();
        writer.Rollback(error);
        return false;
    }
}

} // namespace Astral::Projects
