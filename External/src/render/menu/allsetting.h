#pragma once

#include "library.h"
#include <windows.h>

namespace imGuiCustom
{
inline void DrawSwitch(ImDrawList* draw_list, const ImVec2& min, const ImVec2& size, float t)
{
    const Theme& theme = GetTheme();
    draw_list->AddRectFilled(min, min + size, ColorU32(LerpColor(theme.ControlInactive, theme.Accent, t)), size.y * 0.5f);
    const float knob_r = size.y * 0.5f - 2.0f;
    const ImVec2 knob(min.x + 2.0f + knob_r + (size.x - 4.0f - knob_r * 2.0f) * t, min.y + size.y * 0.5f);
    draw_list->AddCircleFilled(knob + ImVec2(0.0f, 1.0f), knob_r + 0.5f, ColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.25f)));
    draw_list->AddCircleFilled(knob, knob_r, ColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
}

inline bool Checkbox(const char* label, bool* value, const ImVec2& pos)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 base = window->Pos;
    const ImVec2 min = ImVec2(std::floor(base.x + pos.x + g_contentOffset.x), std::floor(base.y + pos.y + g_contentOffset.y));
    const ImVec2 box_size(30.0f, 18.0f);
    const float label_gap = 8.0f;
    const Fonts& fonts = GetFonts();
    ImFont* font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
    const ImVec2 text_size = font->CalcTextSizeA(FontLabel(), FLT_MAX, 0.0f, label);
    ImGui::SetCursorScreenPos(min);
    ImGui::PushID(label);
    const bool pressed = ImGui::InvisibleButton("##checkbox", ImVec2(box_size.x + label_gap + text_size.x, box_size.y));
    if (pressed && !PopupBlocking())
        *value = !*value;

    const ImGuiID id = ImGui::GetItemID();
    const float check = AnimateFloat(id, *value, 14.0f);
    const Theme& theme = GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawSwitch(draw, min, box_size, check);

    draw->AddText(font, FontLabel(), ImVec2(min.x + box_size.x + label_gap, std::floor(min.y + (box_size.y - text_size.y) * 0.5f)), ColorU32(LerpColor(theme.Text, theme.TextBright, check * 0.6f)), label);
    ImGui::PopID();
    return pressed;
}

inline bool ParseHexColor(const char* text, ImVec4& out) {
    if (!text)
        return false;
    while (*text == ' ' || *text == '\t')
        ++text;
    if (*text == '#')
        ++text;
    size_t len = 0;
    while (text[len] != '\0' && len < 9)
        ++len;
    if ((len != 6 && len != 8) || text[len] != '\0')
        return false;
    auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    unsigned int v[8];
    for (size_t i = 0; i < len; ++i) {
        int h = hexVal(text[i]);
        if (h < 0)
            return false;
        v[i] = (unsigned int)h;
    }
    out.x = (float)(v[0] * 16 + v[1]) / 255.0f;
    out.y = (float)(v[2] * 16 + v[3]) / 255.0f;
    out.z = (float)(v[4] * 16 + v[5]) / 255.0f;
    out.w = (len == 8) ? (float)(v[6] * 16 + v[7]) / 255.0f : out.w;
    return true;
}

inline bool SliderFloat(const char* label, float* value, float min_value, float max_value, const ImVec2& pos, float width, const char* text_label, const char* format);

inline bool ColorSquare(const char* id_text, ImVec4* color, const ImVec2& pos)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImVec2 base = window->Pos;
    const ImVec2 min = ImVec2(std::floor(base.x + pos.x + g_contentOffset.x), std::floor(base.y + pos.y + g_contentOffset.y));
    const ImVec2 size(18.0f, 18.0f);

    ImGui::SetCursorScreenPos(min);

    const bool pressed = ImGui::InvisibleButton(id_text, size);
    static bool colorDragging = false;
    static ImVec2 colorGrabOff{};

    ImDrawList* draw = ImGui::GetWindowDrawList();

    const ImVec2 swatch_center = min + size * 0.5f;
    draw->AddCircleFilled(swatch_center, size.x * 0.5f, ColorU32(*color));
    draw->AddCircle(swatch_center, size.x * 0.5f, ColorU32(GetTheme().TextBright, 0.25f), 0, 1.0f);
    if (pressed)
        ImGui::OpenPopup(id_text);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    const float picker_w = 200.0f;
    ImGui::SetNextWindowSize(ImVec2(picker_w + 20.0f, 0.0f), ImGuiCond_Always);
    if (ImGui::BeginPopup(id_text))
    {
        ImVec2 popPos = ImGui::GetWindowPos();
        ImVec2 popSize = ImGui::GetWindowSize();
        ImGui::SetCursorScreenPos(popPos + ImVec2(10.0f, 0.0f));
        ImGui::PushID("color_drag");
        ImGui::InvisibleButton("##color_drag", ImVec2(picker_w, 10.0f));
        ImGuiID dragId = ImGui::GetItemID();
        float dragHov = AnimateFloat(dragId, ImGui::IsItemHovered(), 18.0f);
        ImDrawList* dragDraw = ImGui::GetWindowDrawList();
        const float grabX = popPos.x + popSize.x * 0.5f;
        dragDraw->AddRectFilled(ImVec2(grabX - 16.0f, popPos.y + 3.0f), ImVec2(grabX + 16.0f, popPos.y + 7.0f), ColorU32(LerpColor(GetTheme().ControlInactive, GetTheme().Text, dragHov)), 2.0f);
        if (ImGui::IsItemActivated()) {
            colorGrabOff = ImGui::GetIO().MousePos - popPos;
            colorDragging = true;
        }
        if (colorDragging) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                ImGui::SetWindowPos(ImGui::GetIO().MousePos - colorGrabOff, ImGuiCond_Always);
            else
                colorDragging = false;
        }
        ImGui::PopID();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, GetTheme().CardBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, GetTheme().ControlInactive);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, GetTheme().ControlInactive);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, GetTheme().Accent);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, GetTheme().Accent);
        ImGui::PushStyleColor(ImGuiCol_Button, GetTheme().ControlBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetTheme().ControlInactive);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetTheme().ControlInactive);
        ImGui::PushStyleColor(ImGuiCol_Header, GetTheme().CardBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, GetTheme().ControlInactive);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, GetTheme().ControlInactive);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(8.0f, 8.0f));
        ImGui::SetNextItemWidth(picker_w);
        ImGui::ColorPicker4("##picker", (float*)color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_PickerHueBar);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(11);
        const Theme& ptheme = GetTheme();
        ImFont* pfont = GetFonts().CascadiaMonoBL ? GetFonts().CascadiaMonoBL : ImGui::GetFont();
        ImGuiWindow* pwin = ImGui::GetCurrentWindow();
        ImVec2 prel = ImGui::GetCursorScreenPos() - pwin->Pos + ImVec2(0.0f, SliderTop() + 4.0f);
        SliderFloat("hex_opacity", &color->w, 0.0f, 1.0f, prel, picker_w, "Opacity", "%.2f");
        if (color->w < 0.0f) color->w = 0.0f;
        if (color->w > 1.0f) color->w = 1.0f;
        char hex[16];
        ImFormatString(hex, IM_ARRAYSIZE(hex), "#%02X%02X%02X%02X",
            (int)(ImClamp(color->x, 0.0f, 1.0f) * 255.0f),
            (int)(ImClamp(color->y, 0.0f, 1.0f) * 255.0f),
            (int)(ImClamp(color->z, 0.0f, 1.0f) * 255.0f),
            (int)(ImClamp(color->w, 0.0f, 1.0f) * 255.0f));
        ImVec2 hexRel = prel + ImVec2(0.0f, 18.0f);
        ImVec2 hexMin = pwin->Pos + hexRel;
        ImVec2 hexTs = pfont->CalcTextSizeA(FontSmall(), FLT_MAX, 0.0f, hex);
        ImGui::SetCursorScreenPos(hexMin);
        ImGui::PushID("hex_ctx_btn");
        ImGui::InvisibleButton("##hex", ImVec2(hexTs.x + 6.0f, 20.0f));
        ImGuiID hexId = ImGui::GetItemID();
        float hexHov = AnimateFloat(hexId, ImGui::IsItemHovered(), 18.0f);
        ImGui::GetWindowDrawList()->AddText(pfont, FontSmall(), hexMin, ColorU32(LerpColor(ptheme.Text, ptheme.TextBright, hexHov)), hex);
        static ImGuiID hexCtxOpen = 0;
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            hexCtxOpen = (hexCtxOpen == hexId) ? 0 : hexId;
        bool hexCtx = (hexCtxOpen == hexId);
        float hexCtxAnim = AnimateFloat(hexId + 40, hexCtx, 18.0f);
        const float hexRowH = 22.0f;
        const float hexPad = 3.0f;
        const float hexPopW = 130.0f;
        const float hexFullH = hexPad * 2.0f + hexRowH * 2.0f;
        ImVec2 hexPopMin(hexMin.x, hexMin.y + 22.0f);
        if (hexCtx && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImRect(hexPopMin, ImVec2(hexPopMin.x + hexPopW, hexPopMin.y + hexFullH)).Contains(ImGui::GetIO().MousePos))
            hexCtxOpen = 0;
        if (hexCtxAnim > 0.01f) {
            ImDrawList* hexFg = ImGui::GetForegroundDrawList();
            hexFg->PushClipRect(hexPopMin, ImVec2(hexPopMin.x + hexPopW, hexPopMin.y + hexFullH * hexCtxAnim), true);
            ImVec2 hexBoxMax = ImVec2(hexPopMin.x + hexPopW, hexPopMin.y + hexFullH);
            hexFg->AddRectFilled(hexPopMin, hexBoxMax, ColorU32(ptheme.ControlBg, hexCtxAnim), kPopupRounding);
            hexFg->AddRect(hexPopMin, hexBoxMax, Hairline(hexCtxAnim), kPopupRounding, 0, 1.0f);
            const char* hexOpts[2] = {"Copy Hex", "Paste Hex"};
            for (int i = 0; i < 2; ++i) {
                ImVec2 iMin(hexPopMin.x + 3.0f, hexPopMin.y + hexPad + hexRowH * i);
                ImVec2 iMax(hexPopMin.x + hexPopW - 3.0f, iMin.y + hexRowH);
                bool hov = ImRect(iMin, iMax).Contains(ImGui::GetIO().MousePos);
                ImGui::PushID(100 + i);
                ImGuiID iid = ImGui::GetID("hex_opt");
                float ih = AnimateFloat(iid, hov, 18.0f);
                ImVec4 parsed{};
                bool valid = (i == 1) ? ParseHexColor(ImGui::GetClipboardText(), parsed) : true;
                if (ih > 0.01f)
                    hexFg->AddRectFilled(iMin, iMax, ColorU32(LerpColor(ptheme.ControlBg, ptheme.ControlInactive, ih * 0.8f), hexCtxAnim), kRowRounding);
                hexFg->AddText(pfont, FontSmall(), ImVec2(iMin.x + 6.0f, iMin.y + 3.0f), ColorU32(valid ? LerpColor(ptheme.Text, ptheme.TextBright, ih * 0.35f) : ImVec4(0.45f, 0.45f, 0.45f, 1.0f), hexCtxAnim), hexOpts[i]);
                if (hexCtx && hexCtxAnim > 0.70f && hov && valid && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    if (i == 0)
                        ImGui::SetClipboardText(hex);
                    else
                        *color = parsed;
                    hexCtxOpen = 0;
                }
                ImGui::PopID();
            }
            hexFg->PopClipRect();
        }
        ImGui::PopID();
        popPos = ImGui::GetWindowPos();
        popSize = ImGui::GetWindowSize();
        ImGui::EndPopup();
        ImDrawList* popFg = ImGui::GetForegroundDrawList();
        popFg->AddRect(popPos, popPos + popSize, Hairline(), kCardRounding, 0, 1.0f);
    } else {
        colorDragging = false;
    }
    ImGui::PopStyleVar();

    return pressed;
}

inline const char* KeyName(int key)
{
    switch (key)
    {
    case 0x01: return "Left Mouse";
    case 0x02: return "Right Mouse";
    case 0x04: return "Middle Mouse";
    case 0x10: return "Shift";
    case 0x11: return "Ctrl";
    case 0x12: return "Alt";
    case 0x20: return "Space";
    default: break;
    }
    if (key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END)
        return ImGui::GetKeyName((ImGuiKey)key);

    static char name[16];

    if (key >= 'A' && key <= 'Z')
        ImFormatString(name, IM_ARRAYSIZE(name), "%c", key);
    else
        ImFormatString(name, IM_ARRAYSIZE(name), "Key %d", key);

    return name;
}

inline bool Keybind(const char* label, int* key, const ImVec2& pos, const ImVec2& size = ImVec2(84.0f, 22.0f), int* mode = nullptr)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 base = window->Pos;
    const ImVec2 min = ImVec2(std::floor(base.x + pos.x + g_contentOffset.x), std::floor(base.y + pos.y + g_contentOffset.y));
    ImGui::SetCursorScreenPos(min);
    const bool pressed = ImGui::InvisibleButton(label, size);
    const ImGuiID id = ImGui::GetItemID();
    const bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

    static std::unordered_map<ImGuiID, int> s_keybind_modes;
    int& current_mode = mode ? *mode : s_keybind_modes[id];

    static ImGuiID context_open_id = 0;
    if (right_clicked)
    {
        context_open_id = (context_open_id == id) ? 0 : id;
    }

    static ImGuiID waiting_id = 0;
    static bool wait_mouse_release = false;
    if (pressed && !PopupBlocking())
    {
        context_open_id = 0;
        waiting_id = id;
        wait_mouse_release = true;
        ImGui::SetActiveID(id, window);
    }

    const bool active = waiting_id == id;
    if (active)
    {
        ImGuiIO& io = ImGui::GetIO();
        bool any_mouse_down = false;
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); ++i)
            any_mouse_down |= io.MouseDown[i];
        if (!any_mouse_down)
            wait_mouse_release = false;

        if ((GetAsyncKeyState(VK_ESCAPE) & 1) != 0)
        {
            *key = 0;
            waiting_id = 0;
            ImGui::ClearActiveID();
        }
        else
        {
            for (int vk = 0x08; vk < 0xFE; ++vk)
            {
                if (vk == VK_ESCAPE) continue;
                if (!wait_mouse_release && (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2)) {
                    if ((GetAsyncKeyState(vk) & 1) != 0) {
                        *key = vk;
                        waiting_id = 0;
                        ImGui::ClearActiveID();
                        break;
                    }
                    continue;
                }
                if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2) continue;
                if ((GetAsyncKeyState(vk) & 1) != 0)
                {
                    *key = vk;
                    waiting_id = 0;
                    ImGui::ClearActiveID();
                    break;
                }
            }
        }
    }

    const float active_anim = AnimateFloat(id, active || ImGui::IsItemHovered(), 16.0f);
    const Theme& theme = GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float waiting_anim = AnimateFloat(id + 5, active, 16.0f);
    draw->AddRectFilled(min, min + size, ColorU32(LerpColor(theme.KeybindBg, theme.ControlInactive, active_anim * 0.5f)), kRowRounding);
    if (waiting_anim > 0.01f)
        draw->AddRect(min, min + size, ColorU32(theme.Accent, waiting_anim), kRowRounding, 0, 1.0f);

    const Fonts& fonts = GetFonts();
    ImFont* font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
    const float font_size = FontSmall();
    const char* text = active ? "..." : (current_mode == 2 && *key == 0 ? "Always" : KeyName(*key));
    const ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);
    draw->AddText(font, font_size, ImVec2(std::floor(min.x + (size.x - text_size.x) * 0.5f), std::floor(min.y + (size.y - text_size.y) * 0.5f)), ColorU32(LerpColor(theme.Text, theme.TextBright, active_anim)), text);

    const bool context_open = (context_open_id == id);
    const float context_anim = AnimateFloat(id + 20, context_open, 18.0f);
    const float popup_w = ImMax(size.x, 84.0f);
    const float row_height = 22.0f;
    const float popup_padding = 3.0f;
    const float full_height = popup_padding * 2.0f + row_height * 3.0f;
    const float visible_height = full_height * context_anim;

    const ImVec2 popup_min(min.x, min.y + size.y + 4.0f);
    const ImVec2 popup_max(popup_min.x + popup_w, popup_min.y + visible_height);
    const ImRect total_rect(min, ImVec2(min.x + popup_w, popup_min.y + full_height));

    if (context_open && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !total_rect.Contains(ImGui::GetIO().MousePos))
    {
        context_open_id = 0;
    }

    if (context_anim > 0.01f)
    {
        ImDrawList* overlay = ImGui::GetForegroundDrawList();
        DrawPopupFrame(overlay, popup_min, popup_max, context_anim);
        overlay->PushClipRect(popup_min, popup_max, true);

        struct ModeDef { const char* label; int value; };
        static const ModeDef mode_list[3] = {
            { "Toggle", 1 },
            { "Hold",   0 },
            { "Always", 2 }
        };

        for (int i = 0; i < 3; ++i)
        {
            const ImVec2 item_min(popup_min.x + 3.0f, popup_min.y + popup_padding + row_height * i);
            const ImVec2 item_max(popup_min.x + popup_w - 3.0f, item_min.y + row_height);
            const ImRect item_rect(item_min, item_max);
            const bool item_hovered = item_rect.Contains(ImGui::GetIO().MousePos);
            const bool item_pressed = context_open && context_anim > 0.70f && item_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            ImGui::PushID(i);
            const ImGuiID item_id = ImGui::GetID("kb_mode");
            const float item_hover = AnimateFloat(item_id, item_hovered, 18.0f);
            const bool is_current = (current_mode == mode_list[i].value);
            const float item_selected = AnimateFloat(item_id + 1, is_current, 18.0f);
            const float item_appear = ImClamp((context_anim - i * 0.08f) / 0.45f, 0.0f, 1.0f);

            const ImVec4 row_color = LerpColor(theme.ControlBg, theme.ControlInactive, item_hover * 0.8f + item_selected * 0.4f);
            if (item_hover > 0.01f || item_selected > 0.01f)
                overlay->AddRectFilled(item_min, item_max, ColorU32(row_color, context_anim), kRowRounding);

            const ImVec4 base_text_color = is_current ? theme.Accent : theme.Text;
            const ImVec4 text_color = LerpColor(base_text_color, theme.TextBright, item_hover * 0.35f);
            const ImVec2 text_sz = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, mode_list[i].label);
            const ImVec2 text_pos(item_min.x + 6.0f, std::floor(item_min.y + (row_height - text_sz.y) * 0.5f));
            overlay->AddText(font, font_size, text_pos, ColorU32(text_color, item_appear), mode_list[i].label);

            if (item_pressed)
            {
                current_mode = mode_list[i].value;
                if (mode) *mode = mode_list[i].value;
                context_open_id = 0;
                ComboClosedFrame() = ImGui::GetFrameCount();
            }
            ImGui::PopID();
        }

        overlay->PopClipRect();
    }
    if (context_open_id == id)
        ComboOpenId() = id;
    else if (ComboOpenId() == id)
        ComboOpenId() = 0;
    return pressed;
}

inline bool SliderFloat(const char* label, float* value, float min_value, float max_value, const ImVec2& pos, float width, const char* text_label, const char* format = "%.0f")
{
    const float font_size = FontSmall();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 base = window->Pos;
    const ImVec2 origin = ImVec2(std::floor(base.x + pos.x + g_contentOffset.x), std::floor(base.y + pos.y + g_contentOffset.y));
    const float height = 12.0f;

    const float step_w = 11.0f;
    const float step_gap = 4.0f;
    const ImVec2 minus_min(origin.x, origin.y);
    const ImVec2 minus_max(origin.x + step_w, origin.y + height);
    const ImVec2 track_min(origin.x + step_w + step_gap, origin.y);
    const float track_w = width - (step_w + step_gap) * 2.0f;
    const ImVec2 track_max(track_min.x + track_w, origin.y + height);
    const ImVec2 plus_min(track_max.x + step_gap, origin.y);
    const ImVec2 plus_max(plus_min.x + step_w, origin.y + height);

    ImGui::PushID(label);
    ImGui::SetCursorScreenPos(origin);
    const bool pressed = ImGui::InvisibleButton("##slider_total", ImVec2(width, height + 4.0f));
    const ImGuiID id = ImGui::GetItemID();

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool minus_hovered = (mouse.x >= minus_min.x - 2.0f && mouse.x <= minus_max.x + 2.0f &&
                                mouse.y >= origin.y - 4.0f && mouse.y <= origin.y + height + 4.0f);
    const bool plus_hovered = (mouse.x >= plus_min.x - 2.0f && mouse.x <= plus_max.x + 2.0f &&
                               mouse.y >= origin.y - 4.0f && mouse.y <= origin.y + height + 4.0f);
    const bool track_hovered = (mouse.x >= track_min.x && mouse.x <= track_max.x &&
                                mouse.y >= origin.y - 3.0f && mouse.y <= origin.y + height + 3.0f);

    const bool active = ImGui::IsItemActive();
    bool changed = false;

    const float step = (max_value - min_value <= 5.0f) ? 0.1f : 1.0f;
    if (!PopupBlocking() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (minus_hovered)
        {
            *value = ImClamp(*value - step, min_value, max_value);
            changed = true;
        }
        else if (plus_hovered)
        {
            *value = ImClamp(*value + step, min_value, max_value);
            changed = true;
        }
    }
    const float down_dur = ImGui::GetIO().MouseDownDuration[0];
    if (down_dur > 0.35f && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        static float last_time = 0.0f;
        const float cur_time = (float)ImGui::GetTime();
        if (cur_time - last_time > 0.08f)
        {
            if (minus_hovered)
            {
                *value = ImClamp(*value - step, min_value, max_value);
                changed = true;
                last_time = cur_time;
            }
            else if (plus_hovered)
            {
                *value = ImClamp(*value + step, min_value, max_value);
                changed = true;
                last_time = cur_time;
            }
        }
    }

    if (active && !minus_hovered && !plus_hovered && !PopupBlocking())
    {
        float t = (mouse.x - track_min.x) / track_w;
        t = ImClamp(t, 0.0f, 1.0f);
        const float new_value = min_value + (max_value - min_value) * t;
        if (*value != new_value)
        {
            *value = new_value;
            changed = true;
        }
    }

    const float target_t = ImClamp((*value - min_value) / (max_value - min_value), 0.0f, 1.0f);
    static std::unordered_map<ImGuiID, float> slider_values;
    float& animated_t = slider_values[id];
    animated_t = ImLerp(animated_t, target_t, ImClamp(AnimDelta() * 16.0f, 0.0f, 1.0f));
    const float hover = AnimateFloat(id + 1, track_hovered || active, 16.0f);
    const float minus_anim = AnimateFloat(id + 2, minus_hovered, 16.0f);
    const float plus_anim = AnimateFloat(id + 3, plus_hovered, 16.0f);

    const Theme& theme = GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const float center_y = origin.y + height * 0.5f;
    const float knob_r = 6.0f + hover * 0.6f;
    const float knob_x = ImLerp(track_min.x + 6.0f, track_max.x - 6.0f, animated_t);
    draw->AddRectFilled(ImVec2(track_min.x, center_y - 2.0f), ImVec2(track_max.x, center_y + 2.0f), ColorU32(theme.ControlInactive), 2.0f);
    draw->AddRectFilled(ImVec2(track_min.x, center_y - 2.0f), ImVec2(knob_x, center_y + 2.0f), ColorU32(theme.Accent), 2.0f);
    draw->AddCircleFilled(ImVec2(knob_x, center_y + 1.0f), knob_r + 0.5f, ColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.30f)));
    draw->AddCircleFilled(ImVec2(knob_x, center_y), knob_r, ColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

    const Fonts& fonts = GetFonts();
    ImFont* font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();

    const ImU32 minus_col = ColorU32(LerpColor(theme.Text, theme.TextBright, minus_anim), 0.6f + minus_anim * 0.4f);
    const ImU32 plus_col = ColorU32(LerpColor(theme.Text, theme.TextBright, plus_anim), 0.6f + plus_anim * 0.4f);
    const float minus_cx = std::floor(minus_min.x + step_w * 0.5f);
    const float plus_cx = std::floor(plus_min.x + step_w * 0.5f);
    const float glyph_y = std::floor(center_y) + 0.5f;
    draw->AddLine(ImVec2(minus_cx - 3.0f, glyph_y), ImVec2(minus_cx + 4.0f, glyph_y), minus_col, 1.5f);
    draw->AddLine(ImVec2(plus_cx - 3.0f, glyph_y), ImVec2(plus_cx + 4.0f, glyph_y), plus_col, 1.5f);
    draw->AddLine(ImVec2(plus_cx + 0.5f, glyph_y - 3.5f), ImVec2(plus_cx + 0.5f, glyph_y + 3.5f), plus_col, 1.5f);

    char value_text[64];
    ImFormatString(value_text, IM_ARRAYSIZE(value_text), format, *value);
    const ImVec2 value_ts = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, value_text);
    const float label_y = origin.y - font_size - 5.0f;
    draw->AddText(font, font_size, ImVec2(origin.x + 1.0f, label_y), ColorU32(theme.Text), text_label);
    draw->AddText(font, font_size, ImVec2(std::floor(origin.x + width - value_ts.x - 1.0f), label_y), ColorU32(theme.TextBright, 0.6f + hover * 0.4f), value_text);

    ImGui::PopID();
    return pressed || changed;
}

inline void DrawComboField(ImDrawList* draw_list, const ImVec2& min, const ImVec2& size, float hover, float open_anim)
{
    const Theme& theme = GetTheme();
    draw_list->AddRectFilled(min, min + size, ColorU32(LerpColor(theme.ControlBg, theme.ControlInactive, hover * 0.5f)), kControlRounding);
    const ImVec2 chevron(min.x + size.x - 11.0f, min.y + size.y * 0.5f);
    const float dir = 1.0f - 2.0f * open_anim;
    const ImVec2 pts[3] = {
        ImVec2(chevron.x - 3.5f, chevron.y - 1.75f * dir),
        ImVec2(chevron.x, chevron.y + 1.75f * dir),
        ImVec2(chevron.x + 3.5f, chevron.y - 1.75f * dir)
    };
    draw_list->AddPolyline(pts, 3, ColorU32(LerpColor(theme.Text, theme.Accent, open_anim)), 0, 1.5f);
}

struct ComboTextAnimation
{
    std::string Previous;
    std::string Current;
    float Blend = 1.0f;
};

inline bool Combo(const char* label, int* current_item, const char* const items[], int items_count, const ImVec2& pos, float width, const char* text_label)
{
    const float font_size = FontSmall();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 base = window->Pos;
    const ImVec2 min = ImVec2(std::floor(base.x + pos.x + g_contentOffset.x), std::floor(base.y + pos.y + g_contentOffset.y));
    const ImVec2 size(width, 24.0f);
    ImGui::PushID(label);
    ImGui::SetCursorScreenPos(min);
    const bool pressed = ImGui::InvisibleButton("##combo_preview", size);

    const bool hovered = ImGui::IsItemHovered();
    const ImGuiID id = ImGui::GetItemID();
    static ImGuiID open_id = 0;
    if (pressed)
        open_id = open_id == id ? 0 : id;

    const bool open = open_id == id;
    const float hover = AnimateFloat(id, hovered, 18.0f);
    const float open_anim = AnimateFloat(id + 10, open, 18.0f);
    const Theme& theme = GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawComboField(draw, min, size, hover, open_anim);

    const Fonts& fonts = GetFonts();
    ImFont* font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
    if (text_label)
        draw->AddText(font, font_size, ImVec2(min.x + 1.0f, min.y - font_size - 3.0f), ColorU32(theme.Text), text_label);
    const char* preview = (*current_item >= 0 && *current_item < items_count) ? items[*current_item] : "";
    static std::unordered_map<ImGuiID, ComboTextAnimation> text_animations;
    ComboTextAnimation& text_anim = text_animations[id];
    if (text_anim.Current.empty())
        text_anim.Current = preview;
    if (text_anim.Current != preview)
    {
        text_anim.Previous = text_anim.Current;
        text_anim.Current = preview;
        text_anim.Blend = 0.0f;
    }
    text_anim.Blend = ImLerp(text_anim.Blend, 1.0f, ImClamp(AnimDelta() * 14.0f, 0.0f, 1.0f));
    ImVec2 preview_sz = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text_anim.Current.c_str());
    const ImVec2 preview_pos(min.x + 7.0f, std::floor(min.y + (size.y - preview_sz.y) * 0.5f));
    if (!text_anim.Previous.empty() && text_anim.Blend < 0.98f)
        draw->AddText(font, font_size, preview_pos, ColorU32(theme.Text, 1.0f - text_anim.Blend), text_anim.Previous.c_str());
    draw->AddText(font, font_size, preview_pos, ColorU32(theme.Text, text_anim.Blend), text_anim.Current.c_str());

    bool changed = false;
    const float row_height = 22.0f;
    const float popup_padding = 3.0f;
    const float full_height = popup_padding * 2.0f + row_height * items_count;
    const float visible_height = full_height * open_anim;
    const ImVec2 popup_min(min.x, min.y + size.y + 4.0f);
    const ImVec2 popup_max(popup_min.x + width, popup_min.y + visible_height);
    const ImRect total_rect(min, ImVec2(min.x + width, popup_min.y + full_height));

    if (open && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !total_rect.Contains(ImGui::GetIO().MousePos)) {
        open_id = 0;
        ComboClosedFrame() = ImGui::GetFrameCount();
    }

    if (open_anim > 0.01f)
    {
        ImDrawList* overlay = ImGui::GetForegroundDrawList();
        DrawPopupFrame(overlay, popup_min, popup_max, open_anim);
        overlay->PushClipRect(popup_min, popup_max, true);

        for (int i = 0; i < items_count; ++i)
        {
            const ImVec2 item_min(popup_min.x + 3.0f, popup_min.y + popup_padding + row_height * i);
            const ImVec2 item_max(popup_min.x + width - 3.0f, item_min.y + row_height);
            const ImRect item_rect(item_min, item_max);
            const bool item_hovered = item_rect.Contains(ImGui::GetIO().MousePos);
            const bool item_pressed = open && open_anim > 0.70f && item_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            ImGui::PushID(i);
            const ImGuiID item_id = ImGui::GetID("combo_item");
            const float item_hover = AnimateFloat(item_id, item_hovered, 18.0f);
            const float item_selected = AnimateFloat(item_id + 1, i == *current_item, 18.0f);
            const float item_appear = ImClamp((open_anim - i * 0.08f) / 0.45f, 0.0f, 1.0f);
            if (item_hover > 0.01f)
                overlay->AddRectFilled(item_min, item_max, ColorU32(LerpColor(theme.ControlBg, theme.ControlInactive, item_hover * 0.8f), open_anim), kRowRounding);
            const ImVec4 text_color = LerpColor(theme.Text, theme.TextBright, item_hover * 0.35f + item_selected * 0.55f);
            overlay->AddText(font, font_size, ImVec2(item_min.x + 6.0f, std::floor(item_min.y + (row_height - font_size) * 0.5f)), ColorU32(text_color, item_appear), items[i]);
            if (item_selected > 0.01f)
                DrawCheckmark(overlay, ImVec2(item_max.x - 10.0f, item_min.y + row_height * 0.5f), 3.5f, ColorU32(theme.Accent, item_selected * item_appear));
            if (item_pressed)
            {
                *current_item = i;
                changed = true;
                open_id = 0;
            }
            ImGui::PopID();
        }
        overlay->PopClipRect();
    }
    if (open_id == id)
        ComboOpenId() = id;
    else if (ComboOpenId() == id) {
        ComboOpenId() = 0;
        ComboClosedFrame() = ImGui::GetFrameCount();
    }
    ImGui::PopID();
    return changed;
}

struct MultiComboTextAnimation
{
    std::string Previous;
    std::string Current;
    float Blend = 1.0f;
};

inline bool MultiCombo(const char* label, bool values[], const char* const items[], int items_count, const ImVec2& pos, float width, const char* text_label)
{
    const float font_size = FontSmall();
    std::string preview;
    int selected_count = 0;
    for (int i = 0; i < items_count; ++i)
    {
        if (!values[i])
            continue;
        ++selected_count;
        if (!preview.empty())
            preview += ", ";
        preview += items[i];
    }
    if (selected_count > 3)
        preview = std::to_string(selected_count) + " Selected";
    if (preview.empty())
        preview = "Select...";

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 base = window->Pos;
    const ImVec2 min = ImVec2(std::floor(base.x + pos.x + g_contentOffset.x), std::floor(base.y + pos.y + g_contentOffset.y));
    const ImVec2 size(width, 24.0f);
    const ImVec2 hit_size(width, 24.0f);
    ImGui::PushID(label);
    ImGui::SetCursorScreenPos(min - ImVec2(0.0f, 0.0f));
    const bool pressed = ImGui::InvisibleButton("##multicombo_preview", hit_size);

    const bool hovered = ImGui::IsItemHovered();
    const ImGuiID id = ImGui::GetItemID();
    static ImGuiID open_id = 0;
    if (pressed)
        open_id = open_id == id ? 0 : id;

    const bool open = open_id == id;
    const float hover = AnimateFloat(id, hovered, 16.0f);
    const float open_anim = AnimateFloat(id + 10, open, 18.0f);
    const Theme& theme = GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawComboField(draw, min, size, hover, open_anim);

    const Fonts& fonts = GetFonts();
    ImFont* font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
    if (text_label)
        draw->AddText(font, font_size, ImVec2(min.x + 1.0f, min.y - font_size - 3.0f), ColorU32(theme.Text), text_label);
    static std::unordered_map<ImGuiID, MultiComboTextAnimation> text_animations;
    MultiComboTextAnimation& text_anim = text_animations[id];
    if (text_anim.Current.empty())
        text_anim.Current = preview;
    if (text_anim.Current != preview)
    {
        text_anim.Previous = text_anim.Current;
        text_anim.Current = preview;
        text_anim.Blend = 0.0f;
    }
    text_anim.Blend = ImLerp(text_anim.Blend, 1.0f, ImClamp(AnimDelta() * 14.0f, 0.0f, 1.0f));
    ImVec2 preview_sz = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text_anim.Current.c_str());
    const ImVec2 preview_pos(min.x + 7.0f, std::floor(min.y + (size.y - preview_sz.y) * 0.5f));
    draw->PushClipRect(min, ImVec2(min.x + size.x - 20.0f, min.y + size.y), true);
    if (!text_anim.Previous.empty() && text_anim.Blend < 0.98f)
        draw->AddText(font, font_size, preview_pos, ColorU32(theme.Text, 1.0f - text_anim.Blend), text_anim.Previous.c_str());
    draw->AddText(font, font_size, preview_pos, ColorU32(theme.Text, text_anim.Blend), text_anim.Current.c_str());
    draw->PopClipRect();

    bool changed = false;
    const float row_height = 22.0f;
    const float popup_padding = 3.0f;
    const float full_height = popup_padding * 2.0f + row_height * items_count;
    const float visible_height = full_height * open_anim;
    const ImVec2 popup_min(min.x, min.y + size.y + 4.0f);
    const ImVec2 popup_max(popup_min.x + width, popup_min.y + visible_height);
    const ImRect total_rect(min, ImVec2(min.x + width, popup_min.y + full_height));

    if (open && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !total_rect.Contains(ImGui::GetIO().MousePos))
        open_id = 0;

    if (open_anim > 0.01f)
    {
        ImDrawList* overlay = ImGui::GetForegroundDrawList();
        DrawPopupFrame(overlay, popup_min, popup_max, open_anim);
        overlay->PushClipRect(popup_min, popup_max, true);

        for (int i = 0; i < items_count; ++i)
        {
            const ImVec2 item_min(popup_min.x + 3.0f, popup_min.y + popup_padding + row_height * i);
            const ImVec2 item_max(popup_min.x + width - 3.0f, item_min.y + row_height);
            const ImRect item_rect(item_min, item_max);
            const bool item_hovered = item_rect.Contains(ImGui::GetIO().MousePos);
            const bool item_pressed = open && open_anim > 0.70f && item_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            ImGui::PushID(i);
            const ImGuiID item_id = ImGui::GetID("multicombo_item");
            const float item_hover = AnimateFloat(item_id, item_hovered, 18.0f);
            const float item_selected = AnimateFloat(item_id + 1, values[i], 18.0f);
            const float item_appear = ImClamp((open_anim - i * 0.08f) / 0.45f, 0.0f, 1.0f);
            if (item_hover > 0.01f)
                overlay->AddRectFilled(item_min, item_max, ColorU32(LerpColor(theme.ControlBg, theme.ControlInactive, item_hover * 0.8f), open_anim), kRowRounding);
            const ImVec4 text_color = LerpColor(theme.Text, theme.TextBright, item_hover * 0.35f + item_selected * 0.55f);
            overlay->AddText(font, font_size, ImVec2(item_min.x + 6.0f, std::floor(item_min.y + (row_height - font_size) * 0.5f)), ColorU32(text_color, item_appear), items[i]);
            if (item_selected > 0.01f)
                DrawCheckmark(overlay, ImVec2(item_max.x - 10.0f, item_min.y + row_height * 0.5f), 3.5f, ColorU32(theme.Accent, item_selected * item_appear));
            if (item_pressed)
            {
                values[i] = !values[i];
                changed = true;
            }
            ImGui::PopID();
        }
        overlay->PopClipRect();
    }
    ImGui::PopID();
    return changed;
}
}

namespace AllSetting
{
namespace Layout
{
constexpr float kWindowW = 720.0f;
constexpr float kWindowH = 440.0f;
constexpr float kSidebarW = 150.0f;
constexpr float kPanelX[2] = {158.0f, 439.0f};
constexpr float kPanelY = 52.0f;
constexpr float kPanelW = 273.0f;
constexpr float kPanelH = 380.0f;
constexpr float kPanelPad = 14.0f;
constexpr float kRowH = 32.0f;
}

struct State
{
    float Left = 0.0f;
    float Right = 0.0f;
    float Y = 0.0f;
    int Rows = 0;
};

inline State& GetState()
{
    static State g_State;
    return g_State;
}

inline ImFont* Font()
{
    const imGuiCustom::Fonts& fonts = imGuiCustom::GetFonts();
    return fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
}

inline void BeginPanelAt(const char* title, float x, float y, float width)
{
    State& s = GetState();
    s.Left = x + Layout::kPanelPad;
    s.Right = x + width - Layout::kPanelPad;
    s.Y = y + 6.0f;
    s.Rows = 0;
    if (title)
    {
        const ImVec2 base = ImGui::GetWindowPos();
        const float font_size = imGuiCustom::FontSmall();
        ImGui::GetWindowDrawList()->AddText(Font(), font_size, ImVec2(std::floor(base.x + s.Left), std::floor(base.y + s.Y + 6.0f)), imGuiCustom::ColorU32(imGuiCustom::GetTheme().Text, 0.55f), title);
        s.Y += font_size + 14.0f;
    }
}

inline void BeginPanel(const char* title, int column)
{
    BeginPanelAt(title, Layout::kPanelX[column], Layout::kPanelY, Layout::kPanelW);
}

inline float EndPanel()
{
    return GetState().Y;
}

inline float BeginRow(const char* label, float height = Layout::kRowH)
{
    State& s = GetState();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 base = ImGui::GetWindowPos();
    if (s.Rows > 0)
    {
        const float line_y = std::floor(base.y + s.Y) + 0.5f;
        draw->AddLine(ImVec2(base.x + s.Left, line_y), ImVec2(base.x + s.Right, line_y), imGuiCustom::Hairline(), 1.0f);
    }
    if (label)
    {
        const float font_size = imGuiCustom::FontLabel();
        const char* label_end = ImGui::FindRenderedTextEnd(label);
        const ImVec2 text_size = Font()->CalcTextSizeA(font_size, FLT_MAX, 0.0f, label, label_end);
        draw->AddText(Font(), font_size, ImVec2(std::floor(base.x + s.Left), std::floor(base.y + s.Y + (Layout::kRowH - text_size.y) * 0.5f)), imGuiCustom::ColorU32(imGuiCustom::GetTheme().Text), label, label_end);
    }
    const float top = s.Y;
    s.Y += height;
    ++s.Rows;
    return top;
}

inline void Label(const char* text)
{
    BeginRow(text);
}

inline void Info(const char* label, const char* value)
{
    State& s = GetState();
    const float top = BeginRow(label);
    const ImVec2 base = ImGui::GetWindowPos();
    const float font_size = imGuiCustom::FontLabel();
    const ImVec2 text_size = Font()->CalcTextSizeA(font_size, FLT_MAX, 0.0f, value);
    ImGui::GetWindowDrawList()->AddText(Font(), font_size, ImVec2(std::floor(base.x + s.Right - text_size.x), std::floor(base.y + top + (Layout::kRowH - text_size.y) * 0.5f)), imGuiCustom::ColorU32(imGuiCustom::GetTheme().TextBright, 0.6f), value);
}

inline bool Checkbox(const char* label, bool* value)
{
    State& s = GetState();
    const float top = BeginRow(label);
    const ImVec2 base = ImGui::GetWindowPos();
    const ImVec2 switch_size(30.0f, 18.0f);
    const ImVec2 switch_min(std::floor(base.x + s.Right - switch_size.x), std::floor(base.y + top + (Layout::kRowH - switch_size.y) * 0.5f));
    ImGui::PushID(label);
    ImGui::SetCursorScreenPos(ImVec2(base.x + s.Left, base.y + top));
    const bool pressed = ImGui::InvisibleButton("##row", ImVec2(s.Right - s.Left, Layout::kRowH)) && !imGuiCustom::PopupBlocking();
    if (pressed)
        *value = !*value;
    imGuiCustom::DrawSwitch(ImGui::GetWindowDrawList(), switch_min, switch_size, imGuiCustom::AnimateFloat(ImGui::GetItemID(), *value, 14.0f));
    ImGui::PopID();
    return pressed;
}

inline bool Swatches(const char* label, int* current, const ImVec4 colors[], int count)
{
    State& s = GetState();
    const float top = BeginRow(label);
    const ImVec2 base = ImGui::GetWindowPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float radius = 8.0f;
    const float step = 26.0f;
    bool changed = false;
    ImGui::PushID("##swatches");
    ImGui::PushID(s.Rows);
    for (int i = 0; i < count; ++i)
    {
        const ImVec2 center(std::floor(base.x + s.Right - radius - step * (float)(count - 1 - i)), std::floor(base.y + top + Layout::kRowH * 0.5f));
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(center - ImVec2(step * 0.5f, step * 0.5f));
        if (ImGui::InvisibleButton("##swatch", ImVec2(step, step)) && !imGuiCustom::PopupBlocking() && *current != i)
        {
            *current = i;
            changed = true;
        }
        const ImGuiID id = ImGui::GetItemID();
        const float hover = imGuiCustom::AnimateFloat(id, ImGui::IsItemHovered(), 14.0f);
        const float selected = imGuiCustom::AnimateFloat(id + 1, *current == i, 14.0f);
        draw->AddCircleFilled(center, radius - 1.5f * selected + hover * 0.8f * (1.0f - selected), imGuiCustom::ColorU32(colors[i]));
        if (selected > 0.01f)
            draw->AddCircle(center, radius + 2.0f, imGuiCustom::ColorU32(colors[i], selected), 0, 1.5f);
        ImGui::PopID();
    }
    ImGui::PopID();
    ImGui::PopID();
    return changed;
}

inline bool Slider(const char* label, float* value, float min_value, float max_value, const char* format = "%.0f")
{
    State& s = GetState();
    const float top = BeginRow(nullptr, imGuiCustom::SliderTop() + 30.0f);
    return imGuiCustom::SliderFloat(label, value, min_value, max_value, ImVec2(s.Left, top + 8.0f + imGuiCustom::SliderTop()), s.Right - s.Left, label, format);
}

inline bool SliderInt(const char* label, int* value, int min_value, int max_value)
{
    float as_float = (float)*value;
    const bool changed = Slider(label, &as_float, (float)min_value, (float)max_value, "%.0f");
    *value = (int)std::round(as_float);
    return changed;
}

inline bool Combo(const char* label, int* current_item, const char* const items[], int items_count, float width = 120.0f)
{
    State& s = GetState();
    const float top = BeginRow(label);
    return imGuiCustom::Combo(label, current_item, items, items_count, ImVec2(s.Right - width, top + (Layout::kRowH - 24.0f) * 0.5f), width, nullptr);
}

inline bool MultiCombo(const char* label, bool values[], const char* const items[], int items_count, float width = 120.0f)
{
    State& s = GetState();
    const float top = BeginRow(label);
    return imGuiCustom::MultiCombo(label, values, items, items_count, ImVec2(s.Right - width, top + (Layout::kRowH - 24.0f) * 0.5f), width, nullptr);
}

inline bool Keybind(const char* label, int* key, int* mode = nullptr)
{
    State& s = GetState();
    const float top = BeginRow(label);
    const ImVec2 size(84.0f, 22.0f);
    return imGuiCustom::Keybind(label, key, ImVec2(s.Right - size.x, top + (Layout::kRowH - size.y) * 0.5f), size, mode);
}

inline bool ColorPicker(const char* label, ImVec4* color)
{
    State& s = GetState();
    const float top = BeginRow(label);
    return imGuiCustom::ColorSquare(label, color, ImVec2(s.Right - 18.0f, top + (Layout::kRowH - 18.0f) * 0.5f));
}
}
