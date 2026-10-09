#pragma once
#include "../../../sdk/w2s.h"
#include "../../../core/cache/cache.h"
#include "../../../core/variables/variables.h"
#include "../../../sdk/sdk.h"
#include <windows.h>
#include <cstdint>
#include <vector>
#include <atomic>

namespace Aimbot {
inline std::uintptr_t lockedPlayerAddr = 0;
extern std::atomic<bool> g_ms_L, g_ms_R, g_ms_M, g_ms_X1, g_ms_X2;
extern std::atomic<int> g_hookStatus;

void MoveMouse(float x, float y);
void AutoClick();
float GetDistance2D(const RBX::Vec2& a, const RBX::Vec2& b);
bool IsAimKeyDown(int vk);
bool IsTargetVisible(const RBX::Vec3& worldPos);
void CollectHitboxParts(std::uintptr_t characterAddr, int hitbox, std::vector<std::uintptr_t>& out);
RBX::Vec3 PartWorldPos(std::uintptr_t partAddr);
void WriteMemoryAngles(const RBX::Vec3& targetWorld);
void RunAimbot(const RBX::Mat4& view);
void InstallMouseHook();
void AimLoop();
extern std::atomic<std::uintptr_t> g_aimPartAddr;
}
