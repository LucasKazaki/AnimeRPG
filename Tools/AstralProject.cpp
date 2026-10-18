#include "Engine/Projects/ProjectCreation.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace {
int Usage() {
    std::fprintf(stderr, "usage: AstralProject create <existing-parent> <new-directory> --template blank3d|3d [--name <display>]\n");
    return 1;
}

int Run(const std::vector<std::string>& arguments) {
    if (arguments.size() < 6 || arguments[1] != "create") return Usage();
    Astral::Projects::ProjectCreationRequest request;
    request.approvedParent = arguments[2];
    request.directoryName = arguments[3];
    bool haveTemplate = false, haveName = false;
    for (std::size_t i = 4; i < arguments.size(); i += 2) {
        if (i + 1 == arguments.size()) return Usage();
        if (arguments[i] == "--template" && !haveTemplate) {
            haveTemplate = true;
            if (arguments[i + 1] == "blank3d") request.templateKind = Astral::Projects::ProjectTemplate::Blank3D;
            else if (arguments[i + 1] == "3d") request.templateKind = Astral::Projects::ProjectTemplate::Basic3D;
            else return Usage();
        } else if (arguments[i] == "--name" && !haveName) {
            haveName = true;
            request.displayName = arguments[i + 1];
        } else return Usage();
    }
    if (!haveTemplate) return Usage();
    Astral::Projects::CreatedProject created;
    std::string error;
    if (!Astral::Projects::CreateProject(request, created, error)) {
        std::fprintf(stderr, "project creation failed: %s\n", error.c_str());
        return 2;
    }
    std::printf("created %s\ncontent root: %s\nproject: %s\n", created.root.c_str(), created.contentRoot.c_str(), created.projectFile.c_str());
    return 0;
}
} // namespace

#if defined(_WIN32)
int wmain(int argc, wchar_t** argv) {
    try {
        std::vector<std::string> arguments;
        for (int i = 0; i < argc; ++i) arguments.push_back(std::filesystem::path(argv[i]).u8string());
        return Run(arguments);
    } catch (const std::system_error& e) {
        std::fprintf(stderr, "cannot convert project arguments: %s\n", e.code().message().c_str());
        return 1;
    }
}
#else
int main(int argc, char** argv) {
    return Run(std::vector<std::string>(argv, argv + argc));
}
#endif
