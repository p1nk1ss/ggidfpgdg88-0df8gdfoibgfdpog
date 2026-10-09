#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "world.h"
#include "../../globals/globals.h"
#include "../../variables/variables.h"
#include "../../../memory/memory.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/sdk.h"
#include "../../../render/menu/library.h"
#include "../../../render/menu/allsetting.h"
#include <thread>
#include <chrono>

namespace World {
namespace {
void Tick() {
    if (!Globals::dataModel.Addr)
        return;
    auto lighting = Globals::dataModel.FindChildByClass("Lighting");
    if (!lighting.Addr)
        return;

    if (variables::World::timeEnabled)
        memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, variables::World::clockTime);

    if (variables::World::skyEnabled) {
        auto sky = lighting.FindChildByClass("Sky");
        if (sky.Addr) {
            memory->write<float>(sky.Addr + Offsets::Sky::SunAngularSize, variables::World::sunSize);
            memory->write<float>(sky.Addr + Offsets::Sky::MoonAngularSize, variables::World::moonSize);
            memory->write<int>(sky.Addr + Offsets::Sky::StarCount, (int)variables::World::starCount);
        }
    }
}
}

void Loop() {
    using namespace std::chrono_literals;
    while (Globals::running) {
        if (memory->IsConnected())
            Tick();
        std::this_thread::sleep_for(100ms);
    }
}

void RenderMenu() {
    AllSetting::BeginPanel("Time & Sky", 0);
    AllSetting::Checkbox("Enable Time Override", &variables::World::timeEnabled);
    AllSetting::Slider("Clock Time", &variables::World::clockTime, 0.0f, 24.0f, "%.1f");
    AllSetting::Checkbox("Enable Sky Edit", &variables::World::skyEnabled);
    AllSetting::Slider("Sun Size", &variables::World::sunSize, 0.0f, 180.0f, "%.1f");
    AllSetting::Slider("Moon Size", &variables::World::moonSize, 0.0f, 180.0f, "%.1f");
    AllSetting::Slider("Star Count", &variables::World::starCount, 0.0f, 20000.0f, "%.0f");

    AllSetting::BeginPanel("Aspect Stretch", 1);
    AllSetting::Checkbox("Enable Stretch", &variables::Aspect::enabled);
    AllSetting::Slider("Horizontal (X)", &variables::Aspect::scaleX, 0.2f, 5.0f, "%.2f");
    AllSetting::Slider("Vertical (Y)", &variables::Aspect::scaleY, 0.2f, 5.0f, "%.2f");
    AllSetting::Slider("Depth (Z)", &variables::Aspect::scaleZ, 0.2f, 5.0f, "%.2f");
}
}
