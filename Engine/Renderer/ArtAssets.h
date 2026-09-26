#pragma once

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <memory>
#include <string>

namespace Astral::Renderer {

class ArtAssets {
public:
    ArtAssets();
    ~ArtAssets();

    ArtAssets(const ArtAssets&) = delete;
    ArtAssets& operator=(const ArtAssets&) = delete;

    Gdiplus::Image* WorldBackdrop() const {
        return worldBackdropBitmap_ ? worldBackdropBitmap_.get() : worldBackdrop_.get();
    }
    Gdiplus::Image* Shadowblade() const {
        return shadowbladeBitmap_ ? shadowbladeBitmap_.get() : shadowblade_.get();
    }
    Gdiplus::Image* TrainingDummy() const {
        return trainingDummyBitmap_ ? trainingDummyBitmap_.get() : trainingDummy_.get();
    }
    Gdiplus::Image* AstralSigil() const {
        return astralSigilBitmap_ ? astralSigilBitmap_.get() : astralSigil_.get();
    }
    bool IsReady() const;

private:
    bool LoadImage(const std::wstring& fileName,
        std::unique_ptr<Gdiplus::Image>& destination);
    void BuildRuntimeBitmaps();
    std::unique_ptr<Gdiplus::Bitmap> MakeRuntimeBitmap(
        Gdiplus::Image* source, int width, int height) const;

    ULONG_PTR gdiplusToken_{};
    std::unique_ptr<Gdiplus::Image> worldBackdrop_;
    std::unique_ptr<Gdiplus::Image> shadowblade_;
    std::unique_ptr<Gdiplus::Image> trainingDummy_;
    std::unique_ptr<Gdiplus::Image> astralSigil_;
    std::unique_ptr<Gdiplus::Bitmap> worldBackdropBitmap_;
    std::unique_ptr<Gdiplus::Bitmap> shadowbladeBitmap_;
    std::unique_ptr<Gdiplus::Bitmap> trainingDummyBitmap_;
    std::unique_ptr<Gdiplus::Bitmap> astralSigilBitmap_;
};

} // namespace Astral::Renderer
