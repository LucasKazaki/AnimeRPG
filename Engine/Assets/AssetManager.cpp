#include "Engine/Assets/AssetManager.h"

#include "Engine/Assets/Gltf.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Core/Json.h"
#include "Engine/Graphics/Texture.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>

namespace Astral::Assets {

namespace {

Detail::FileState Observe(const std::string& path, const std::string& contentRoot) {
    Detail::FileState state;
    try {
        std::error_code ec;
        const auto root = std::filesystem::canonical(contentRoot.empty() ? "." : contentRoot, ec);
        if (ec) { state.error = ec.value(); return state; }
        const auto target = std::filesystem::weakly_canonical(path, ec);
        if (ec) { state.error = ec.value(); return state; }
        auto part = target.begin();
        std::filesystem::path prefix;
        for (auto rootPart = root.begin(); rootPart != root.end() && part != target.end(); ++rootPart) prefix /= *part++;
        if (!std::filesystem::equivalent(root, prefix, ec) || ec) {
            // Canonical validation may consult metadata, but never collect an
            // outside target's size/time or retain it as a watch. The logical
            // in-root path can transition from this denied state after repair.
            state.error = static_cast<int>(std::errc::permission_denied);
            return state;
        }
        state.target = target.string();
        state.exists = std::filesystem::exists(target, ec);
        if (ec) { state.error = ec.value(); return state; }
        if (!state.exists) return state;
        state.stamp = std::filesystem::last_write_time(target, ec);
        if (ec) { state.error = ec.value(); return state; }
        state.size = std::filesystem::file_size(target, ec);
        state.error = ec.value();
    } catch (const std::system_error& e) {
        state.error = e.code().value();
    }
    return state;
}

bool ReadBytes(const std::string& path, std::vector<std::uint8_t>& out, std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

} // namespace

AssetManager::AssetManager(std::string contentRoot, Core::JobSystem* jobs) : root_(std::move(contentRoot)), jobs_(jobs) {
    while (root_.size() > 1 && (root_.back() == '/' || root_.back() == '\\')) root_.pop_back();
}

AssetManager::~AssetManager() {
    // Jobs capture `this`; never let one outlive the manager.
    if (jobs_ && !inflight_.empty()) jobs_->WaitAll(inflight_);
}

bool AssetManager::ValidPath(const std::string& path) {
    if (path.empty() || path[0] == '/' || path[0] == '\\' || path.find(':') != std::string::npos
        || path.find('\0') != std::string::npos) return false;
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t end = path.find_first_of("/\\", start);
        const std::string segment = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (segment == "..") return false;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return true;
}

std::string AssetManager::Resolve(const std::string& path) const { return root_.empty() ? path : root_ + "/" + path; }

std::shared_ptr<Detail::AssetSlot> AssetManager::LoadSlot(std::type_index type, const std::string& path, bool async) {
    const auto key = std::make_pair(type, path);
    const auto found = cache_.find(key);
    if (found != cache_.end()) {
        const std::shared_ptr<Detail::AssetSlot>& slot = found->second;
        if (!async && slot->pending) WaitForAll(); // a synchronous caller needs the result now
        return slot;
    }
    auto slot = std::make_shared<Detail::AssetSlot>();
    slot->path = path;
    slot->type = type;
    cache_[key] = slot;
    ++loads_;
    if (!ValidPath(path)) {
        slot->state = AssetState::Failed;
        slot->error = "invalid content path '" + path + "' (relative paths without '..' only)";
        ++slot->attempt;
        return slot;
    }
    if (!loaders_.count(type)) {
        slot->state = AssetState::Failed;
        slot->error = "no loader registered for this asset type";
        ++slot->attempt;
        return slot;
    }
    RunLoad(slot, async);
    return slot;
}

void AssetManager::RunLoad(const std::shared_ptr<Detail::AssetSlot>& slot, bool async) {
    const ErasedLoader loader = loaders_.at(slot->type);
    const std::string absolute = Resolve(slot->path);
    auto work = [this, slot, loader, absolute] {
        Finished finished;
        finished.slot = slot;
        finished.source = Observe(absolute, root_);
        LoadInfo info;
        try {
            finished.ok = loader(absolute, finished.data, finished.error, info);
        } catch (const std::exception& e) {
            finished.ok = false;
            finished.error = e.what();
        }
        finished.files = std::move(info.files);
        finished.issues = std::move(info.issues);
        std::lock_guard<std::mutex> lock(finishedMutex_);
        finished_.push_back(std::move(finished));
    };
    if (async && jobs_) {
        slot->pending = true;
        inflight_.push_back(jobs_->Schedule(work));
    } else {
        slot->pending = true;
        work();
        Update();
    }
}

void AssetManager::Publish(Finished finished) {
    Detail::AssetSlot& slot = *finished.slot;
    slot.pending = false;
    slot.source = std::move(finished.source);
    slot.attemptFiles = std::move(finished.files);
    slot.issues = std::move(finished.issues);
    ++slot.attempt;
    // Retain the last-good manifest if this attempt stopped before rediscovering
    // it, but acknowledge each file's current observation once per attempt.
    for (auto& file : slot.goodFiles) file.state = Observe(file.path, root_);
    // Initial import remains compatible with glTF's documented image warnings.
    // A corrupt replacement must not erase an existing model's good images.
    if (finished.ok && slot.version != 0 && !slot.issues.empty()) {
        finished.ok = false;
        finished.error = slot.issues.front();
    }
    if (!finished.ok && finished.error.empty()) finished.error = "asset load failed";
    if (finished.ok) {
        slot.data = std::move(finished.data);
        slot.error.clear();
        slot.state = AssetState::Ready;
        slot.goodFiles = slot.attemptFiles;
        ++slot.version;
    } else if (slot.state == AssetState::Ready) {
        // A failed hot reload keeps the last good data and reports the error.
        slot.error = finished.error;
    } else {
        slot.state = AssetState::Failed;
        slot.error = finished.error;
    }
    if (slot.state != AssetState::Loading) {
        auto callbacks = std::move(slot.onReady);
        slot.onReady.clear();
        for (auto& callback : callbacks) callback();
    }
}

std::size_t AssetManager::Update() {
    std::vector<Finished> batch;
    {
        std::lock_guard<std::mutex> lock(finishedMutex_);
        batch.swap(finished_);
    }
    for (Finished& finished : batch) Publish(std::move(finished));
    inflight_.erase(std::remove_if(inflight_.begin(), inflight_.end(), [](const Core::JobHandle& h) { return h.IsComplete(); }),
        inflight_.end());
    return batch.size();
}

void AssetManager::WaitForAll() {
    if (jobs_ && !inflight_.empty()) jobs_->WaitAll(inflight_);
    inflight_.clear();
    Update();
}

void AssetManager::OnReadySlot(const std::string& path, std::type_index type, std::function<void()> callback) {
    const auto found = cache_.find(std::make_pair(type, path));
    if (found == cache_.end()) return;
    if (found->second->state == AssetState::Loading || found->second->pending) {
        found->second->onReady.push_back(std::move(callback));
    } else {
        callback();
    }
}

bool AssetManager::Reload(const std::string& path) {
    bool any = false;
    for (auto& entry : cache_) {
        const std::shared_ptr<Detail::AssetSlot>& slot = entry.second;
        if (slot->path != path || slot->pending || !loaders_.count(slot->type) || !ValidPath(slot->path)) continue;
        RunLoad(slot, false);
        ++reloads_;
        any = true;
    }
    return any;
}

std::size_t AssetManager::PollChanges() {
    std::vector<std::string> changed;
    for (const auto& entry : cache_) {
        const Detail::AssetSlot& slot = *entry.second;
        if (slot.pending || !ValidPath(slot.path) || !loaders_.count(slot.type)) continue;
        bool different = Observe(Resolve(slot.path), root_) != slot.source;
        const auto check = [&](const std::vector<Detail::FileObservation>& files) {
            for (const auto& file : files) if (Observe(file.path, root_) != file.state) return true;
            return false;
        };
        if (!different) different = check(slot.attemptFiles);
        // Compare old good dependencies against their most recent observation,
        // not their successful stamp, otherwise an unchanged failed edit loops.
        if (!different) {
            for (const auto& file : slot.goodFiles) {
                const auto latest = std::find_if(slot.attemptFiles.begin(), slot.attemptFiles.end(),
                    [&](const auto& attempted) { return attempted.path == file.path; });
                if (latest == slot.attemptFiles.end() && Observe(file.path, root_) != file.state) { different = true; break; }
            }
        }
        if (different) changed.push_back(slot.path);
    }
    std::sort(changed.begin(), changed.end());
    changed.erase(std::unique(changed.begin(), changed.end()), changed.end());
    std::size_t reloaded = 0;
    for (const std::string& path : changed) reloaded += Reload(path) ? 1u : 0u;
    return reloaded;
}

std::size_t AssetManager::Collect() {
    std::size_t removed = 0;
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (it->second.use_count() == 1 && !it->second->pending) {
            it = cache_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

AssetManagerStats AssetManager::Stats() const {
    AssetManagerStats stats;
    stats.cached = cache_.size();
    for (const auto& entry : cache_) {
        switch (entry.second->state) {
        case AssetState::Ready: ++stats.ready; break;
        case AssetState::Failed: ++stats.failed; break;
        case AssetState::Loading: ++stats.loading; break;
        }
    }
    stats.loads = loads_;
    stats.reloads = reloads_;
    return stats;
}

void AssetManager::RegisterDefaultLoaders() {
    RegisterLoader<Graphics::Texture2D>([](const std::string& path, Graphics::Texture2D& out, std::string& error) {
        std::vector<std::uint8_t> bytes;
        if (!ReadBytes(path, bytes, error)) return false;
        Graphics::ImageRgba8 image;
        const bool png = bytes.size() >= 8 && bytes[0] == 0x89 && bytes[1] == 'P';
        const bool ok = png ? Graphics::DecodePng(bytes.data(), bytes.size(), image, error)
                            : DecodeJpeg(bytes.data(), bytes.size(), image, error);
        if (!ok) return false;
        out.FromImage(image, true, true);
        out.name = path;
        return true;
    });
    GltfImportOptions gltfOptions;
    gltfOptions.approvedRoot = root_.empty() ? "." : root_;
    loaders_[std::type_index(typeid(GltfDocument))] = [gltfOptions](const std::string& path,
        std::shared_ptr<void>& out, std::string& error, LoadInfo& info) {
        auto object = std::make_shared<GltfDocument>();
        GltfImportReport report;
        GltfImportOptions options = gltfOptions;
        options.report = &report;
        options.observeFile = [&](const std::string& file) {
            if (std::none_of(info.files.begin(), info.files.end(), [&](const auto& observed) { return observed.path == file; }))
                info.files.push_back({file, Observe(file, gltfOptions.approvedRoot)});
        };
        const bool ok = ImportGltf(path, *object, error, options);
        info.issues = std::move(report.imageErrors);
        if (ok) out = std::move(object);
        return ok;
    };
    RegisterLoader<Audio::AudioClip>([](const std::string& path, Audio::AudioClip& out, std::string& error) {
        return Audio::ReadWav(path, out, error);
    });
    RegisterLoader<Core::JsonValue>([](const std::string& path, Core::JsonValue& out, std::string& error) {
        return Core::ReadJsonFile(path, out, error);
    });
    RegisterLoader<std::string>([](const std::string& path, std::string& out, std::string& error) {
        std::vector<std::uint8_t> bytes;
        if (!ReadBytes(path, bytes, error)) return false;
        out.assign(bytes.begin(), bytes.end());
        return true;
    });
}

} // namespace Astral::Assets
