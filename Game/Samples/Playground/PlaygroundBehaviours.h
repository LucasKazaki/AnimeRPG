#pragma once

// Playground sample: a small third-person collect-a-thon built only from
// engine features (the role of Unity's Roll-a-Ball / UE's Third Person
// template). Scenes wire these behaviours up by name:
//
//   PlayerController  camera-relative walk/sprint/jump on a CharacterMover
//                     from the "Move", "Sprint" and "Jump" input actions
//   FollowCamera      spring-arm camera: orbit ("Look"), lag, wall collision
//   Collectible       trigger pickup that scores on the GameMode
//   JumpPad           trigger that launches characters
//   Spinner, Bobber   idle motion for props
//   GameMode          score, timer and HUD; plays the win timeline, then
//                     loads the next scene (or restarts)

#include "Engine/Audio/AudioMixer.h"
#include "Engine/Framework/Behaviour.h"
#include "Engine/Math/VectorMath.h"

#include <memory>
#include <string>

namespace Astral::UI {
class Label;
class Widget;
} // namespace Astral::UI

namespace Astral::Samples {

using Framework::Entity;

class PlayerController : public Framework::Behaviour {
public:
    float moveSpeed{6.0f};
    float sprintMultiplier{1.7f};
    float turnSpeed{14.0f};      // facing smoothing rate (1/s)
    std::string camera{"Camera"}; // movement is relative to this entity's view
    float killHeight{-25.0f};    // falling below respawns at the start

    void OnStart() override;
    void OnFixedUpdate(float dt) override;

    int Respawns() const { return respawns_; }
    int Jumps() const { return jumps_; }

private:
    Math::Vec3 spawn_{};
    Entity cameraEntity_{};
    int respawns_{};
    int jumps_{};
};

class FollowCamera : public Framework::Behaviour {
public:
    std::string target{"Player"};
    float distance{7.0f};
    float height{1.6f};        // pivot above the target's origin
    float pitchDegrees{18.0f}; // positive looks down
    float yawDegrees{0.0f};
    float turnDegreesPerSecond{140.0f}; // "Look" action x
    float lag{10.0f};                   // position smoothing rate (1/s); 0 = none
    float wallMargin{0.3f};
    unsigned collisionMask{1u};

    void OnLateUpdate(float dt) override;

private:
    Entity targetEntity_{};
    Math::Vec3 position_{};
    bool placed_{};
};

class Collectible : public Framework::Behaviour {
public:
    int points{1};
    void OnTriggerEnter(const Framework::CollisionInfo& info) override;
    bool Collected() const { return collected_; }

private:
    bool collected_{};
};

class JumpPad : public Framework::Behaviour {
public:
    float launchSpeed{13.0f};
    float keepHorizontal{1.0f}; // fraction of the character's horizontal speed kept
    void OnTriggerEnter(const Framework::CollisionInfo& info) override;
    int Launches() const { return launches_; }

private:
    int launches_{};
};

class Spinner : public Framework::Behaviour {
public:
    float degreesPerSecond{90.0f};
    Math::Vec3 axis{0.0f, 1.0f, 0.0f};
    void OnUpdate(float dt) override;
};

class Bobber : public Framework::Behaviour {
public:
    float amplitude{0.25f};
    float frequency{0.5f}; // Hz
    float phase{0.0f};     // cycles
    void OnStart() override;
    void OnUpdate(float dt) override;

private:
    Math::Vec3 base_{};
    float time_{};
};

class GameMode : public Framework::Behaviour {
public:
    std::string title{"Playground"};
    std::string nextScene;     // "" restarts this scene
    float restartDelay{4.0f};  // seconds after winning
    std::string winTimeline;   // entity with a Timeline behaviour played on winning

    void OnStart() override;
    void OnUpdate(float dt) override;
    void OnDestroy() override;

    // Called by collectibles.
    void Collect(int points, Math::Vec3 position);
    int Score() const { return score_; }
    int Collected() const { return collected_; }
    int Total() const { return total_; }
    bool Won() const { return won_; }
    float Elapsed() const { return elapsed_; }
    const UI::Label* StatusLabel() const { return status_; }
    const UI::Label* Banner() const { return banner_; }

private:
    void Refresh();

    int score_{};
    int collected_{};
    int total_{};
    bool won_{};
    float elapsed_{};
    UI::Widget* panel_{};
    UI::Label* status_{};
    UI::Label* timer_{};
    UI::Label* banner_{};
    std::shared_ptr<Audio::AudioClip> chime_;
};

// Registers the sample behaviours with the framework's built-ins ("Timeline").
void RegisterPlaygroundBehaviours();

} // namespace Astral::Samples
