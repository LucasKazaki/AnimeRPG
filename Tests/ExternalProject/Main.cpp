// Copy this source/CMake fixture outside the repository before configuring it.
// It consumes only public engine APIs and its own native behavior registration.
#include "Engine/Framework/GameHost.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
struct Counts { int created{}, updated{}, destroyed{}; } counts;
struct ExternalProbe final : Astral::Framework::Behaviour {
    void OnCreate() override { ++counts.created; }
    void OnUpdate(float) override { ++counts.updated; }
    void OnDestroy() override { ++counts.destroyed; }
};
void Require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message); // always active in Release
}
std::string ReadBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    Require(file.good(), "cannot read fixture bytes");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
bool Absent(const std::filesystem::path& path) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    return status.type() == std::filesystem::file_type::not_found
        && (!error || error == std::errc::no_such_file_or_directory);
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) { std::fprintf(stderr, "usage: ExternalProjectConsumer <generated-content-root>\n"); return 1; }
    std::filesystem::path ownedProbe;
    bool reservedProbe = false;
    try {
        using namespace Astral;
        namespace fs = std::filesystem;
        Require(!Framework::BehaviourRegistry::Global().Has("PlayerController"), "unexpected game registration");
        Framework::BehaviourRegistry::Global().Register<ExternalProbe>("ExternalProjectProbe");
        Framework::HostSettings settings;
        settings.contentRoot = argv[1];
        settings.workerThreads = 0;
        settings.audio = false;
        settings.hotReload = false;
        Framework::GameHost host(settings);
        std::string error;
        Require(host.LoadProject("project.json", error), error);
        Require(!host.World().ActiveCamera().IsNull(), "generated project has no active camera");
        const fs::path originalPath = fs::path(settings.contentRoot) / "scenes/main.scene.json";
        const std::string originalBytes = ReadBytes(originalPath);
        const fs::path sceneDirectory = fs::canonical(originalPath.parent_path());
        ownedProbe = sceneDirectory / "external-owned-probe.scene.json";
        const fs::path temporary(ownedProbe.string() + ".tmp");
        Require(Absent(ownedProbe) && Absent(temporary), "outside consumer probe or temporary already exists; preserving it");
        reservedProbe = true; // both fixed sibling paths were absent before this fixture's write
        Core::JsonValue original;
        Require(Core::ReadJsonFile(originalPath.string(), original, error), error);
        Core::JsonValue authored = original; // preserve the generated template on disk
        Core::JsonValue probe;
        Require(Core::ParseJson(R"({"name":"External authored probe","behaviours":[{"type":"ExternalProjectProbe"}]})", probe, error), error);
        authored.Find("entities")->Append(probe);
        Require(Core::WriteJsonFile(ownedProbe.string(), authored, error), error);
        Core::JsonValue onDisk;
        Require(Core::ReadJsonFile(ownedProbe.string(), onDisk, error) && onDisk == authored, "authored scene was not written intact");
        Require(host.LoadScene("scenes/external-owned-probe.scene.json", error), error);
        Require(counts.created == 1 && counts.updated == 0 && counts.destroyed == 0, "authored native OnCreate did not run once");
        const auto entity = host.World().Find("External authored probe");
        Require(!entity.IsNull() && host.World().Get<Framework::Behaviours>(entity), "scene did not attach native behavior");
        for (int frame = 0; frame < 3; ++frame) host.Update(1.0f / 60.0f, {});
        Require(counts.updated == 3, "authored native behavior did not update");
        const auto& image = host.Render(96, 54);
        Require(image.width == 96 && image.height == 54 && !image.pixels.empty(), "external project did not render");
        Require(host.LoadScene("scenes/main.scene.json", error), error);
        Require(counts.destroyed == 1, "authored native behavior did not destroy exactly once");
        Require(host.World().Find("External authored probe").IsNull(), "original scene was not restored");
        Core::JsonValue after;
        Require(Core::ReadJsonFile(originalPath.string(), after, error), error);
        Require(after == original && ReadBytes(originalPath) == originalBytes, "outside consumer modified the original template");
        // Remove only this fixture's checked ordinary sibling. Unknown or changed
        // entries, including temporary aliases, are retained with a diagnostic.
        std::error_code cleanupError;
        const auto status = fs::symlink_status(ownedProbe, cleanupError);
        Require(!cleanupError && fs::is_regular_file(status) && !fs::is_symlink(status)
            && fs::canonical(ownedProbe.parent_path()) == sceneDirectory,
            "outside consumer retained an unsafe probe entry");
        Require(fs::hard_link_count(ownedProbe, cleanupError) == 1 && !cleanupError,
            "outside consumer retained an aliased probe entry");
        Require(Core::ReadJsonFile(ownedProbe.string(), onDisk, error) && onDisk == authored && Absent(temporary),
            "outside consumer retained changed probe or unexpected temporary entry");
        Require(fs::remove(ownedProbe, cleanupError) && !cleanupError, "cannot remove owned outside consumer probe");
        reservedProbe = false;
        std::printf("external Runtime-only consumer: create=%d update=%d destroy=%d\n", counts.created, counts.updated, counts.destroyed);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "external consumer failed: %s\n", error.what());
        if (reservedProbe) std::fprintf(stderr, "retaining owned probe/temporary for diagnosis: %s[.tmp]\n", ownedProbe.string().c_str());
        return 2;
    }
}
