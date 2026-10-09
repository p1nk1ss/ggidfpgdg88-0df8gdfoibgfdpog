#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../../../ext/imgui/imgui.h"
#include "../../../ext/imgui/imgui_internal.h"

#include <cmath>
#include <string>
#include <unordered_map>

namespace imGuiCustom
{
struct Theme
{
    ImVec4 WindowBg;
    ImVec4 CardBg;
    ImVec4 ControlBg;
    ImVec4 ControlInactive;
    ImVec4 Border;
    ImVec4 Accent;
    ImVec4 AccentText;
    ImVec4 Text;
    ImVec4 TextBright;
    ImVec4 KeybindBg;
};

struct Fonts
{
    ImFont* CascadiaMonoBL = nullptr;
};

inline Theme& GetThemeMutable()
{
    static Theme g_Theme = {
        ImVec4(0.0392f, 0.0392f, 0.0471f, 1.0f),
        ImVec4(0.0745f, 0.0745f, 0.0863f, 1.0f),
        ImVec4(0.1176f, 0.1176f, 0.1333f, 1.0f),
        ImVec4(0.2235f, 0.2235f, 0.2392f, 1.0f),
        ImVec4(1.0f, 1.0f, 1.0f, 0.06f),
        ImVec4(0.0392f, 0.5176f, 1.0f, 1.0f),
        ImVec4(0.0392f, 0.5176f, 1.0f, 1.0f),
        ImVec4(0.8200f, 0.8200f, 0.8400f, 1.0f),
        ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
        ImVec4(0.1176f, 0.1176f, 0.1333f, 1.0f),
    };
    return g_Theme;
}

inline const Theme& GetTheme() { return GetThemeMutable(); }

inline Fonts& GetFontsMutable()
{
    static Fonts g_Fonts = {};
    return g_Fonts;
}

inline const Fonts& GetFonts() { return GetFontsMutable(); }

constexpr float kWindowRounding = 14.0f;
constexpr float kCardRounding = 12.0f;
constexpr float kPopupRounding = 8.0f;
constexpr float kControlRounding = 6.0f;
constexpr float kRowRounding = 5.0f;

inline void ApplyStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    const Theme& g_Theme = GetTheme();
    style.WindowRounding = kWindowRounding;
    style.ChildRounding = kCardRounding;
    style.FrameRounding = kControlRounding;
    style.PopupRounding = kCardRounding;
    style.ScrollbarRounding = kControlRounding;
    style.GrabRounding = kControlRounding;
    style.TabRounding = kControlRounding;
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(0.0f, 0.0f);
    style.FramePadding = ImVec2(0.0f, 0.0f);
    style.ItemSpacing = ImVec2(0.0f, 0.0f);
    style.ItemInnerSpacing = ImVec2(0.0f, 0.0f);
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill = true;
    style.Colors[ImGuiCol_WindowBg] = g_Theme.WindowBg;
    style.Colors[ImGuiCol_PopupBg] = g_Theme.CardBg;
    style.Colors[ImGuiCol_Text] = g_Theme.Text;
}

inline void Initialize(ImFont* cascadiaMonoBL)
{
    GetFontsMutable().CascadiaMonoBL = cascadiaMonoBL;
    ApplyStyle();
}

inline float g_animSpeed = 1.0f;
inline float AnimDelta() { return ImGui::GetIO().DeltaTime * g_animSpeed; }

inline float AnimateFloat(ImGuiID id, bool enabled, float speed = 12.0f)
{
    static std::unordered_map<ImGuiID, float> g_Animations;
    ImGuiIO& io = ImGui::GetIO();
    float& value = g_Animations[id];
    const float target = enabled ? 1.0f : 0.0f;
    value = ImLerp(value, target, ImClamp(AnimDelta() * speed, 0.0f, 1.0f));
    return value;
}

inline ImVec4 LerpColor(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(ImLerp(a.x, b.x, t), ImLerp(a.y, b.y, t), ImLerp(a.z, b.z, t), ImLerp(a.w, b.w, t));
}

inline ImU32 OutlineBlack() { return ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)); }
inline ImU32 OutlineInner() { return ImGui::GetColorU32(IM_COL32(52, 52, 56, 255)); }

inline ImVec2 g_contentOffset = ImVec2(0.0f, 0.0f);
inline float g_fontScale = 1.0f;
inline float FontLabel() { return std::round(15.0f * g_fontScale); }
inline float FontSmall() { return std::round(14.0f * g_fontScale); }
inline float SliderTop() { return FontSmall() + 5.0f; }
inline float SliderStep() { return FontSmall() + 5.0f + 22.0f; }
inline float ComboTop() { return FontSmall() + 3.0f; }
inline float ComboStep() { return 28.0f; }
inline float CheckStep() { return ImMax(FontLabel(), 18.0f) + 8.0f; }

inline ImGuiID& ComboOpenId() { static ImGuiID v = 0; return v; }
inline int& ComboClosedFrame() { static int f = -100000; return f; }
inline bool PopupBlocking() {
    if (ImGui::GetFrameCount() == ComboClosedFrame())
        return true;
    return ComboOpenId() != 0;
}

inline ImU32 ColorU32(const ImVec4& color, float alpha_mul = 1.0f)
{
    ImVec4 c = color;
    c.w *= alpha_mul;
    return ImGui::GetColorU32(c);
}

inline ImU32 Hairline(float alpha_mul = 1.0f) { return ColorU32(GetTheme().Border, alpha_mul); }

inline void DrawShadow(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, float rounding, float alpha_mul = 1.0f)
{
    const int layers = 6;
    for (int i = 0; i < layers; ++i)
    {
        const float spread = (float)(i + 1) * 1.5f;
        const float alpha = 0.07f * (1.0f - (float)i / (float)layers) * alpha_mul;
        draw_list->AddRectFilled(min - ImVec2(spread, spread - 2.0f), max + ImVec2(spread, spread + 2.0f), ColorU32(ImVec4(0.0f, 0.0f, 0.0f, alpha)), rounding + spread);
    }
}

inline void DrawPopupFrame(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, float alpha_mul)
{
    DrawShadow(draw_list, min, max, kPopupRounding, alpha_mul);
    draw_list->AddRectFilled(min, max, ColorU32(GetTheme().ControlBg, alpha_mul), kPopupRounding);
    draw_list->AddRect(min, max, Hairline(alpha_mul), kPopupRounding, 0, 1.0f);
}

inline void DrawCheckmark(ImDrawList* draw_list, const ImVec2& center, float size, ImU32 col)
{
    const ImVec2 pts[3] = {
        ImVec2(center.x - size, center.y),
        ImVec2(center.x - size * 0.3f, center.y + size * 0.7f),
        ImVec2(center.x + size, center.y - size * 0.7f)
    };
    draw_list->AddPolyline(pts, 3, col, 0, 1.5f);
}

inline void AddTextWithOutline(ImDrawList* draw_list, ImFont* font, float font_size, const ImVec2& pos, ImU32 text_col, const char* text)
{
    const ImU32 outline_col = OutlineBlack();
    static const ImVec2 offsets[8] = {
        ImVec2(-1.0f, -1.0f), ImVec2(0.0f, -1.0f), ImVec2(1.0f, -1.0f),
        ImVec2(-1.0f,  0.0f),                      ImVec2(1.0f,  0.0f),
        ImVec2(-1.0f,  1.0f), ImVec2(0.0f,  1.0f), ImVec2(1.0f,  1.0f)
    };
    if (font)
    {
        for (int i = 0; i < 8; ++i)
            draw_list->AddText(font, font_size, ImVec2(pos.x + offsets[i].x, pos.y + offsets[i].y), outline_col, text);
        draw_list->AddText(font, font_size, pos, text_col, text);
    }
    else
    {
        for (int i = 0; i < 8; ++i)
            draw_list->AddText(ImVec2(pos.x + offsets[i].x, pos.y + offsets[i].y), outline_col, text);
        draw_list->AddText(pos, text_col, text);
    }
}
}
