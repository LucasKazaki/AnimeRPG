#pragma once

#include <windows.h>

#include "Engine/Platform/WindowInputMessages.h"

namespace Astral::Tests {

inline void PrepareNoActivate(STARTUPINFOW& startup) {
    startup.dwFlags |= STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNOACTIVATE;
}

class WindowInputTarget {
public:
    bool Bind(HWND window, DWORD processId, DWORD threadId, HANDLE process) {
        window_ = window;
        processId_ = processId;
        threadId_ = threadId;
        process_ = process;
        return process_ && GetProcessId(process_) == processId_ && Verify();
    }

    bool SetKey(WORD key, bool down) const {
        if (!Verify()) return false;
        const UINT scanCode = MapVirtualKeyW(key, MAPVK_VK_TO_VSC);
        LPARAM keyData = 1 | (static_cast<LPARAM>(scanCode) << 16);
        if (!down) keyData |= (static_cast<LPARAM>(1) << 30) | (static_cast<LPARAM>(1) << 31);
        return PostMessageW(window_, down ? Astral::Platform::kTestKeyDownMessage
                                         : Astral::Platform::kTestKeyUpMessage,
            key, keyData) != FALSE;
    }

    bool SendKey(WORD key, DWORD downMilliseconds = 100,
        DWORD releaseMilliseconds = 100) const {
        if (key == VK_ESCAPE) {
            if (!process_ || GetProcessId(process_) != processId_
                || WaitForSingleObject(process_, 0) != WAIT_TIMEOUT) return false;
            return PostMessageW(window_, WM_CLOSE, 0, 0) != FALSE;
        }
        if (!SetKey(key, true)) return false;
        Sleep(downMilliseconds);
        if (!Verify()) return false;
        if (!SetKey(key, false)) return false;
        Sleep(releaseMilliseconds);
        return true;
    }

    bool HoldKey(WORD key, DWORD milliseconds, DWORD releaseMilliseconds = 150) const {
        if (!SetKey(key, true)) return false;
        Sleep(milliseconds);
        if (!SetKey(key, false)) return false;
        Sleep(releaseMilliseconds);
        return true;
    }

private:
    bool VerifyOwner() const {
        if (!window_ || !processId_ || !IsWindow(window_)) return false;
        DWORD actualProcessId = 0;
        GetWindowThreadProcessId(window_, &actualProcessId);
        return actualProcessId == processId_;
    }

    bool Verify(bool requireVisible = true) const {
        if (!VerifyOwner()) return false;
        if (requireVisible && !IsWindowVisible(window_)) return false;
        return GetAncestor(window_, GA_ROOT) == window_;
    }

    HWND window_{};
    DWORD processId_{};
    DWORD threadId_{};
    HANDLE process_{};
};

} // namespace Astral::Tests
