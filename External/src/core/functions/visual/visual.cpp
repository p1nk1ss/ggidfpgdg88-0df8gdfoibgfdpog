#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "visual.h"
#include "../../../render/menu/library.h"
#include <cmath>
#include <algorithm>

namespace Visuals {
ImFont* EspFont() {
    const imGuiCustom::Fonts& fonts = imGuiCustom::GetFonts();
    return fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
}

float EspSize() {
    return variables::Misc::espFontSize;
}

ImU32 ToU32(const ImVec4& c) {
    return IM_COL32(static_cast<int>(c.x * 255.0f), static_cast<int>(c.y * 255.0f), static_cast<int>(c.z * 255.0f), static_cast<int>(c.w * 255.0f));
}

void DrawOutlinedText(ImDrawList* dl, const ImVec2& pos, const std::string& text, ImU32 col) {
    ImFont* font = EspFont();
    dl->AddText(font, EspSize(), ImVec2(pos.x - 1, pos.y), IM_COL32(0, 0, 0, 255), text.c_str());
    dl->AddText(font, EspSize(), ImVec2(pos.x + 1, pos.y), IM_COL32(0, 0, 0, 255), text.c_str());
    dl->AddText(font, EspSize(), ImVec2(pos.x, pos.y - 1), IM_COL32(0, 0, 0, 255), text.c_str());
    dl->AddText(font, EspSize(), ImVec2(pos.x, pos.y + 1), IM_COL32(0, 0, 0, 255), text.c_str());
    dl->AddText(font, EspSize(), pos, col, text.c_str());
}

void DrawLine(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col, float t, bool outline) {
    if (outline)
        dl->AddLine(a, b, IM_COL32(0, 0, 0, 255), t + 2.0f);
    dl->AddLine(a, b, col, t);
}

bool ToScreen(const RBX::Vec3& w, const RBX::Mat4& v, ImVec2& out) {
    const auto s = W2S::WorldToScreen(w, v);
    if (s.X == 0 && s.Y == 0)
        return false;
    out = ImVec2(s.X, s.Y);
    return true;
}

void Bone(ImDrawList* dl, const RBX::Vec3& a, const RBX::Vec3& b, const RBX::Mat4& v) {
    ImVec2 sa, sb;
    if (!ToScreen(a, v, sa) || !ToScreen(b, v, sb))
        return;
    DrawLine(dl, sa, sb, ToU32(variables::ESP::skeletonColor), variables::ESP::skeletonThickness, variables::ESP::skeletonOutline);
}

RBX::Vec3 PartPos(std::uintptr_t partAddr) {
    if (!partAddr)
        return {};
    const uintptr_t prim = memory->read<uintptr_t>(partAddr + Offsets::BasePart::Primitive);
    if (!prim)
        return {};
    return memory->read<RBX::Vec3>(prim + Offsets::Primitive::Position);
}

void BoneAddr(ImDrawList* dl, std::uintptr_t a, std::uintptr_t b, const RBX::Mat4& v) {
    if (!a || !b)
        return;
    const RBX::Vec3 pa = PartPos(a);
    const RBX::Vec3 pb = PartPos(b);
    if ((pa.X == 0 && pa.Y == 0 && pa.Z == 0) || (pb.X == 0 && pb.Y == 0 && pb.Z == 0))
        return;
    Bone(dl, pa, pb, v);
}

bool DynamicBounds(const PlayerCache::LimbAddrs& l, bool r6, const RBX::Mat4& v, float& x0, float& x1, float& y0, float& y1) {
    if (!l.head || !l.hrp)
        return false;
    const RBX::Vec3 hp = PartPos(l.head);
    const RBX::Vec3 rp = PartPos(l.hrp);
    if ((hp.X == 0 && hp.Y == 0 && hp.Z == 0) || (rp.X == 0 && rp.Y == 0 && rp.Z == 0))
        return false;
    RBX::Vec3 pts[20];
    int n = 0;
    pts[n++] = {hp.X, hp.Y + 0.6f, hp.Z};
    pts[n++] = hp;
    const std::uintptr_t extra[] = {
        r6 ? l.torso : l.upperTorso, r6 ? l.lArm : l.lUpperArm, r6 ? l.rArm : l.rUpperArm,
        r6 ? l.lLeg : l.lUpperLeg, r6 ? l.rLeg : l.rUpperLeg, r6 ? 0 : l.lowerTorso,
        r6 ? 0 : l.lLowerArm, r6 ? 0 : l.lHand, r6 ? 0 : l.rLowerArm, r6 ? 0 : l.rHand,
        r6 ? 0 : l.lLowerLeg, r6 ? 0 : l.lFoot, r6 ? 0 : l.rLowerLeg, r6 ? 0 : l.rFoot
    };
    for (auto addr : extra) {
        if (!addr || n >= 17)
            continue;
        const RBX::Vec3 p = PartPos(addr);
        if (p.X == 0 && p.Y == 0 && p.Z == 0)
            continue;
        pts[n++] = p;
    }
    pts[n++] = rp;
    pts[n++] = {rp.X, rp.Y - (r6 ? 3.0f : 2.5f), rp.Z};
    bool any = false;
    float mnX = 1e9f, mxX = -1e9f, mnY = 1e9f, mxY = -1e9f;
    for (int i = 0; i < n; ++i) {
        const auto s = W2S::WorldToScreen(pts[i], v);
        if (s.X == 0 && s.Y == 0)
            continue;
        any = true;
        if (s.X < mnX) mnX = s.X;
        if (s.X > mxX) mxX = s.X;
        if (s.Y < mnY) mnY = s.Y;
        if (s.Y > mxY) mxY = s.Y;
    }
    if (!any)
        return false;
    const float pad = std::clamp((mxY - mnY) * 0.06f, 3.0f, 10.0f);
    x0 = mnX - pad * 0.8f;
    x1 = mxX + pad * 0.8f;
    y0 = mnY - pad;
    y1 = mxY + pad;
    return x1 > x0 && y1 > y0;
}

void DrawBoxFill(ImDrawList* dl, float x0, float y0, float x1, float y1) {
    if (!variables::ESP::boxFilled)
        return;
    if (variables::ESP::boxFillGradient)
        dl->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y1), ToU32(variables::ESP::boxFillColor), ToU32(variables::ESP::boxFillColor), ToU32(variables::ESP::boxFillColor2), ToU32(variables::ESP::boxFillColor2));
    else
        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), ToU32(variables::ESP::boxFillColor));
}

void DrawBox(ImDrawList* dl, float x0, float y0, float x1, float y1) {
    const ImU32 col = ToU32(variables::ESP::boxColor);
    if (variables::ESP::boxStyle == 1) {
        const float w = x1 - x0;
        const float h = y1 - y0;
        float lineW = std::clamp(w * 0.25f, 5.0f, w * 0.40f);
        float lineH = std::clamp(h * 0.25f, 5.0f, h * 0.40f);
        auto corner = [&](ImVec2 a, ImVec2 b, ImVec2 c) {
            float dirHx = (b.x > a.x) ? 1.0f : -1.0f;
            float dirVy = (c.y > a.y) ? 1.0f : -1.0f;
            dl->AddLine(ImVec2(a.x - dirHx, a.y), ImVec2(b.x + dirHx, b.y), IM_COL32(0, 0, 0, 255), 3.0f);
            dl->AddLine(ImVec2(a.x, a.y - dirVy), ImVec2(c.x, c.y + dirVy), IM_COL32(0, 0, 0, 255), 3.0f);
            dl->AddLine(a, b, col, 1.0f);
            dl->AddLine(a, c, col, 1.0f);
        };
        corner(ImVec2(x0, y0), ImVec2(x0 + lineW, y0), ImVec2(x0, y0 + lineH));
        corner(ImVec2(x1, y0), ImVec2(x1 - lineW, y0), ImVec2(x1, y0 + lineH));
        corner(ImVec2(x0, y1), ImVec2(x0 + lineW, y1), ImVec2(x0, y1 - lineH));
        corner(ImVec2(x1, y1), ImVec2(x1 - lineW, y1), ImVec2(x1, y1 - lineH));
        return;
    }
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 0, 0, 255), 0.0f, 0, 3.0f);
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), col, 0.0f, 0, 1.0f);
    dl->AddRect(ImVec2(x0 + 1, y0 + 1), ImVec2(x1 - 1, y1 - 1), IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.0f);
}

void RenderSkeleton(ImDrawList* dl, const PlayerCache::LimbAddrs& l, const RBX::Mat4& v) {
    if (l.r6) {
        if (!l.head || !l.torso)
            return;
        const auto hp = PartPos(l.head);
        const auto tp = PartPos(l.torso);
        Bone(dl, hp, tp, v);
        const std::uintptr_t limbs[] = {l.lArm, l.rArm, l.lLeg, l.rLeg};
        for (auto addr : limbs) {
            if (!addr)
                continue;
            Bone(dl, tp, PartPos(addr), v);
        }
        return;
    }
    if (!l.head || !l.upperTorso)
        return;
    const auto hp = PartPos(l.head);
    const auto up = PartPos(l.upperTorso);
    Bone(dl, hp, up, v);
    if (l.lowerTorso) {
        const auto lp = PartPos(l.lowerTorso);
        Bone(dl, up, lp, v);
        const std::uintptr_t leftArm[] = {l.lUpperArm, l.lLowerArm, l.lHand};
        const std::uintptr_t rightArm[] = {l.rUpperArm, l.rLowerArm, l.rHand};
        const std::uintptr_t leftLeg[] = {l.lUpperLeg, l.lLowerLeg, l.lFoot};
        const std::uintptr_t rightLeg[] = {l.rUpperLeg, l.rLowerLeg, l.rFoot};
        auto chain = [&](const std::uintptr_t* c) {
            std::uintptr_t prev = 0;
            RBX::Vec3 prevPos{};
            for (int i = 0; i < 3; ++i) {
                if (!c[i])
                    return;
                const RBX::Vec3 p = PartPos(c[i]);
                if (prev)
                    Bone(dl, prevPos, p, v);
                prev = c[i];
                prevPos = p;
            }
        };
        BoneAddr(dl, l.upperTorso, l.lUpperArm, v);
        BoneAddr(dl, l.upperTorso, l.rUpperArm, v);
        chain(leftArm);
        chain(rightArm);
        BoneAddr(dl, l.lowerTorso, l.lUpperLeg, v);
        BoneAddr(dl, l.lowerTorso, l.rUpperLeg, v);
        chain(leftLeg);
        chain(rightLeg);
    }
}

void RenderESP(ImDrawList* dl, const RBX::Mat4& v) {
    if (!variables::ESP::enabled)
        return;
    const ImVec2 disp = ImGui::GetIO().DisplaySize;
    for (auto& p : PlayerCache::players) {
        if (!p.isValid || !p.headAddr || !p.rootPartAddr)
            continue;
        const PlayerCache::LimbAddrs& limbs = PlayerCache::GetLimbs(p.characterAddr);
        const bool r6 = p.isR6;
        const RBX::Vec3 hp = PartPos(p.headAddr);
        const RBX::Vec3 rp = PartPos(p.rootPartAddr);
        if ((hp.X == 0 && hp.Y == 0 && hp.Z == 0) || (rp.X == 0 && rp.Y == 0 && rp.Z == 0))
            continue;
        const float dist = sqrtf((rp.X - PlayerCache::localPlayerPos.X) * (rp.X - PlayerCache::localPlayerPos.X) + (rp.Y - PlayerCache::localPlayerPos.Y) * (rp.Y - PlayerCache::localPlayerPos.Y) + (rp.Z - PlayerCache::localPlayerPos.Z) * (rp.Z - PlayerCache::localPlayerPos.Z));
        float x0, x1, y0, y1;
        if (variables::ESP::boxMode == 1) {
            if (!DynamicBounds(limbs, r6, v, x0, x1, y0, y1))
                continue;
        } else {
            RBX::Vec3 top3{hp.X, hp.Y + 0.5f, hp.Z};
            RBX::Vec3 bot3{rp.X, rp.Y - (r6 ? 3.0f : 2.5f), rp.Z};
            const auto top = W2S::WorldToScreen(top3, v);
            const auto bot = W2S::WorldToScreen(bot3, v);
            if ((top.X == 0 && top.Y == 0) || (bot.X == 0 && bot.Y == 0))
                continue;
            const float h = bot.Y - top.Y;
            const float w = h * 0.55f;
            x0 = top.X - w * 0.5f;
            x1 = top.X + w * 0.5f;
            y0 = top.Y;
            y1 = bot.Y;
        }
        if (x0 < -500 || y0 < -500 || x1 > disp.x + 500 || y1 > disp.y + 500)
            continue;
        if (variables::ESP::boxes)
            DrawBoxFill(dl, x0, y0, x1, y1);
        if (variables::ESP::skeleton)
            RenderSkeleton(dl, limbs, v);
        if (variables::ESP::boxes)
            DrawBox(dl, x0, y0, x1, y1);
        if (variables::ESP::healthBar && p.maxHealth > 0) {
            const float f = std::clamp(p.health / p.maxHealth, 0.0f, 1.0f);
            const ImVec4 hc = variables::ESP::healthColor;
            const float bx0 = std::floor(x0 - 6.0f);
            const float bx1 = std::floor(x0 - 2.0f);
            const float by0 = std::floor(y0);
            const float by1 = std::floor(y1);
            dl->AddRectFilled(ImVec2(bx0, by0), ImVec2(bx1, by1), IM_COL32(0, 0, 0, 200));
            dl->AddRectFilled(ImVec2(bx0 + 1.0f, by1 - (by1 - by0) * f), ImVec2(bx1, by1),
                              IM_COL32(static_cast<int>(255.0f * (1.0f - f) + hc.x * 255.0f * f), static_cast<int>(hc.y * 255.0f * f), static_cast<int>(hc.z * 255.0f * f), 255));
        }
        if (variables::ESP::names) {
            const ImVec2 ts = EspFont()->CalcTextSizeA(EspSize(), FLT_MAX, 0.0f, p.name.c_str());
            DrawOutlinedText(dl, ImVec2((x0 + x1) * 0.5f - ts.x * 0.5f, y0 - ts.y - 2), p.name, ToU32(variables::ESP::nameColor));
        }
        if (variables::ESP::distance) {
            std::string dt = std::to_string(static_cast<int>(dist)) + "m";
            const ImVec2 ts = EspFont()->CalcTextSizeA(EspSize(), FLT_MAX, 0.0f, dt.c_str());
            DrawOutlinedText(dl, ImVec2((x0 + x1) * 0.5f - ts.x * 0.5f, y1 + 2), dt, ToU32(variables::ESP::distanceColor));
        }
        if (variables::ESP::tool && p.tool != "None" && !p.tool.empty()) {
            const ImVec2 ts = EspFont()->CalcTextSizeA(EspSize(), FLT_MAX, 0.0f, p.tool.c_str());
            float ty = y1 + 2;
            if (variables::ESP::distance)
                ty += ts.y + 3.0f;
            DrawOutlinedText(dl, ImVec2((x0 + x1) * 0.5f - ts.x * 0.5f, ty), p.tool, ToU32(variables::ESP::toolColor));
        }
    }
}
}
