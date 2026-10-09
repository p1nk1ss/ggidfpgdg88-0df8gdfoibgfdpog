#pragma once

#include <Windows.h>
#include "../../../ext/imgui/imgui.h"
#include "../../../ext/imgui/imgui_internal.h"

namespace GdiFont
{
constexpr int kSupersample = 4;

inline float& CoverageGamma() { static float v = 0.7f; return v; }

struct SrcData
{
    HDC Dc = nullptr;
    HFONT ProbeFont = nullptr;
    wchar_t Face[LF_FACESIZE] = {};
    int Weight = FW_NORMAL;
};

struct BakedData
{
    HFONT Font;
};

inline const wchar_t*& PendingFace() { static const wchar_t* v = L"Segoe UI"; return v; }
inline int& PendingWeight() { static int v = FW_NORMAL; return v; }

inline bool FaceExists(const wchar_t* face)
{
    LOGFONTW lf{};
    lf.lfCharSet = DEFAULT_CHARSET;
    wcsncpy_s(lf.lfFaceName, face, _TRUNCATE);
    bool found = false;
    HDC dc = GetDC(nullptr);
    EnumFontFamiliesExW(dc, &lf, [](const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM param) -> int { *(bool*)param = true; return 0; }, (LPARAM)&found, 0);
    ReleaseDC(nullptr, dc);
    return found;
}

inline HFONT CreateFace(const SrcData* data, int pixel_height)
{
    return CreateFontW(pixel_height, 0, 0, 0, data->Weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, data->Face);
}

inline bool FontSrcInit(ImFontAtlas*, ImFontConfig* src)
{
    SrcData* data = IM_NEW(SrcData);
    wcsncpy_s(data->Face, PendingFace(), _TRUNCATE);
    data->Weight = PendingWeight();
    data->Dc = CreateCompatibleDC(nullptr);
    data->ProbeFont = data->Dc ? CreateFace(data, 16) : nullptr;
    if (!data->ProbeFont)
    {
        if (data->Dc)
            DeleteDC(data->Dc);
        IM_DELETE(data);
        return false;
    }
    src->FontLoaderData = data;
    return true;
}

inline void FontSrcDestroy(ImFontAtlas*, ImFontConfig* src)
{
    SrcData* data = (SrcData*)src->FontLoaderData;
    if (!data)
        return;
    DeleteObject(data->ProbeFont);
    DeleteDC(data->Dc);
    IM_DELETE(data);
    src->FontLoaderData = nullptr;
}

inline bool HasGlyph(HDC dc, ImWchar codepoint)
{
    const wchar_t ch = (wchar_t)codepoint;
    WORD index = 0xFFFF;
    if (GetGlyphIndicesW(dc, &ch, 1, &index, GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR)
        return false;
    return index != 0xFFFF;
}

inline bool FontSrcContainsGlyph(ImFontAtlas*, ImFontConfig* src, ImWchar codepoint)
{
    SrcData* data = (SrcData*)src->FontLoaderData;
    SelectObject(data->Dc, data->ProbeFont);
    return HasGlyph(data->Dc, codepoint);
}

inline bool FontBakedInit(ImFontAtlas*, ImFontConfig* src, ImFontBaked* baked, void* loader_data)
{
    SrcData* data = (SrcData*)src->FontLoaderData;
    BakedData* baked_data = (BakedData*)loader_data;
    const float density = src->RasterizerDensity * baked->RasterizerDensity;
    baked_data->Font = CreateFace(data, ImMax(1, (int)(baked->Size * density * kSupersample + 0.5f)));
    if (!baked_data->Font)
        return false;
    if (!src->MergeMode)
    {
        TEXTMETRICW tm{};
        SelectObject(data->Dc, baked_data->Font);
        GetTextMetricsW(data->Dc, &tm);
        baked->Ascent = ImCeil((float)tm.tmAscent / (density * kSupersample));
        baked->Descent = ImFloor(-(float)tm.tmDescent / (density * kSupersample));
    }
    return true;
}

inline void FontBakedDestroy(ImFontAtlas*, ImFontConfig*, ImFontBaked*, void* loader_data)
{
    BakedData* baked_data = (BakedData*)loader_data;
    if (baked_data->Font)
        DeleteObject(baked_data->Font);
    baked_data->Font = nullptr;
}

inline bool FontBakedLoadGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void* loader_data, ImWchar codepoint, ImFontGlyph* out_glyph, float* out_advance_x)
{
    SrcData* data = (SrcData*)src->FontLoaderData;
    BakedData* baked_data = (BakedData*)loader_data;
    if (!baked_data->Font)
        return false;
    SelectObject(data->Dc, baked_data->Font);
    if (!HasGlyph(data->Dc, codepoint))
        return false;

    static const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    GLYPHMETRICS gm{};
    const DWORD buffer_size = GetGlyphOutlineW(data->Dc, codepoint, GGO_GRAY8_BITMAP, &gm, 0, nullptr, &identity);
    if (buffer_size == GDI_ERROR)
        return false;

    const float density = src->RasterizerDensity * baked->RasterizerDensity;
    const float advance = (float)gm.gmCellIncX / (density * kSupersample);
    if (out_advance_x != nullptr)
    {
        *out_advance_x = advance;
        return true;
    }

    out_glyph->Codepoint = codepoint;
    out_glyph->AdvanceX = advance;

    const int src_w = (int)gm.gmBlackBoxX;
    const int src_h = (int)gm.gmBlackBoxY;
    if (buffer_size == 0 || src_w <= 0 || src_h <= 0)
        return true;

    const int ss = kSupersample;
    const int src_left = (int)gm.gmptGlyphOrigin.x;
    const int src_top = -(int)gm.gmptGlyphOrigin.y;
    const int x0 = (int)ImFloor((float)src_left / (float)ss);
    const int y0 = (int)ImFloor((float)src_top / (float)ss);
    const int pad_x = src_left - x0 * ss;
    const int pad_y = src_top - y0 * ss;
    const int w = (pad_x + src_w + ss - 1) / ss;
    const int h = (pad_y + src_h + ss - 1) / ss;

    const int src_pitch = (src_w + 3) & ~3;
    static ImVector<unsigned char> gray;
    static ImVector<int> sums;
    gray.resize((int)buffer_size);
    sums.resize(w * h);
    memset(sums.Data, 0, (size_t)sums.Size * sizeof(int));
    if (GetGlyphOutlineW(data->Dc, codepoint, GGO_GRAY8_BITMAP, &gm, buffer_size, gray.Data, &identity) == GDI_ERROR)
        return false;
    for (int y = 0; y < src_h; ++y)
        for (int x = 0; x < src_w; ++x)
            sums[((y + pad_y) / ss) * w + (x + pad_x) / ss] += gray[y * src_pitch + x];
    ImVector<unsigned char>& temp = atlas->Builder->TempBuffer;
    temp.resize(w * h);
    unsigned char* pixels = temp.Data;
    const float gamma = CoverageGamma();
    for (int i = 0; i < w * h; ++i)
        pixels[i] = (unsigned char)(ImPow(ImMin((float)sums[i] / (float)(64 * ss * ss), 1.0f), gamma) * 255.0f + 0.5f);

    ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, w, h);
    if (pack_id == ImFontAtlasRectId_Invalid)
        return false;
    ImTextureRect* r = ImFontAtlasPackGetRect(atlas, pack_id);

    const float recip = 1.0f / density;
    out_glyph->X0 = (float)x0 * recip;
    out_glyph->Y0 = (float)y0 * recip + IM_ROUND(baked->Ascent);
    out_glyph->X1 = out_glyph->X0 + (float)w * recip;
    out_glyph->Y1 = out_glyph->Y0 + (float)h * recip;
    out_glyph->Visible = true;
    out_glyph->PackId = pack_id;
    ImFontAtlasBakedSetFontGlyphBitmap(atlas, baked, src, out_glyph, r, pixels, ImTextureFormat_Alpha8, w);
    return true;
}

inline const ImFontLoader* GetLoader()
{
    static ImFontLoader loader;
    loader.Name = "gdi";
    loader.FontSrcInit = FontSrcInit;
    loader.FontSrcDestroy = FontSrcDestroy;
    loader.FontSrcContainsGlyph = FontSrcContainsGlyph;
    loader.FontBakedInit = FontBakedInit;
    loader.FontBakedDestroy = FontBakedDestroy;
    loader.FontBakedLoadGlyph = FontBakedLoadGlyph;
    loader.FontBakedSrcLoaderDataSize = sizeof(BakedData);
    return &loader;
}

inline ImFont* AddFont(ImFontAtlas* atlas, const wchar_t* face, int weight, float size_pixels)
{
    PendingFace() = face;
    PendingWeight() = weight;
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.SizePixels = size_pixels;
    cfg.PixelSnapH = true;
    cfg.FontLoader = GetLoader();
    ImFormatString(cfg.Name, IM_ARRAYSIZE(cfg.Name), "%ls (GDI)", face);
    return atlas->AddFont(&cfg);
}
}
