#include "Engine/Renderer/ArtAssets.h"

#include <windows.h>

#include <array>

namespace {

std::wstring DirectoryOf(const std::wstring& path) {
    const std::size_t separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? std::wstring{} : path.substr(0, separator);
}

std::wstring ParentOf(const std::wstring& path) {
    return DirectoryOf(path);
}

std::wstring Join(const std::wstring& directory, const std::wstring& fileName) {
    if (directory.empty()) return fileName;
    return directory + L"\\" + fileName;
}

std::array<std::wstring, 5> AssetCandidates(const std::wstring& fileName) {
    std::array<std::wstring, 5> candidates{};
    wchar_t currentDirectory[MAX_PATH]{};
    wchar_t modulePath[MAX_PATH]{};
    const DWORD currentLength = GetCurrentDirectoryW(MAX_PATH, currentDirectory);
    const DWORD moduleLength = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    const std::wstring current = currentLength > 0 ? currentDirectory : L".";
    const std::wstring moduleDirectory = moduleLength > 0
        ? DirectoryOf(modulePath) : current;
    const std::wstring buildDirectory = ParentOf(moduleDirectory);
    const std::wstring projectDirectory = ParentOf(buildDirectory);

    candidates[0] = Join(current, L"Game\\Assets\\" + fileName);
    candidates[1] = Join(current, L"Assets\\" + fileName);
    candidates[2] = Join(moduleDirectory, L"Assets\\" + fileName);
    candidates[3] = Join(buildDirectory, L"Assets\\" + fileName);
    candidates[4] = Join(projectDirectory, L"Game\\Assets\\" + fileName);
    return candidates;
}

} // namespace

namespace Astral::Renderer {

ArtAssets::ArtAssets() {
    Gdiplus::GdiplusStartupInput startupInput{};
    if (Gdiplus::GdiplusStartup(&gdiplusToken_, &startupInput, nullptr)
        != Gdiplus::Ok) {
        gdiplusToken_ = 0;
        return;
    }

    LoadImage(L"astral_mall_backdrop.png", worldBackdrop_);
    LoadImage(L"shadowblade.png", shadowblade_);
    LoadImage(L"training_dummy.png", trainingDummy_);
    LoadImage(L"astral_sigil.png", astralSigil_);
    BuildRuntimeBitmaps();
}

ArtAssets::~ArtAssets() {
    astralSigilBitmap_.reset();
    trainingDummyBitmap_.reset();
    shadowbladeBitmap_.reset();
    worldBackdropBitmap_.reset();
    astralSigil_.reset();
    trainingDummy_.reset();
    shadowblade_.reset();
    worldBackdrop_.reset();
    if (gdiplusToken_ != 0) Gdiplus::GdiplusShutdown(gdiplusToken_);
}

bool ArtAssets::LoadImage(const std::wstring& fileName,
    std::unique_ptr<Gdiplus::Image>& destination) {
    if (gdiplusToken_ == 0) return false;

    for (const std::wstring& candidate : AssetCandidates(fileName)) {
        if (GetFileAttributesW(candidate.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        std::unique_ptr<Gdiplus::Image> image(
            Gdiplus::Image::FromFile(candidate.c_str(), FALSE));
        if (image && image->GetLastStatus() == Gdiplus::Ok) {
            destination = std::move(image);
            return true;
        }
    }
    return false;
}

void ArtAssets::BuildRuntimeBitmaps() {
    // Generated source art stays in the project for future export work, while
    // the loop draws small cached versions sized for the playtest window.
    worldBackdropBitmap_ = MakeRuntimeBitmap(worldBackdrop_.get(), 1280, 720);
    shadowbladeBitmap_ = MakeRuntimeBitmap(shadowblade_.get(), 256, 384);
    trainingDummyBitmap_ = MakeRuntimeBitmap(trainingDummy_.get(), 256, 384);
    astralSigilBitmap_ = MakeRuntimeBitmap(astralSigil_.get(), 96, 96);
}

std::unique_ptr<Gdiplus::Bitmap> ArtAssets::MakeRuntimeBitmap(
    Gdiplus::Image* source, int width, int height) const {
    if (!source || width <= 0 || height <= 0) return nullptr;

    auto bitmap = std::make_unique<Gdiplus::Bitmap>(width, height,
        PixelFormat32bppARGB);
    if (bitmap->GetLastStatus() != Gdiplus::Ok) return nullptr;

    Gdiplus::Graphics graphics(bitmap.get());
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.DrawImage(source, Gdiplus::Rect(0, 0, width, height), 0, 0,
        static_cast<int>(source->GetWidth()), static_cast<int>(source->GetHeight()),
        Gdiplus::UnitPixel);
    return bitmap;
}

bool ArtAssets::IsReady() const {
    return worldBackdrop_ != nullptr && shadowblade_ != nullptr
        && trainingDummy_ != nullptr && astralSigil_ != nullptr;
}

} // namespace Astral::Renderer
