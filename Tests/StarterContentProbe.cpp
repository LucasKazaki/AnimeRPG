#include "Engine/Assets/StaticMesh.h"
#include <iostream>

// Uses the actual engine loader. Explicit conditions survive NDEBUG/Release.
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: StarterContentProbe mesh-file [mesh-file ...]\n";
        return 2;
    }
    Astral::Assets::StaticMesh mesh;
    for (int i = 1; i < argc; ++i) {
        if (!mesh.LoadFromFile(argv[i])) {
            std::cerr << "REJECTED " << argv[i] << '\n';
            return 1;
        }
        std::cout << mesh.Vertices().size() << ' ' << mesh.Edges().size() << '\n';
    }
    return 0;
}
