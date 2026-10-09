#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "mics.h"
#include "../../../render/menu/library.h"
#include "../../../render/menu/allsetting.h"
#include "../../globals/globals.h"
#include "../../variables/variables.h"
#include "../../cache/cache.h"
#include "../../../memory/memory.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/sdk.h"
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace Mics {
namespace {
struct SizeBackup {
    RBX::Vec3 original;
    bool valid = false;
};
std::unordered_map<std::uintptr_t, SizeBackup> g_sizeBackup;
std::mutex g_sizeMtx;

void ApplyPartStretch(std::uintptr_t partAddr, const RBX::Vec3& scale) {
    if (!partAddr)
        return;
    const uintptr_t prim = memory->read<uintptr_t>(partAddr + Offsets::BasePart::Primitive);
    if (!prim)
        return;
    std::lock_guard<std::mutex> lk(g_sizeMtx);
    auto& b = g_sizeBackup[partAddr];
    if (!b.valid) {
        b.original = memory->read<RBX::Vec3>(prim + Offsets::Primitive::Size);
        if (b.original.X <= 0 || b.original.Y <= 0 || b.original.Z <= 0)
            return;
        b.valid = true;
    }
    const RBX::Vec3 stretched{b.original.X * scale.X, b.original.Y * scale.Y, b.original.Z * scale.Z};
    memory->write<RBX::Vec3>(prim + Offsets::Primitive::Size, stretched);
}

void RestoreAll() {
    std::lock_guard<std::mutex> lk(g_sizeMtx);
    for (auto& [addr, b] : g_sizeBackup) {
        if (!b.valid) continue;
        const uintptr_t prim = memory->read<uintptr_t>(addr + Offsets::BasePart::Primitive);
        if (prim)
            memory->write<RBX::Vec3>(prim + Offsets::Primitive::Size, b.original);
    }
    g_sizeBackup.clear();
}

void AspectTick() {
    static bool wasOn = false;
    if (!variables::Aspect::enabled) {
        if (wasOn) {
            RestoreAll();
            wasOn = false;
        }
        return;
    }
    wasOn = true;
    const RBX::Vec3 s{variables::Aspect::scaleX, variables::Aspect::scaleY, variables::Aspect::scaleZ};
    for (auto& p : PlayerCache::players) {
        if (!p.isValid || !p.characterAddr)
            continue;
        if (p.characterAddr == Globals::localPlayer.GetModelRef().Addr)
            continue;
        const auto& l = PlayerCache::GetLimbs(p.characterAddr);
        const std::uintptr_t parts[] = {l.head, l.hrp, l.torso, l.upperTorso, l.lowerTorso,
            l.lUpperArm, l.lLowerArm, l.lHand, l.rUpperArm, l.rLowerArm, l.rHand,
            l.lUpperLeg, l.lLowerLeg, l.lFoot, l.rUpperLeg, l.rLowerLeg, l.rFoot,
            l.lArm, l.rArm, l.lLeg, l.rLeg};
        for (auto a : parts)
            ApplyPartStretch(a, s);
    }
}
}

void Tick() { AspectTick(); }

void Loop() {
    using namespace std::chrono_literals;
    while (Globals::running) {
        if (memory->IsConnected())
            Tick();
        std::this_thread::sleep_for(50ms);
    }
}

void RenderLocalMenu() {}

void RenderMiscMenu() {
    variables::Movement::flyKey = 0;
    variables::Movement::noclipKey = 0;

    AllSetting::BeginPanel("Fly (always on)", 0);
    AllSetting::Checkbox("Enable Fly", &variables::Movement::fly);
    AllSetting::Slider("Fly Speed", &variables::Movement::flySpeed, 10.0f, 2000.0f, "%.0f");
    AllSetting::Slider("Vertical Boost", &variables::Movement::flyVerticalBoost, 0.5f, 5.0f, "%.1f");
    AllSetting::Checkbox("Bypass (anti-kick)", &variables::Movement::flyBypass);

    AllSetting::BeginPanel("Movement", 1);
    AllSetting::Checkbox("Noclip", &variables::Movement::noclip);
    AllSetting::Checkbox("Bunny Hop", &variables::Movement::bunnyHop);
    AllSetting::Slider("BHop Speed", &variables::Movement::bunnyHopSpeed, 16.0f, 250.0f, "%.0f");
    AllSetting::Checkbox("FOV Override", &variables::Movement::fov);
    AllSetting::Slider("FOV Value", &variables::Movement::fovValue, 30.0f, 150.0f, "%.0f");
}
}
