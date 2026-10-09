#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "settings.h"
#include "../../variables/variables.h"
#include "../../../render/menu/library.h"
#include "../../../render/menu/allsetting.h"
#include <windows.h>

namespace Settings {
void RenderAimMenu() {}

void RenderVisualMenu() {
    AllSetting::BeginPanel("ESP Box", 0);
    AllSetting::Checkbox("Enable ESP", &variables::ESP::enabled);
    AllSetting::Checkbox("Boxes", &variables::ESP::boxes);
    const char* boxStyles[] = {"Full", "Corner"};
    AllSetting::Combo("Box Style", &variables::ESP::boxStyle, boxStyles, 2);
    AllSetting::ColorPicker("Box Color", &variables::ESP::boxColor);
    AllSetting::Checkbox("Box Filled", &variables::ESP::boxFilled);
    AllSetting::ColorPicker("Fill Color", &variables::ESP::boxFillColor);
    AllSetting::Checkbox("Skeleton", &variables::ESP::skeleton);
    AllSetting::ColorPicker("Skeleton Color", &variables::ESP::skeletonColor);

    AllSetting::BeginPanel("ESP Info", 1);
    AllSetting::Checkbox("Names", &variables::ESP::names);
    AllSetting::ColorPicker("Name Color", &variables::ESP::nameColor);
    AllSetting::Checkbox("Distance", &variables::ESP::distance);
    AllSetting::Checkbox("Health Bar", &variables::ESP::healthBar);
    AllSetting::ColorPicker("Health Color", &variables::ESP::healthColor);
    AllSetting::Checkbox("Tool", &variables::ESP::tool);
    AllSetting::Checkbox("Dead Check", &variables::ESP::deadCheck);
    AllSetting::Checkbox("Local Player", &variables::ESP::localPlayer);
}

void RenderSettingsMenu() {}

namespace {
struct ThemePreset {
    const char* name;
    ImVec4 background, panels, controls, accent;
};

ImVec4 Rgb(int r, int g, int b) {
    return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
}

const ThemePreset kThemes[] = {
    {"Midnight", Rgb(10, 10, 12), Rgb(19, 19, 22), Rgb(30, 30, 34), Rgb(10, 132, 255)},
    {"Violet", Rgb(12, 10, 18), Rgb(21, 18, 30), Rgb(34, 29, 48), Rgb(169, 112, 255)},
    {"Crimson", Rgb(14, 10, 10), Rgb(24, 17, 18), Rgb(39, 27, 28), Rgb(255, 69, 58)},
    {"Mint", Rgb(9, 13, 12), Rgb(17, 24, 23), Rgb(27, 38, 36), Rgb(48, 209, 88)},
    {"Amber", Rgb(13, 11, 8), Rgb(23, 20, 15), Rgb(37, 32, 24), Rgb(255, 159, 10)},
};
constexpr int kThemeCount = IM_ARRAYSIZE(kThemes);
}

float RenderClientMenu(float width) {
    const float margin = 8.0f;
    const float panel_w = width - margin * 2.0f;

    AllSetting::BeginPanelAt("Subscription", margin, 52.0f, panel_w);
    AllSetting::Info("Plan", variables::Client::plan);
    AllSetting::Info("Expires", variables::Client::expires);
    float y = AllSetting::EndPanel();

    AllSetting::BeginPanelAt("Animation", margin, y + 6.0f, panel_w);
    AllSetting::Slider("Speed", &variables::Misc::animSpeed, 0.5f, 2.0f, "%.1fx");
    y = AllSetting::EndPanel();

    ImVec4 accents[kThemeCount];
    for (int i = 0; i < kThemeCount; ++i)
        accents[i] = kThemes[i].accent;
    AllSetting::BeginPanelAt("Theme", margin, y + 6.0f, panel_w);
    if (AllSetting::Swatches(kThemes[ImClamp(variables::Misc::theme, 0, kThemeCount - 1)].name, &variables::Misc::theme, accents, kThemeCount)) {
        const ThemePreset& preset = kThemes[variables::Misc::theme];
        variables::Theme::background = preset.background;
        variables::Theme::panels = preset.panels;
        variables::Theme::controls = preset.controls;
        variables::Theme::accent = preset.accent;
    }
    AllSetting::ColorPicker("Custom accent", &variables::Theme::accent);
    AllSetting::Slider("Font scale", &variables::Misc::menuFontSize, 0.8f, 1.4f, "%.1f");
    y = AllSetting::EndPanel();

    return y + margin;
}
}
