#include "Engine/Projects/ProjectCreation.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Graphics/RenderTarget.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Tests/EngineTestSupport.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#endif

using namespace Astral;
namespace {
namespace fs = std::filesystem;

struct ProjectFiles {
    fs::path directory, parent, outside;
    ProjectFiles() {
        static unsigned sequence = 0;
        directory = fs::temp_directory_path() / ("astral-project-creation-"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
            + "-" + std::to_string(++sequence));
        parent = directory / "projects";
        outside = directory / "outside";
        fs::create_directories(parent);
        fs::create_directory(outside);
    }
    ~ProjectFiles() { fs::remove_all(directory); } // unique fixture tree, never a user project
    Projects::ProjectCreationRequest Request(const std::string& name = "NeutralProject") const {
        return {parent.u8string(), name, "Neutral Box", Projects::ProjectTemplate::Blank3D};
    }
};

std::string Text(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void Unchanged(const Projects::CreatedProject& result) {
    ASTRAL_CHECK(result.root == "old-root" && result.contentRoot == "old-content" && result.projectFile == "old-project");
}

Framework::HostSettings HostSettings(const Projects::CreatedProject& project) {
    Framework::HostSettings settings;
    settings.contentRoot = project.contentRoot;
    settings.workerThreads = 0;
    settings.audio = false;
    settings.hotReload = false;
    return settings;
}

bool DirectoryLink(const fs::path& target, const fs::path& link) {
#if defined(_WIN32)
    struct JunctionData {
        DWORD tag; WORD length, reserved, substituteOffset, substituteLength, printOffset, printLength;
        wchar_t paths[4096];
    } data{};
    const std::wstring absolute = fs::absolute(target).make_preferred().wstring();
    const std::wstring substitute = L"\\??\\" + absolute;
    if (substitute.size() + absolute.size() + 2 > 4096) return false;
    data.tag = IO_REPARSE_TAG_MOUNT_POINT;
    data.substituteLength = static_cast<WORD>(substitute.size() * sizeof(wchar_t));
    data.printOffset = static_cast<WORD>((substitute.size() + 1) * sizeof(wchar_t));
    data.printLength = static_cast<WORD>(absolute.size() * sizeof(wchar_t));
    std::memcpy(data.paths, substitute.c_str(), data.substituteLength + sizeof(wchar_t));
    std::memcpy(data.paths + substitute.size() + 1, absolute.c_str(), data.printLength + sizeof(wchar_t));
    data.length = static_cast<WORD>(8 + data.printOffset + data.printLength + sizeof(wchar_t));
    if (!CreateDirectoryW(link.c_str(), nullptr)) return false;
    HANDLE handle = CreateFileW(link.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    DWORD returned = 0;
    const bool ok = DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, &data, data.length + 8,
        nullptr, 0, &returned, nullptr) != FALSE;
    CloseHandle(handle);
    return ok;
#else
    std::error_code ec;
    fs::create_directory_symlink(target, link, ec);
    return !ec;
#endif
}
} // namespace

ASTRAL_TEST(BlankProjectLoadsWithARealCameraAndNoGeometry) {
    ProjectFiles files;
    const auto request = files.Request();
    Projects::CreatedProject created;
    std::string error;
    ASTRAL_CHECK(Projects::CreateProject(request, created, error));
    ASTRAL_CHECK(error.empty() && fs::path(created.root).is_absolute());
    ASTRAL_CHECK(fs::equivalent(fs::path(created.root).parent_path(), files.parent));
    Core::JsonValue project, scene;
    ASTRAL_CHECK(Core::ReadJsonFile(created.projectFile, project, error));
    ASTRAL_CHECK(project.String("name") == "Neutral Box" && project.String("startupScene") == "scenes/main.scene.json");
    ASTRAL_CHECK(Core::ReadJsonFile(created.contentRoot + "/scenes/main.scene.json", scene, error));
    ASTRAL_CHECK(scene["entities"].Size() == 1);
    Framework::GameHost host(HostSettings(created));
    ASTRAL_CHECK(host.LoadProject("project.json", error));
    ASTRAL_CHECK(!host.World().ActiveCamera().IsNull());
    ASTRAL_CHECK(host.World().Registry().Count<Framework::MeshRenderer>() == 0);
    host.Update(1.0f / 60.0f, {});
    const auto& image = host.Render(96, 54);
    ASTRAL_CHECK(image.width == 96 && image.height == 54 && !image.pixels.empty());
    ASTRAL_CHECK(host.Stats().render.drawsVisible == 0); // blank does not pretend to draw a mesh
    ASTRAL_CHECK(host.World().Physics().BodyCount() == 0);
    std::size_t fileCount = 0;
    for (const auto& entry : fs::recursive_directory_iterator(created.root)) {
        ASTRAL_CHECK(entry.path().extension() != ".tmp" && !entry.is_symlink());
        if (entry.is_regular_file()) ++fileCount;
    }
    ASTRAL_CHECK(fileCount == 3);
    ASTRAL_CHECK(Text(fs::path(created.root) / "README.md").find("Neutral Box") != std::string::npos);
}

ASTRAL_TEST(Basic3DProjectSimulatesAFallingRestingBoxAndRendersItsId) {
    ProjectFiles files;
    auto request = files.Request();
    request.templateKind = Projects::ProjectTemplate::Basic3D;
    Projects::CreatedProject created;
    std::string error;
    ASTRAL_CHECK(Projects::CreateProject(request, created, error));
    Framework::GameHost host(HostSettings(created));
    ASTRAL_CHECK(host.LoadProject("project.json", error));
    const auto box = host.World().Find("Box");
    const auto ground = host.World().Find("Ground");
    ASTRAL_CHECK(!box.IsNull() && !ground.IsNull());
    ASTRAL_CHECK_NEAR(host.World().GetWorld(box).translation.y, 2, 1e-5f);
    Graphics::RenderScene scene;
    Graphics::RenderView view;
    ASTRAL_CHECK(host.World().BuildRenderScene(scene, view, 160, 90));
    Graphics::SceneRenderer renderer;
    Graphics::RenderTarget target;
    target.Resize(160, 90);
    renderer.Render(scene, view, target);
    std::size_t boxPixels = 0, groundPixels = 0;
    for (const auto id : target.objectId) { boxPixels += id == box.index + 1; groundPixels += id == ground.index + 1; }
    ASTRAL_CHECK(boxPixels > 0 && groundPixels > 0);
    for (int frame = 0; frame < 15; ++frame) host.Update(1.0f / 60.0f, {});
    const float falling = host.World().GetWorld(box).translation.y;
    ASTRAL_CHECK(falling < 1.9f && falling > 0.5f);
    for (int frame = 0; frame < 225; ++frame) host.Update(1.0f / 60.0f, {});
    const auto* collider = host.World().Get<Framework::Collider>(box);
    const auto* body = host.World().Physics().GetBody(collider->body);
    ASTRAL_CHECK(body && body->type == Physics::BodyType::Dynamic && host.World().Physics().BodyCount() == 2);
    const float resting = host.World().GetWorld(box).translation.y;
    ASTRAL_CHECK(resting > 0.35f && resting < 0.65f && std::fabs(body->linearVelocity.y) < 0.3f);
    ASTRAL_CHECK_NEAR(resting, body->pose.position.y, 1e-5f);
    const auto& image = host.Render(160, 90);
    ASTRAL_CHECK(image.width == 160 && image.height == 90 && host.Stats().render.drawsVisible > 0);
}

ASTRAL_TEST(InvalidProjectRequestsPreserveOutputAndDoNotCreateEntries) {
    ProjectFiles files;
    const std::string invalid[] = {"", ".", "..", "../escape", "a/b", "a\\b", "C:project", "bad:",
        "bad?name", "trailing.", "trailing ", "CON", "nul.json", "LPT1.project", std::string("nul") + '\0' + "tail"};
    for (const auto& name : invalid) {
        auto request = files.Request(name);
        Projects::CreatedProject result{"old-root", "old-content", "old-project"};
        std::string error;
        ASTRAL_CHECK(!Projects::CreateProject(request, result, error) && !error.empty());
        Unchanged(result);
        ASTRAL_CHECK(fs::is_empty(files.parent));
    }
    for (const int mode : {0, 1, 2, 3}) {
        auto request = files.Request();
        if (mode == 0) request.approvedParent.clear();
        if (mode == 1) request.approvedParent = (files.directory / "missing").u8string();
        if (mode == 2) request.displayName = "invalid\nname";
        if (mode == 3) request.templateKind = static_cast<Projects::ProjectTemplate>(99);
        Projects::CreatedProject result{"old-root", "old-content", "old-project"};
        std::string error;
        ASTRAL_CHECK(!Projects::CreateProject(request, result, error) && !error.empty());
        Unchanged(result);
        ASTRAL_CHECK(fs::is_empty(files.parent));
    }
}

ASTRAL_TEST(ExistingProjectDestinationsAreNeverOverwrittenOrCleaned) {
    ProjectFiles files;
    std::ofstream(files.parent / "file") << "keep file bytes";
    fs::create_directory(files.parent / "empty");
    fs::create_directory(files.parent / "nonempty");
    std::ofstream(files.parent / "nonempty/marker") << "keep directory bytes";
    for (const char* name : {"file", "empty", "nonempty"}) {
        Projects::CreatedProject result{"old-root", "old-content", "old-project"};
        std::string error;
        ASTRAL_CHECK(!Projects::CreateProject(files.Request(name), result, error) && !error.empty());
        Unchanged(result);
    }
    ASTRAL_CHECK(Text(files.parent / "file") == "keep file bytes");
    ASTRAL_CHECK(fs::is_empty(files.parent / "empty"));
    ASTRAL_CHECK(Text(files.parent / "nonempty/marker") == "keep directory bytes");
}

ASTRAL_TEST(ProjectDestinationLinksAndBrokenLinksAreRefused) {
    ProjectFiles files;
    std::ofstream(files.outside / "marker") << "keep outside fixture";
    const auto link = files.parent / "linked";
    if (DirectoryLink(files.outside, link)) {
        Projects::CreatedProject result{"old-root", "old-content", "old-project"};
        std::string error;
        ASTRAL_CHECK(!Projects::CreateProject(files.Request("linked"), result, error));
        Unchanged(result);
        ASTRAL_CHECK(Text(files.outside / "marker") == "keep outside fixture");
        ASTRAL_CHECK(!fs::exists(files.outside / "Content"));
        fs::remove(link);
    } else std::printf("[ SKIP ] destination junction/directory symlink unavailable\n");
    std::error_code ec;
    fs::create_symlink(files.outside / "missing", link, ec);
    if (ec) { std::printf("[ SKIP ] broken file symlink unavailable: %s\n", ec.message().c_str()); return; }
    Projects::CreatedProject result{"old-root", "old-content", "old-project"};
    std::string error;
    ASTRAL_CHECK(!Projects::CreateProject(files.Request("linked"), result, error));
    Unchanged(result);
    ASTRAL_CHECK(fs::is_symlink(fs::symlink_status(link)) && !fs::exists(files.outside / "missing"));
    fs::remove(link);
}

ASTRAL_TEST(AnExplicitSelectedParentAliasUsesItsResolvedApprovedRoot) {
    ProjectFiles files;
    const auto alias = files.directory / "selected-parent";
    if (!DirectoryLink(files.outside, alias)) { std::printf("[ SKIP ] approved-parent alias unavailable\n"); return; }
    auto request = files.Request("CanonicalChild");
    request.approvedParent = alias.u8string();
    request.displayName.clear();
    Projects::CreatedProject result;
    std::string error;
    ASTRAL_CHECK(Projects::CreateProject(request, result, error));
    ASTRAL_CHECK(fs::equivalent(fs::path(result.root).parent_path(), files.outside));
    Core::JsonValue project;
    ASTRAL_CHECK(Core::ReadJsonFile(result.projectFile, project, error));
    ASTRAL_CHECK(project.String("name") == "CanonicalChild");
    fs::remove(alias);
}

ASTRAL_TEST(ProjectCreationHasNoGameBehaviourRegistrationDependency) {
    ASTRAL_CHECK(!Framework::BehaviourRegistry::Global().Has("PlayerController"));
    ASTRAL_CHECK(!Framework::BehaviourRegistry::Global().Has("Spinner"));
    ProjectFiles files;
    for (const auto kind : {Projects::ProjectTemplate::Blank3D, Projects::ProjectTemplate::Basic3D}) {
        auto request = files.Request(kind == Projects::ProjectTemplate::Blank3D ? "Blank" : "Basic");
        request.templateKind = kind;
        Projects::CreatedProject result;
        std::string error;
        ASTRAL_CHECK(Projects::CreateProject(request, result, error));
        Framework::GameHost host(HostSettings(result));
        ASTRAL_CHECK(host.LoadProject("project.json", error));
        ASTRAL_CHECK(host.World().Registry().Count<Framework::Behaviours>() == 0);
    }
    ASTRAL_CHECK(!Framework::BehaviourRegistry::Global().Has("PlayerController"));
}

ASTRAL_TEST_MAIN("EngineProjectTests")
