#include "aim.h"
#include "../../cache/workspace.h"
#include "../../globals/globals.h"
#include "../../../memory/memory.h"
#include "../../../sdk/offsets.h"
#include "../../../../ext/imgui/imgui.h"
#include <cmath>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <thread>

namespace Aimbot {
std::atomic<bool> g_ms_L{false}, g_ms_R{false}, g_ms_M{false}, g_ms_X1{false}, g_ms_X2{false};
std::atomic<int> g_hookStatus{0};
std::atomic<bool> g_kb[256]{};
std::atomic<std::uintptr_t> g_aimPartAddr{0};

namespace {
HHOOK g_mouseHook = nullptr;
HHOOK g_kbHook = nullptr;
std::thread g_hookThread;
std::atomic<bool> g_hookThreadStarted{false};

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void HookThread() {}
}

void InstallMouseHook() {}

void MoveMouse(float, float) {}

void AutoClick() {}

float GetDistance2D(const RBX::Vec2&, const RBX::Vec2&) { return 0.0f; }

static int ImGuiKeyToVK(int) { return 0; }

bool IsAimKeyDown(int) { return false; }

bool IsTargetVisible(const RBX::Vec3&) { return false; }

void CollectHitboxParts(std::uintptr_t, int, std::vector<std::uintptr_t>&) {}

RBX::Vec3 PartWorldPos(std::uintptr_t) { return {}; }

void WriteMemoryAnglesSmoothed(const RBX::Vec3&, float) {}

void WriteMemoryAngles(const RBX::Vec3&) {}

namespace {
bool BestPartScreen(const PlayerCache::CachedPlayer&, const RBX::Mat4&, const RBX::Vec2&, float, RBX::Vec2&, RBX::Vec3&) {
    return false;
}
}

void RunAimbot(const RBX::Mat4&) {}

void AimLoop() {}
}
