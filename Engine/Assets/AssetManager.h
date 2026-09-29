#pragma once

// Asset manager, after Unreal's asset registry/streamable manager and Unity's
// Addressables: path-addressed, typed, reference-counted assets with
//   - one cached instance per (type, path) shared through AssetHandle<T>,
//   - synchronous Load and background LoadAsync on the job system (the result
//     is published on the owning thread by Update, never mid-frame),
//   - hot reload: PollChanges re-imports assets whose files changed and bumps
//     their version so users can rebuild derived data,
//   - Collect to drop assets nobody references any more,
//   - loaders per type (textures, glTF, audio, JSON, text are built in).
// Paths are relative to the content root; absolute paths and ".." are refused.
// The handle/publish API is single-threaded (the game thread); loaders run on
// workers and must only touch their own output.

#include "Engine/Core/JobSystem.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

namespace Astral::Assets {

enum class AssetState : std::uint8_t { Loading, Ready, Failed };

namespace Detail {
struct AssetSlot {
    std::string path;     // relative content path
    std::type_index type{typeid(void)};
    AssetState state{AssetState::Loading};
    std::shared_ptr<void> data;
    std::string error;
    std::uint32_t version{};
    std::filesystem::file_time_type stamp{};
    bool pending{}; // an async load is in flight
    std::vector<std::function<void()>> onReady;
};
} // namespace Detail

template <typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(std::shared_ptr<Detail::AssetSlot> slot) : slot_(std::move(slot)) {}

    bool Valid() const { return slot_ != nullptr; }
    AssetState State() const { return slot_ ? slot_->state : AssetState::Failed; }
    bool Ready() const { return slot_ && slot_->state == AssetState::Ready; }
    const T* Get() const { return Ready() ? static_cast<const T*>(slot_->data.get()) : nullptr; }
    const T* operator->() const { return Get(); }
    const std::string& Path() const { return slot_ ? slot_->path : Empty(); }
    const std::string& Error() const { return slot_ ? slot_->error : Empty(); }
    // Increments every time the asset is (re)loaded; compare to rebuild derived data.
    std::uint32_t Version() const { return slot_ ? slot_->version : 0u; }
    bool operator==(const AssetHandle& other) const { return slot_ == other.slot_; }

private:
    static const std::string& Empty() {
        static const std::string empty;
        return empty;
    }
    std::shared_ptr<Detail::AssetSlot> slot_;
};

struct AssetManagerStats {
    std::size_t cached{};
    std::size_t ready{};
    std::size_t failed{};
    std::size_t loading{};
    std::size_t loads{};
    std::size_t reloads{};
};

class AssetManager {
public:
    // Loaders run on worker threads for async loads: absolute path in, object out.
    template <typename T>
    using Loader = std::function<bool(const std::string& absolutePath, T& out, std::string& error)>;

    // `jobs` may be null (async loads then run inline).
    AssetManager(std::string contentRoot, Core::JobSystem* jobs);
    ~AssetManager();
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    template <typename T>
    void RegisterLoader(Loader<T> loader) {
        loaders_[std::type_index(typeid(T))] = [loader = std::move(loader)](const std::string& path,
                                                   std::shared_ptr<void>& out, std::string& error) {
            auto object = std::make_shared<T>();
            if (!loader(path, *object, error)) return false;
            out = std::move(object);
            return true;
        };
    }
    // Registers textures, glTF documents, audio clips, JSON and text loaders.
    void RegisterDefaultLoaders();

    template <typename T>
    AssetHandle<T> Load(const std::string& path) {
        return AssetHandle<T>(LoadSlot(std::type_index(typeid(T)), path, false));
    }
    template <typename T>
    AssetHandle<T> LoadAsync(const std::string& path) {
        return AssetHandle<T>(LoadSlot(std::type_index(typeid(T)), path, true));
    }
    // Runs `callback` when the asset is ready (immediately if it already is).
    template <typename T>
    void OnReady(const AssetHandle<T>& handle, std::function<void()> callback) {
        OnReadySlot(handle.Path(), std::type_index(typeid(T)), std::move(callback));
    }

    // Publishes finished async loads (runs their callbacks). Call once per frame.
    std::size_t Update();
    // Blocks until every in-flight load has finished, then publishes them.
    void WaitForAll();
    // Re-imports assets whose source file timestamp changed. Returns the count.
    std::size_t PollChanges();
    // Forces a reload of one asset (e.g. from a console command).
    bool Reload(const std::string& path);
    // Drops cached assets with no outside handles. Returns the count removed.
    std::size_t Collect();

    std::string Resolve(const std::string& path) const;
    const std::string& ContentRoot() const { return root_; }
    AssetManagerStats Stats() const;

private:
    using ErasedLoader = std::function<bool(const std::string&, std::shared_ptr<void>&, std::string&)>;
    struct Finished {
        std::shared_ptr<Detail::AssetSlot> slot;
        std::shared_ptr<void> data;
        std::string error;
        bool ok{};
        std::filesystem::file_time_type stamp{};
    };

    std::shared_ptr<Detail::AssetSlot> LoadSlot(std::type_index type, const std::string& path, bool async);
    void OnReadySlot(const std::string& path, std::type_index type, std::function<void()> callback);
    void RunLoad(const std::shared_ptr<Detail::AssetSlot>& slot, bool async);
    void Publish(Finished finished);
    static bool ValidPath(const std::string& path);

    std::string root_;
    Core::JobSystem* jobs_;
    std::map<std::type_index, ErasedLoader> loaders_;
    std::map<std::pair<std::type_index, std::string>, std::shared_ptr<Detail::AssetSlot>> cache_;
    std::vector<Core::JobHandle> inflight_;
    std::mutex finishedMutex_;
    std::vector<Finished> finished_;
    std::size_t loads_{}, reloads_{};
};

} // namespace Astral::Assets
