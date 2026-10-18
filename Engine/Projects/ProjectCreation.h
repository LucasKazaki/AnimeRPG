#pragma once

#include <string>

namespace Astral::Projects {

enum class ProjectTemplate { Blank3D, Basic3D };

struct ProjectCreationRequest {
    std::string approvedParent; // explicit existing directory; no cwd default
    std::string directoryName;  // one new directory component, never an existing destination
    std::string displayName;    // empty uses directoryName
    ProjectTemplate templateKind{ProjectTemplate::Blank3D};
};

struct CreatedProject {
    std::string root;
    std::string contentRoot;
    std::string projectFile;
};

// Creates original neutral content using the existing project/scene schemas.
// Failure leaves `out` and existing destinations unchanged. Reserves a new
// directory exclusively, writes project.json last, and rolls back only known
// owned files/empty directories. An unsafe/incomplete rollback is reported.
// Single-writer path contract: not atomic directory publication or a sandbox
// against concurrent filesystem replacement. No launch or UI side effects.
bool CreateProject(const ProjectCreationRequest& request, CreatedProject& out, std::string& error);

} // namespace Astral::Projects
