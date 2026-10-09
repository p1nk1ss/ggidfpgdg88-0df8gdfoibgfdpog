#pragma once

#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>

namespace Logo
{
constexpr float kViewMinX = 1.9048f;
constexpr float kViewMinY = -989.4286f;
constexpr float kViewW = 1024.0952f;
constexpr float kViewH = 955.6988f;

constexpr const char* kPath =
    "M512 988Q430 977 343.5 950.0Q257 923 194 887Q146 859 98.5 816.0Q51 773 23 734Q5 709 2.5 699.5Q0 690 "
    "8 677Q12 669 27.0 652.0Q42 635 60 617Q164 519 340 465Q392 448 382.5 446.0Q373 444 310 458Q143 495 41"
    " 578L12 601V555Q12 516 19.0 486.5Q26 457 38 442Q53 426 100.0 407.0Q147 388 206 373Q252 362 284.5 359"
    ".0Q317 356 386.5 358.0Q456 360 486.5 364.0Q517 368 550 380Q591 394 631.5 415.5Q672 437 675 445Q674 4"
    "47 648.5 455.0Q623 463 585 473Q466 505 387.0 543.0Q308 581 260 630Q238 651 236.5 666.0Q235 681 253 7"
    "01Q273 723 337.0 755.0Q401 787 452 801Q508 815 595.0 813.5Q682 812 742 797L798 781L810 796Q828 823 8"
    "59.0 883.5Q890 944 890 955Q863 968 722.0 980.5Q581 993 512 988Q512 988 512 988ZM558 688Q526 682 482."
    "5 669.0Q439 656 426 649Q422 646 442.0 642.5Q462 639 500 636Q608 625 684.0 598.5Q760 572 786 534Q802 "
    "510 803.0 477.5Q804 445 789 414Q765 366 700.5 320.5Q636 275 550 245Q518 234 489.0 229.5Q460 225 408 "
    "223Q334 221 277.0 231.5Q220 242 163 269L121 288L105 267L83 235Q68 213 53 189Q22 142 24.0 126.0Q26 11"
    "0 67 96Q186 48 321.0 37.0Q456 26 587 52Q673 70 757.0 103.0Q841 136 902 176Q956 211 976.5 242.0Q997 2"
    "73 1004 330L1008 361L987 334Q934 262 840.0 207.0Q746 152 635 127Q570 113 556.0 114.5Q542 116 594 131"
    "Q726 171 826.5 240.0Q927 309 975 394Q993 427 1009.0 469.0Q1025 511 1026 529Q1023 543 988.5 579.5Q954"
    " 616 925 636Q911 644 887.0 654.5Q863 665 840 673Q808 684 778.0 687.5Q748 691 688 692Q646 693 609.0 6"
    "92.0Q572 691 558 688Q558 688 558 688Z";

struct Edge
{
    float X0, Y0, X1, Y1;
};

inline std::vector<Edge> BuildEdges(float scale)
{
    std::vector<Edge> edges;
    auto map_x = [&](float x) { return (x - kViewMinX) * scale; };
    auto map_y = [&](float y) { return (-y - kViewMinY) * scale; };
    auto line = [&](float x0, float y0, float x1, float y1) { edges.push_back({x0, y0, x1, y1}); };

    const char* p = kPath;
    char cmd = 0;
    float start_x = 0.0f, start_y = 0.0f, cur_x = 0.0f, cur_y = 0.0f;
    auto number = [&]() { char* end = nullptr; const float v = std::strtof(p, &end); p = end; return v; };
    while (*p)
    {
        if (*p == ' ' || *p == ',')
        {
            ++p;
            continue;
        }
        if (*p == 'M' || *p == 'L' || *p == 'H' || *p == 'V' || *p == 'Q' || *p == 'Z')
            cmd = *p++;
        if (cmd == 'M')
        {
            cur_x = start_x = map_x(number());
            cur_y = start_y = map_y(number());
        }
        else if (cmd == 'L' || cmd == 'H' || cmd == 'V')
        {
            const float nx = (cmd == 'V') ? cur_x : map_x(number());
            const float ny = (cmd == 'H') ? cur_y : map_y(number());
            line(cur_x, cur_y, nx, ny);
            cur_x = nx;
            cur_y = ny;
        }
        else if (cmd == 'Q')
        {
            const float cx = map_x(number());
            const float cy = map_y(number());
            const float ex = map_x(number());
            const float ey = map_y(number());
            const int steps = 8;
            float px = cur_x, py = cur_y;
            for (int i = 1; i <= steps; ++i)
            {
                const float t = (float)i / (float)steps;
                const float u = 1.0f - t;
                const float nx = u * u * cur_x + 2.0f * u * t * cx + t * t * ex;
                const float ny = u * u * cur_y + 2.0f * u * t * cy + t * t * ey;
                line(px, py, nx, ny);
                px = nx;
                py = ny;
            }
            cur_x = ex;
            cur_y = ey;
        }
        else if (cmd == 'Z')
        {
            line(cur_x, cur_y, start_x, start_y);
            cur_x = start_x;
            cur_y = start_y;
            cmd = 0;
        }
        else
        {
            ++p;
        }
    }
    return edges;
}

inline int Rasterize(int height, std::vector<unsigned char>& rgba)
{
    const float scale = (float)height / kViewH;
    const int width = (int)std::ceil(kViewW * scale);
    const std::vector<Edge> edges = BuildEdges(scale);
    std::vector<float> coverage((size_t)width * height, 0.0f);

    const int sub_rows = 8;
    std::vector<std::pair<float, int>> hits;
    for (int row = 0; row < height * sub_rows; ++row)
    {
        const float y = ((float)row + 0.5f) / (float)sub_rows;
        hits.clear();
        for (const Edge& e : edges)
        {
            if ((e.Y0 <= y) == (e.Y1 <= y))
                continue;
            hits.push_back({e.X0 + (y - e.Y0) / (e.Y1 - e.Y0) * (e.X1 - e.X0), e.Y1 > e.Y0 ? 1 : -1});
        }
        std::sort(hits.begin(), hits.end());
        float* out = &coverage[(size_t)(row / sub_rows) * width];
        int winding = 0;
        for (size_t i = 0; i + 1 < hits.size(); ++i)
        {
            winding += hits[i].second;
            if (winding == 0)
                continue;
            const float xa = (std::max)(hits[i].first, 0.0f);
            const float xb = (std::min)(hits[i + 1].first, (float)width);
            for (int x = (int)xa; x < width && (float)x < xb; ++x)
                out[x] += ((std::min)(xb, (float)x + 1.0f) - (std::max)(xa, (float)x)) / (float)sub_rows;
        }
    }

    rgba.resize((size_t)width * height * 4);
    for (size_t i = 0; i < coverage.size(); ++i)
    {
        rgba[i * 4 + 0] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = (unsigned char)((std::min)(std::pow(coverage[i], 0.8f), 1.0f) * 255.0f + 0.5f);
    }
    return width;
}

inline void BuildGlow(const std::vector<unsigned char>& rgba, int width, int height, int pad, float sigma, std::vector<unsigned char>& out_rgba)
{
    const int out_w = width + pad * 2;
    const int out_h = height + pad * 2;
    std::vector<float> src((size_t)out_w * out_h, 0.0f);
    std::vector<float> tmp(src.size(), 0.0f);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            src[(size_t)(y + pad) * out_w + (x + pad)] = (float)rgba[((size_t)y * width + x) * 4 + 3] / 255.0f;

    const int radius = (int)std::ceil(sigma * 3.0f);
    std::vector<float> kernel((size_t)radius * 2 + 1);
    float kernel_sum = 0.0f;
    for (int i = -radius; i <= radius; ++i)
        kernel_sum += kernel[(size_t)(i + radius)] = std::exp(-(float)(i * i) / (2.0f * sigma * sigma));
    for (float& k : kernel)
        k /= kernel_sum;

    for (int y = 0; y < out_h; ++y)
        for (int x = 0; x < out_w; ++x)
        {
            float sum = 0.0f;
            for (int i = -radius; i <= radius; ++i)
                if (x + i >= 0 && x + i < out_w)
                    sum += src[(size_t)y * out_w + (x + i)] * kernel[(size_t)(i + radius)];
            tmp[(size_t)y * out_w + x] = sum;
        }
    out_rgba.resize(src.size() * 4);
    for (int y = 0; y < out_h; ++y)
        for (int x = 0; x < out_w; ++x)
        {
            float sum = 0.0f;
            for (int i = -radius; i <= radius; ++i)
                if (y + i >= 0 && y + i < out_h)
                    sum += tmp[(size_t)(y + i) * out_w + x] * kernel[(size_t)(i + radius)];
            const size_t o = ((size_t)y * out_w + x) * 4;
            out_rgba[o + 0] = out_rgba[o + 1] = out_rgba[o + 2] = 255;
            out_rgba[o + 3] = (unsigned char)((std::min)(sum, 1.0f) * 255.0f + 0.5f);
        }
}
}
