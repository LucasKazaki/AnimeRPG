// Core engine services: JSON, console variables/commands, delegates and the
// event bus, and runtime reflection.

#include "Engine/Core/Console.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Json.h"
#include "Engine/Core/Reflection.h"
#include "Engine/Framework/Scene.h"
#include "Tests/EngineTestSupport.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

using namespace Astral;
using namespace Astral::Core;

// ------------------------------------------------------------------ JSON

ASTRAL_TEST(JsonParsesTheFullGrammar) {
    const char* text = R"({
        // comments are accepted by default
        "name": "Lincol\u006e \"Memorial\"",
        "size": [12, 5.5, -7e-1],
        "flags": {"solid": true, "hidden": false, "none": null},
        "unicode": "\u00e9\ud83d\ude00",
        "big": 1e400, "tiny": -1e-400
    })";
    JsonValue doc;
    std::string error;
    ASTRAL_CHECK(ParseJson(text, doc, error));
    ASTRAL_CHECK(doc.String("name") == "Lincoln \"Memorial\"");
    ASTRAL_CHECK(doc["size"].Size() == 3);
    ASTRAL_CHECK_NEAR(doc["size"][1].AsNumber(), 5.5, 1e-12);
    ASTRAL_CHECK_NEAR(doc["size"][2].AsNumber(), -0.7, 1e-12);
    ASTRAL_CHECK(doc["flags"].Bool("solid") && !doc["flags"].Bool("hidden", true) && doc["flags"]["none"].IsNull());
    ASTRAL_CHECK(doc.String("unicode") == "\xC3\xA9\xF0\x9F\x98\x80"); // é and 😀 as UTF-8
    ASTRAL_CHECK(doc.Number("big") == std::numeric_limits<double>::max());
    ASTRAL_CHECK(doc.Number("tiny") == 0.0);
    ASTRAL_CHECK(doc.Keys().size() == 6 && doc.Keys()[0] == "name"); // insertion order
    ASTRAL_CHECK(doc["missing"]["deeper"][3].IsNull());               // safe chained lookups
    ASTRAL_CHECK(doc.Int("size", 9) == 9);                             // wrong type -> fallback
}

ASTRAL_TEST(JsonRejectsMalformedInputWithPositions) {
    JsonValue doc = JsonValue(42);
    std::string error;
    const char* bad[] = {"", "{", "[1,]", "{\"a\":1,}", "{\"a\" 1}", "[01]", "[1.]", "[.5]", "\"\\x\"", "\"a\nb\"",
        "tru", "{\"a\":1,\"a\":2}", "[1] 2", "\"\\ud800\"", "/* open", "{1:2}", "[-]", "[1e]"};
    for (const char* text : bad) {
        ASTRAL_CHECK(!ParseJson(text, doc, error));
        ASTRAL_CHECK(!error.empty());
    }
    ASTRAL_CHECK(doc.AsInt() == 42); // failures never touch the output
    ASTRAL_CHECK(!ParseJson("{\n  \"a\": [1, 2,\n  }", doc, error));
    ASTRAL_CHECK(error.rfind("3:", 0) == 0); // line 3
    JsonParseOptions strict;
    strict.allowComments = false;
    ASTRAL_CHECK(!ParseJson("[1] // no", doc, error, strict));
    JsonParseOptions shallow;
    shallow.maxDepth = 3;
    ASTRAL_CHECK(!ParseJson("[[[[[1]]]]]", doc, error, shallow));
    ASTRAL_CHECK(ParseJson("[[[1]]]", doc, error, shallow));
    JsonParseOptions trailing;
    trailing.allowTrailingCommas = true;
    ASTRAL_CHECK(ParseJson("{\"a\": [1, 2,],}", doc, error, trailing) && doc["a"].Size() == 2);
}

ASTRAL_TEST(JsonRoundTripsThroughTheWriter) {
    JsonValue doc = JsonValue::MakeObject();
    doc.Set("pi", 3.14159265358979);
    doc.Set("third", 1.0 / 3.0);
    doc.Set("count", static_cast<std::size_t>(12));
    doc.Set("negativeZero", -0.0);
    doc.Set("nan", std::nan(""));
    doc.Set("text", "tab\tquote\"slash\\\x01");
    JsonValue& list = doc.Set("list", JsonValue::MakeArray());
    list.Append(1);
    list.Append(true);
    list.Append(JsonValue::MakeObject()).Set("nested", "yes");
    doc.Set("pi", 3.0); // replaces in place, keeps order
    for (bool pretty : {true, false}) {
        JsonWriteOptions options;
        options.pretty = pretty;
        const std::string text = WriteJson(doc, options);
        JsonValue parsed;
        std::string error;
        ASTRAL_CHECK(ParseJson(text, parsed, error));
        ASTRAL_CHECK(parsed.Number("third") == 1.0 / 3.0); // shortest round-trip is exact
        ASTRAL_CHECK(parsed.Int("count") == 12 && parsed.Number("pi") == 3.0 && parsed.Keys()[0] == "pi");
        ASTRAL_CHECK(parsed["nan"].IsNull());
        ASTRAL_CHECK(parsed.String("text") == "tab\tquote\"slash\\\x01");
        ASTRAL_CHECK(parsed["list"][2].String("nested") == "yes");
        doc.Set("nan", JsonValue());
        ASTRAL_CHECK(parsed == doc);
        doc.Set("nan", std::nan(""));
    }
    ASTRAL_CHECK(doc.Erase("list") && !doc.Has("list") && !doc.Erase("list"));
    ASTRAL_CHECK(JsonValue(std::nan("")).AsInt(-1) == -1 && JsonValue(2.5).AsInt(-1) == -1 && JsonValue(1e30).AsInt(-1) == -1);
}

ASTRAL_TEST(JsonFilesWriteAtomicallyAndReload) {
    const std::string path = (std::filesystem::temp_directory_path() / "astral_json_test.json").string();
    JsonValue doc = JsonValue::MakeObject();
    doc.Set("scene", "Mall");
    std::string error;
    ASTRAL_CHECK(WriteJsonFile(path, doc, error));
    JsonValue loaded;
    ASTRAL_CHECK(ReadJsonFile(path, loaded, error) && loaded == doc);
    // A second write replaces the existing file in place.
    doc.Set("scene", "Reflecting Pool");
    ASTRAL_CHECK(WriteJsonFile(path, doc, error));
    ASTRAL_CHECK(ReadJsonFile(path, loaded, error) && loaded == doc);
    ASTRAL_CHECK(!std::filesystem::exists(path + ".tmp"));
    std::remove(path.c_str());
    ASTRAL_CHECK(!ReadJsonFile(path, loaded, error));
}

// ------------------------------------------------------------------ Console

ASTRAL_TEST(ConsoleVariablesParseClampAndNotify) {
    ConsoleRegistry console;
    ConsoleVariable* quality = console.RegisterInt("r.Quality", 2, "Renderer preset", CVarArchive);
    quality->SetRange(0, 3);
    ConsoleVariable* vsync = console.RegisterBool("r.VSync", true, "Wait for vblank", CVarArchive);
    ConsoleVariable* scale = console.RegisterFloat("t.TimeScale", 1.0f, "Global time scale", CVarCheat);
    ConsoleVariable* name = console.RegisterString("g.PlayerName", "Kaito", "Player name");
    ConsoleVariable* build = console.RegisterString("g.Build", "dev", "Build id", CVarReadOnly);
    int changes = 0;
    quality->OnChanged([&](const ConsoleVariable& v) { changes += v.GetInt(); });

    ASTRAL_CHECK(console.Execute("r.Quality 9") == "r.Quality = 3"); // clamped
    ASTRAL_CHECK(changes == 3);
    ASTRAL_CHECK(console.Execute("R.QUALITY") == "r.Quality = 3"); // case-insensitive
    ASTRAL_CHECK(console.Execute("r.Quality abc").rfind("Error:", 0) == 0 && quality->GetInt() == 3);
    ASTRAL_CHECK(console.Execute("r.VSync off") == "r.VSync = false" && !vsync->GetBool());
    ASTRAL_CHECK(console.Execute("t.TimeScale 0.5").find("requires cheats") != std::string::npos);
    console.EnableCheats(true);
    ASTRAL_CHECK(console.Execute("t.TimeScale 0.5") == "t.TimeScale = 0.5" && scale->GetFloat() == 0.5f);
    ASTRAL_CHECK(console.Execute("g.PlayerName \"Rin Tohsaka\" jr") == "g.PlayerName = Rin Tohsaka jr");
    ASTRAL_CHECK(name->GetString() == "Rin Tohsaka jr");
    ASTRAL_CHECK(console.Execute("g.Build release").find("read-only") != std::string::npos && build->GetString() == "dev");
    ASTRAL_CHECK(console.Execute("reset r.Quality") == "r.Quality = 2");
    ASTRAL_CHECK(console.Execute("r.Quality 2.0") == "r.Quality = 2");
    ASTRAL_CHECK(console.Execute("nope 1") == "Unknown command 'nope'");
    ASTRAL_CHECK(console.History().size() == 11);
    // Same name + type returns the same variable; a type clash is refused.
    ASTRAL_CHECK(console.RegisterInt("r.Quality", 0, "") == quality);
    ASTRAL_CHECK(console.RegisterFloat("r.Quality", 0.0f, "") == nullptr);
}

ASTRAL_TEST(ConsoleCommandsCompletionAndConfig) {
    ConsoleRegistry console;
    std::vector<std::string> received;
    console.RegisterCommand("spawn", "Spawn a prefab", [&](const std::vector<std::string>& args) {
        received = args;
        return "spawned " + std::to_string(args.size());
    });
    console.RegisterInt("r.Quality", 2, "", CVarArchive)->SetRange(0, 3);
    console.RegisterBool("r.Bloom", true, "", CVarArchive);
    console.RegisterFloat("a.Volume", 1.0f, "");
    ASTRAL_CHECK(console.Execute("spawn Crystal \"at player\" 3") == "spawned 3");
    ASTRAL_CHECK(received.size() == 3 && received[1] == "at player");
    ASTRAL_CHECK(console.Complete("r.") == std::vector<std::string>({"r.Bloom", "r.Quality"}));
    ASTRAL_CHECK(console.Complete("SP") == std::vector<std::string>({"spawn"}));
    ASTRAL_CHECK(console.Execute("list r.") == "r.Bloom = true\nr.Quality = 2");
    ASTRAL_CHECK(console.Execute("help r.Quality").find("int, default 2") != std::string::npos);

    std::string error;
    ASTRAL_CHECK(console.LoadConfig("# engine\n[r]\nQuality = 1\nBloom = off\n[]\na.Volume=0.25\nlate.Value = 7\n", error));
    ASTRAL_CHECK(console.Find("r.Quality")->GetInt() == 1 && !console.Find("r.Bloom")->GetBool());
    ASTRAL_CHECK(console.Find("a.Volume")->GetFloat() == 0.25f);
    // Values for variables registered later are applied on registration.
    ASTRAL_CHECK(console.RegisterInt("late.Value", 0, "")->GetInt() == 7);
    // A bad value anywhere rejects the whole file.
    ASTRAL_CHECK(!console.LoadConfig("r.Quality = 0\nr.Bloom = maybe\n", error));
    ASTRAL_CHECK(console.Find("r.Quality")->GetInt() == 1);
    ASTRAL_CHECK(!console.LoadConfig("no equals sign\n", error) && error.find("line 1") != std::string::npos);
    // Only archived, non-default variables are saved; the result reloads.
    const std::string saved = console.SaveConfig();
    ASTRAL_CHECK(saved == "r.Bloom = false\nr.Quality = 1\n");
    ConsoleRegistry fresh;
    fresh.RegisterInt("r.Quality", 2, "", CVarArchive);
    ASTRAL_CHECK(fresh.LoadConfig(saved, error) && fresh.Find("r.Quality")->GetInt() == 1);
    ASTRAL_CHECK(TokenizeCommandLine("a \"b c\" \"d\\\"e\"  ") == std::vector<std::string>({"a", "b c", "d\"e"}));
}

// ------------------------------------------------------------------ Delegates and events

ASTRAL_TEST(MulticastDelegatesSurviveReentrantChanges) {
    MulticastDelegate<int> delegate;
    std::vector<int> calls;
    DelegateHandle second = 0;
    const DelegateHandle first = delegate.Add([&](int v) {
        calls.push_back(v);
        delegate.Remove(second);                            // removes a later handler mid-broadcast
        delegate.Add([&](int w) { calls.push_back(w * 100); }); // joins from the next broadcast
    });
    second = delegate.Add([&](int v) { calls.push_back(-v); });
    delegate.Broadcast(1);
    ASTRAL_CHECK(calls == std::vector<int>({1}));
    ASTRAL_CHECK(delegate.Remove(first));
    delegate.Broadcast(2);
    ASTRAL_CHECK(calls == std::vector<int>({1, 200}));
    // A handler that clears the delegate while running is safe.
    MulticastDelegate<> selfClearing;
    int ran = 0;
    selfClearing.Add([&] {
        ++ran;
        selfClearing.Clear();
    });
    selfClearing.Add([&] { ++ran; });
    selfClearing.Broadcast();
    selfClearing.Broadcast();
    ASTRAL_CHECK(ran == 1 && selfClearing.Count() == 0);
    // Owner-scoped removal.
    int owner = 0;
    MulticastDelegate<> owned;
    owned.Add(&owner, [] {});
    owned.Add(&owner, [] {});
    owned.Add([] {});
    ASTRAL_CHECK(owned.RemoveAll(&owner) == 2 && owned.Count() == 1);
}

namespace {
struct Damaged {
    int amount;
};
struct Healed {
    int amount;
};
} // namespace

ASTRAL_TEST(EventBusPublishesQueuesAndScopes) {
    EventBus bus;
    int damage = 0, heal = 0;
    bus.Subscribe<Damaged>([&](const Damaged& e) {
        damage += e.amount;
        if (e.amount == 5) bus.Queue(Healed{2}); // chained queue delivered in the same Dispatch
    });
    {
        ScopedSubscription scoped = bus.SubscribeScoped<Healed>([&](const Healed& e) { heal += e.amount; });
        bus.Publish(Healed{1});
        bus.Queue(Damaged{5});
        bus.Queue(Damaged{7});
        ASTRAL_CHECK(damage == 0 && bus.QueuedCount() == 2);
        ASTRAL_CHECK(bus.Dispatch() == 3);
        ASTRAL_CHECK(damage == 12 && heal == 3);
        ASTRAL_CHECK(bus.SubscriberCount<Healed>() == 1);
    }
    ASTRAL_CHECK(bus.SubscriberCount<Healed>() == 0);
    bus.Publish(Healed{100});
    ASTRAL_CHECK(heal == 3);
}

// ------------------------------------------------------------------ Reflection

namespace {
enum class LightKind { Directional, Point, Spot };
struct TestLight {
    LightKind kind{LightKind::Point};
    float intensity{1.0f};
    int samples{4};
    bool shadows{true};
    std::string label{"lamp"};
    Math::Vec3 color{1, 1, 1};
    Math::Quat rotation{};
    std::uint8_t priority{3};
};
} // namespace

ASTRAL_TEST(ReflectionSerialisesValidatesAndEdits) {
    TypeRegistry registry;
    registry.Register<TestLight>("TestLight")
        .EnumField("kind", &TestLight::kind, {"Directional", "Point", "Spot"})
        .Field("intensity", &TestLight::intensity, "Brightness")
        .Range(0.0, 50.0)
        .Field("samples", &TestLight::samples)
        .Range(1, 64)
        .Field("shadows", &TestLight::shadows)
        .Field("label", &TestLight::label)
        .Field("color", &TestLight::color)
        .Field("rotation", &TestLight::rotation)
        .Field("priority", &TestLight::priority);
    const TypeInfo* info = registry.Get<TestLight>();
    ASTRAL_CHECK(info && info == registry.Find("TestLight") && info->fields.size() == 8);
    ASTRAL_CHECK(info->Field("intensity")->tooltip == "Brightness" && info->Field("kind")->kind == FieldKind::Enum);

    TestLight light;
    light.kind = LightKind::Spot;
    light.color = {0.5f, 0.25f, 1.0f};
    const JsonValue json = info->ToJson(&light);
    ASTRAL_CHECK(json.String("kind") == "Spot" && json["color"].Size() == 3);

    TestLight loaded;
    std::string error;
    ASTRAL_CHECK(info->FromJson(&loaded, json, error) && error.empty());
    ASTRAL_CHECK(loaded.kind == LightKind::Spot && loaded.color.y == 0.25f && loaded.label == "lamp");

    JsonValue edits;
    std::string parseError;
    ASTRAL_CHECK(ParseJson(R"({"intensity": 99, "samples": 0, "rotation": {"euler": [0, 90, 0]},
        "priority": 900, "extra": 1})", edits, parseError));
    ASTRAL_CHECK(info->FromJson(&loaded, edits, error)); // unknown key reported, not fatal
    ASTRAL_CHECK(error.find("extra") != std::string::npos);
    ASTRAL_CHECK(loaded.intensity == 50.0f && loaded.samples == 1 && loaded.priority == 255); // ranges and type limits
    const Math::Vec3 turned = Math::Rotate(loaded.rotation, Math::Vec3{0, 0, 1});
    ASTRAL_CHECK_NEAR(turned.x, 1.0f, 1e-5); // 90 degrees of yaw turns +Z to +X
    ASTRAL_CHECK(!info->FromJson(&loaded, edits, error, true)); // strict mode rejects unknown keys

    // A bad field anywhere leaves the object untouched (validated on a scratch copy).
    const TestLight before = loaded;
    JsonValue bad;
    ASTRAL_CHECK(ParseJson(R"({"intensity": 2, "kind": "Laser"})", bad, parseError));
    ASTRAL_CHECK(!info->FromJson(&loaded, bad, error) && error.find("Laser") != std::string::npos);
    ASTRAL_CHECK(loaded.intensity == before.intensity);
    ASTRAL_CHECK(!info->SetField(&loaded, "color", JsonValue("red"), error));
    ASTRAL_CHECK(info->SetField(&loaded, "shadows", JsonValue(false), error) && !loaded.shadows);
    ASTRAL_CHECK(info->GetField(&loaded, "label").AsString() == "lamp" && info->GetField(&loaded, "nope").IsNull());

    // Re-registering replaces the definition without dangling lookups.
    registry.Register<TestLight>("TestLight").Field("intensity", &TestLight::intensity);
    ASTRAL_CHECK(registry.Get<TestLight>()->fields.size() == 1 && registry.Names().size() == 1);
}

ASTRAL_TEST(ReflectionRejectsUnrepresentableScalarWithoutMutation) {
    struct Scalars { float finite{7}; float ranged{3}; double wide{11}; };
    TypeRegistry registry;
    registry.Register<Scalars>("Scalars")
        .Field("finite", &Scalars::finite)
        .Field("ranged", &Scalars::ranged).Range(-10, 10)
        .Field("wide", &Scalars::wide);
    const TypeInfo* info = registry.Get<Scalars>();
    Scalars target;
    std::string error;
    const double largest = static_cast<double>(std::numeric_limits<float>::max());
    const double outside = std::nextafter(largest, std::numeric_limits<double>::infinity());
    for (double value : {1e100, -1e100, outside, -outside}) {
        ASTRAL_CHECK(!info->SetField(&target, "finite", JsonValue(value), error));
        ASTRAL_CHECK(target.finite == 7 && !error.empty());
    }
    JsonValue update = JsonValue::MakeObject();
    update.Set("ranged", 9);
    update.Set("wide", 15);
    update.Set("finite", 1e100);
    ASTRAL_CHECK(!info->FromJson(&target, update, error));
    ASTRAL_CHECK(target.finite == 7 && target.ranged == 3 && target.wide == 11);
    for (double value : {largest, -largest, static_cast<double>(std::numeric_limits<float>::denorm_min()), 0.0}) {
        ASTRAL_CHECK(info->SetField(&target, "finite", JsonValue(value), error));
        ASTRAL_CHECK(std::isfinite(target.finite) && target.finite == static_cast<float>(value));
    }
    ASTRAL_CHECK(info->SetField(&target, "ranged", JsonValue(1e100), error) && target.ranged == 10);
    ASTRAL_CHECK(info->SetField(&target, "ranged", JsonValue(-1e100), error) && target.ranged == -10);
    const double doubleLimit = std::numeric_limits<double>::max();
    ASTRAL_CHECK(info->SetField(&target, "wide", JsonValue(doubleLimit), error));
    ASTRAL_CHECK(std::isfinite(target.wide) && target.wide == doubleLimit);
}

ASTRAL_TEST(ReflectedBehaviourRejectsUnrepresentableScalarBeforeAttachment) {
    struct ScalarBehaviour final : Framework::Behaviour { float speed{2}; };
    Framework::BehaviourRegistry types;
    types.Register<ScalarBehaviour>("ScalarBehaviour").Field("speed", &ScalarBehaviour::speed);
    Framework::GameWorld world;
    const World::Entity entity = world.CreateEntity("ReflectedScalar");
    JsonValue properties = JsonValue::MakeObject();
    properties.Set("speed", 1e100);
    std::string error;
    ASTRAL_CHECK(!world.AddBehaviour(entity, "ScalarBehaviour", properties, error, types));
    ASTRAL_CHECK(error.find("speed") != std::string::npos);
    ASTRAL_CHECK(!world.Get<Framework::Behaviours>(entity));
    properties.Set("speed", 12);
    ASTRAL_CHECK(world.AddBehaviour(entity, "ScalarBehaviour", properties, error, types));
    ASTRAL_CHECK(world.GetBehaviour<ScalarBehaviour>(entity)->speed == 12);
}

ASTRAL_TEST(SceneRejectsUnrepresentableReflectedRuntimeScalars) {
    Framework::SceneSerializer serializer;
    Framework::SceneDocument document;
    document.entities.push_back(Framework::EntitySpec{});
    document.entities.front().name = "Retained";
    for (const char* component : {"RigidBody", "CharacterMover"}) {
        const char* field = std::strcmp(component, "RigidBody") == 0 ? "gravityScale" : "gravity";
        JsonValue json = JsonValue::MakeObject();
        json.Set("format", "astral-scene");
        json.Set("version", 1);
        JsonValue entities = JsonValue::MakeArray();
        JsonValue entity = JsonValue::MakeObject(), components = JsonValue::MakeObject(), values = JsonValue::MakeObject();
        entity.Set("name", "Rejected");
        values.Set(field, 1e100);
        components.Set(component, values);
        entity.Set("components", components);
        entities.Append(entity);
        json.Set("entities", entities);
        std::string error;
        ASTRAL_CHECK(!serializer.Parse(json, document, error));
        ASTRAL_CHECK(error.find(field) != std::string::npos);
        ASTRAL_CHECK(document.entities.size() == 1 && document.entities.front().name == "Retained");
    }
}

ASTRAL_TEST_MAIN("EngineServicesTests")
