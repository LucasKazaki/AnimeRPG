#include "Engine/Scene/ThoughtCommands.h"
#include "Engine/Scene/DefensePracticeSession.h"
#include "Engine/Scene/ExplorationFieldGuide.h"
#include "Engine/Scene/LandmarkEncounter.h"
#include "Engine/Scene/LandmarkInteraction.h"
#include "Engine/Scene/ManaReactorExpedition.h"
#include "Engine/Scene/ManaReactorMission.h"
#include "Engine/Scene/ShadowCryptExpedition.h"
#include "Engine/Scene/ShadowCryptMission.h"
#include "Engine/Scene/ShadowbladeLoadout.h"
#include "Engine/Scene/ShadowbladeLoadoutWorkbench.h"
#include "Engine/Scene/ShadowbladeTrainingPath.h"
#include "Engine/Scene/ShadowbladeTrainingCoach.h"

// Passes 25-27 need existing out-of-line gameplay implementations in this
// registered aggregation target. Shared CMake remains owned by the engine worker,
// so compile the exact production implementations into this test TU instead of
// changing target ownership or substituting mocks.
#include "Engine/Scene/LandmarkEncounter.cpp"
#include "Engine/Scene/LandmarkInteraction.cpp"
#include "Engine/Scene/WorldBlockout.cpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>

namespace {
int failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.001f;
}
