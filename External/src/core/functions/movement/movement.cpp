#include "movement.h"
#include "../../variables/variables.h"
#include "../../globals/globals.h"
#include "../../cache/cache.h"
#include "../aim/aim.h"
#include "../../../memory/memory.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/sdk.h"
#include "../../../sdk/math.h"
#include <windows.h>
#include <thread>
#include <chrono>
#include <cmath>

namespace Movement {
namespace {
bool KeyActive(int key, int mode, bool& tog, bool& was) {
    if (key == 0)
        return true;
    const bool down = Aimbot::IsAimKeyDown(key);
    if (mode == 1) {
        if (down && !was)
            tog = !tog;
        was = down;
        return tog;
    }
    if (mode == 2)
        return true;
    was = down;
    return down;
}

bool RobloxFocused() {
    HWND hwnd = FindWindowW(nullptr, L"Roblox");
    return hwnd && IsWindow(hwnd) && GetForegroundWindow() == hwnd;
}

void SetVel(std::uintptr_t prim, const rbx::vector3_t& vel) {
    if (!prim)
        return;
    memory->write<rbx::vector3_t>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
    memory->write<rbx::vector3_t>(prim + Offsets::Primitive::AssemblyAngularVelocity, rbx::vector3_t(0.0f, 0.0f, 0.0f));
}

void TickFov(bool& wasOn) {
    static bool tog = false;
    static bool was = false;
    const bool on = variables::Movement::fov && KeyActive(variables::Movement::fovKey, variables::Movement::fovKeyMode, tog, was);
    if (on && Globals::camera.Addr) {
        wasOn = true;
        memory->write<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView, variables::Movement::fovValue * (3.14159265f / 180.0f));
    } else if (wasOn) {
        wasOn = false;
        if (Globals::camera.Addr)
            memory->write<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView, 70.0f * (3.14159265f / 180.0f));
    }
}

void TickFly(bool& gravOver, float& gravBackup) {
    static bool tog = false;
    static bool was = false;
    static std::uintptr_t pstandHum = 0;
    static int bypassCycle = 0;
    const bool on = variables::Movement::fly && KeyActive(variables::Movement::flyKey, variables::Movement::flyKeyMode, tog, was);

    std::uintptr_t prim = 0;
    std::uintptr_t hum = 0;
    auto localChar = Globals::localPlayer.GetModelRef();
    if (localChar.Addr) {
        const auto& limbs = PlayerCache::GetLimbs(localChar.Addr);
        hum = limbs.humanoid;
        if (limbs.hrp)
            prim = memory->read<std::uintptr_t>(limbs.hrp + Offsets::BasePart::Primitive);
    }
    const std::uintptr_t cam = Globals::workspace.Addr
        ? memory->read<std::uintptr_t>(Globals::workspace.Addr + Offsets::Workspace::CurrentCamera)
        : 0;
    const std::uintptr_t world = Globals::workspace.Addr
        ? memory->read<std::uintptr_t>(Globals::workspace.Addr + Offsets::Workspace::World)
        : 0;

    auto cleanup = [&]() {
        if (prim)
            SetVel(prim, rbx::vector3_t(0.0f, 0.0f, 0.0f));
        if (gravOver && world)
            memory->write<float>(world + Offsets::World::Gravity, gravBackup);
        gravOver = false;
        if (pstandHum)
            memory->write<bool>(pstandHum + Offsets::Humanoid::PlatformStand, false);
        pstandHum = 0;
        bypassCycle = 0;
    };

    if (!on || !prim || !cam) {
        cleanup();
        return;
    }

    if (world) {
        if (!gravOver) {
            gravBackup = memory->read<float>(world + Offsets::World::Gravity);
            gravOver = true;
        }
        memory->write<float>(world + Offsets::World::Gravity, 0.0f);
    }
    if (hum) {
        memory->write<bool>(hum + Offsets::Humanoid::PlatformStand, true);
        pstandHum = hum;
    }

    if (!RobloxFocused() && variables::Movement::flyKey != 0) {
        SetVel(prim, rbx::vector3_t(0.0f, 0.0f, 0.0f));
        return;
    }

    rbx::matrix3_t rot{};
    for (int i = 0; i < 9; ++i)
        rot.data[i] = memory->read<float>(cam + Offsets::Camera::Rotation + (std::uint64_t)i * sizeof(float));
    rbx::vector3_t fwd(-rot.data[2], -rot.data[5], -rot.data[8]);
    rbx::vector3_t right(rot.data[0], rot.data[3], rot.data[6]);
    if (fwd.magnitude() < 1e-4f) fwd = rbx::vector3_t(0, 0, 1); else fwd = fwd.normalize();
    if (right.magnitude() < 1e-4f) right = rbx::vector3_t(1, 0, 0); else right = right.normalize();

    const float spd = variables::Movement::flySpeed > 0.0f ? variables::Movement::flySpeed : 0.0f;
    auto held = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };

    rbx::vector3_t want{};
    if (held('W')) want = want + fwd * spd;
    if (held('S')) want = want - fwd * spd;
    if (held('D')) want = want + right * spd;
    if (held('A')) want = want - right * spd;
    if (held(VK_SPACE))   want.y += spd * variables::Movement::flyVerticalBoost;
    if (held(VK_CONTROL)) want.y -= spd * variables::Movement::flyVerticalBoost;

    if (variables::Movement::flyBypass) {
        ++bypassCycle;
        if (bypassCycle % 7 == 0) {
            SetVel(prim, rbx::vector3_t(0.0f, 0.0f, 0.0f));
            return;
        }
    }

    SetVel(prim, want);
}

void SetCollide(std::uintptr_t partAddr, bool collide) {
    if (!partAddr)
        return;
    const uintptr_t prim = memory->read<uintptr_t>(partAddr + Offsets::BasePart::Primitive);
    if (!prim)
        return;
    uint8_t flags = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
    const uint8_t bit = (uint8_t)Offsets::PrimitiveFlags::CanCollide;
    const uint8_t next = collide ? (uint8_t)(flags | bit) : (uint8_t)(flags & ~bit);
    if (next != flags)
        memory->write<uint8_t>(prim + Offsets::Primitive::Flags, next);
}

void TickNoclip(bool& wasOn) {
    static bool tog = false;
    static bool was = false;
    const bool on = variables::Movement::noclip && KeyActive(variables::Movement::noclipKey, variables::Movement::noclipKeyMode, tog, was);
    auto localChar = Globals::localPlayer.GetModelRef();
    if (!on) {
        if (wasOn && localChar.Addr) {
            const auto& limbs = PlayerCache::GetLimbs(localChar.Addr);
            const std::uintptr_t parts[] = {limbs.hrp, limbs.head, limbs.torso, limbs.upperTorso, limbs.lowerTorso, limbs.lUpperArm, limbs.lLowerArm, limbs.lHand, limbs.rUpperArm, limbs.rLowerArm, limbs.rHand, limbs.lUpperLeg, limbs.lLowerLeg, limbs.lFoot, limbs.rUpperLeg, limbs.rLowerLeg, limbs.rFoot, limbs.lArm, limbs.rArm, limbs.lLeg, limbs.rLeg};
            for (auto p : parts)
                SetCollide(p, true);
        }
        wasOn = false;
        return;
    }
    if (!localChar.Addr)
        return;
    wasOn = true;
    const auto& limbs = PlayerCache::GetLimbs(localChar.Addr);
    if (variables::Movement::noclipMode == 1) {
        SetCollide(limbs.hrp, false);
        return;
    }
    const std::uintptr_t parts[] = {limbs.hrp, limbs.head, limbs.torso, limbs.upperTorso, limbs.lowerTorso, limbs.lUpperArm, limbs.lLowerArm, limbs.lHand, limbs.rUpperArm, limbs.rLowerArm, limbs.rHand, limbs.lUpperLeg, limbs.lLowerLeg, limbs.lFoot, limbs.rUpperLeg, limbs.rLowerLeg, limbs.rFoot, limbs.lArm, limbs.rArm, limbs.lLeg, limbs.rLeg};
    for (auto p : parts)
        SetCollide(p, false);
}

void TickBhop() {
    if (!variables::Movement::bunnyHop)
        return;
    if (!Aimbot::IsAimKeyDown(VK_SPACE))
        return;
    auto localChar = Globals::localPlayer.GetModelRef();
    if (!localChar.Addr)
        return;
    const auto& limbs = PlayerCache::GetLimbs(localChar.Addr);
    if (!limbs.hrp)
        return;
    const std::uintptr_t prim = memory->read<std::uintptr_t>(limbs.hrp + Offsets::BasePart::Primitive);
    if (!prim)
        return;
    const RBX::Vec3 vel = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
    if (std::abs(vel.Y) < 3.0f) {
        const float boost = std::clamp(variables::Movement::bunnyHopSpeed, 16.0f, 250.0f) / 16.0f;
        RBX::Vec3 newVel{vel.X * boost, 50.0f, vel.Z * boost};
        memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, newVel);
    }
    if (limbs.hasRealHumanoid && limbs.humanoid)
        memory->write<bool>(limbs.humanoid + Offsets::Humanoid::Jump, true);
}
}

void Loop() {
    using namespace std::chrono_literals;
    bool fovOn = false;
    bool gravOver = false;
    float gravBackup = 0.0f;
    bool noclipOn = false;
    while (Globals::running) {
        if (memory->IsConnected() && Globals::localPlayer.Addr) {
            TickFov(fovOn);
            TickFly(gravOver, gravBackup);
            TickNoclip(noclipOn);
            TickBhop();
        }
        std::this_thread::sleep_for(variables::Movement::fly ? 1ms : 8ms);
    }
}
}
