#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "render.h"
#include "menu/CascadiaMonoBL.c"
#include <dwmapi.h>
#include <cmath>
#include <algorithm>
#include "menu/library.h"
#include "menu/allsetting.h"
#include "menu/gdi_font.h"
#include "menu/logo.h"
#include "../../ext/imgui/imgui_impl_win32.h"
#include "../../ext/imgui/imgui_impl_dx11.h"
#include "../../src/core/variables/variables.h"
#include "../../src/core/functions/settings/settings.h"
#include "../../src/core/functions/mics/mics.h"
#include "../../src/core/functions/aim/aim.h"
#include "../../src/core/functions/visual/visual.h"
#include "../../src/core/functions/world/world.h"
#include "../../src/core/cache/cache.h"
#include "../../src/core/net/ping.h"
#include "../../src/sdk/offsets.h"
#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

namespace {
constexpr int kLogoHeight = 46;
constexpr int kLogoGlowPad = 24;
constexpr float kLogoGlowSigma = 7.0f;
constexpr float kLogoGlowAlpha = 0.55f;
ID3D11ShaderResourceView* g_logoTexture = nullptr;
ID3D11ShaderResourceView* g_logoGlowTexture = nullptr;
ImVec2 g_logoSize(0.0f, 0.0f);

ID3D11ShaderResourceView* CreateTexture(ID3D11Device* device, const std::vector<unsigned char>& pixels, int width, int height) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = pixels.data();
    data.SysMemPitch = (UINT)width * 4;
    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* view = nullptr;
    if (!device || FAILED(device->CreateTexture2D(&desc, &data, &texture)))
        return nullptr;
    device->CreateShaderResourceView(texture, nullptr, &view);
    texture->Release();
    return view;
}

void CreateLogoTexture(ID3D11Device* device) {
    std::vector<unsigned char> pixels;
    std::vector<unsigned char> glow;
    const int width = Logo::Rasterize(kLogoHeight, pixels);
    Logo::BuildGlow(pixels, width, kLogoHeight, kLogoGlowPad, kLogoGlowSigma, glow);
    g_logoTexture = CreateTexture(device, pixels, width, kLogoHeight);
    g_logoGlowTexture = CreateTexture(device, glow, width + kLogoGlowPad * 2, kLogoHeight + kLogoGlowPad * 2);
    if (g_logoTexture)
        g_logoSize = ImVec2((float)width, (float)kLogoHeight);
}
}

OverlayWindow::OverlayWindow() : windowHandle(nullptr), d3dDevice(nullptr), d3dContext(nullptr), swapChain(nullptr), renderTarget(nullptr) {
    ZeroMemory(&windowClass, sizeof(windowClass));
}

void OverlayWindow::SetupD3D11(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL obtainedLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &swapChain, &d3dDevice, &obtainedLevel, &d3dContext);
    if (hr == DXGI_ERROR_UNSUPPORTED) {
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &swapChain, &d3dDevice, &obtainedLevel, &d3dContext);
    }
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) {
        d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
        backBuffer->Release();
    }
}

void OverlayWindow::CleanupD3D11() {
    if (renderTarget) { renderTarget->Release(); renderTarget = nullptr; }
    if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    if (d3dContext) { d3dContext->Release(); d3dContext = nullptr; }
    if (d3dDevice) { d3dDevice->Release(); d3dDevice = nullptr; }
}

bool OverlayWindow::Initialize() {
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = OverlayWndProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"jatos";
    if (!RegisterClassExW(&windowClass))
        return false;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    windowHandle = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW, windowClass.lpszClassName, L"jatos", WS_POPUP, 0, 0, screenW, screenH, nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!windowHandle)
        return false;
    SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(windowHandle, &margins);
    ShowWindow(windowHandle, SW_SHOW);
    UpdateWindow(windowHandle);
    SetupD3D11(windowHandle);
    CreateLogoTexture(d3dDevice);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    font_cfg.PixelSnapH = true;
    const wchar_t* menuFace = GdiFont::FaceExists(L"Segoe UI Variable Text") ? L"Segoe UI Variable Text" : L"Segoe UI";
    ImFont* menuFont = GdiFont::AddFont(io.Fonts, menuFace, FW_SEMIBOLD, 15.0f);
    if (!menuFont)
        menuFont = io.Fonts->AddFontFromMemoryTTF((void*)CascadiaMonoBL, (int)sizeof(CascadiaMonoBL), 15.0f, &font_cfg);
    io.FontGlobalScale = 1.0f;
    imGuiCustom::Initialize(menuFont);
    ImGui_ImplWin32_Init(windowHandle);
    ImGui_ImplDX11_Init(d3dDevice, d3dContext);
    return true;
}

void OverlayWindow::BeginFrame() {
    MSG msg;
    imGuiCustom::g_fontScale = variables::Misc::menuFontSize;
    imGuiCustom::g_animSpeed = variables::Misc::animSpeed;
    ImGui::GetStyle().Colors[ImGuiCol_PopupBg] = variables::Theme::panels;
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = variables::Theme::background;
    imGuiCustom::Theme& theme = imGuiCustom::GetThemeMutable();
    theme.WindowBg = variables::Theme::background;
    theme.CardBg = variables::Theme::panels;
    theme.ControlBg = variables::Theme::controls;
    theme.ControlInactive = imGuiCustom::LerpColor(variables::Theme::controls, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 0.12f);
    theme.KeybindBg = variables::Theme::controls;
    theme.Accent = variables::Theme::accent;
    theme.AccentText = variables::Theme::accent;
    theme.Text = variables::Theme::text;
    theme.TextBright = variables::Theme::textBright;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    {
        static bool prevMenuOpen = false;
        auto forceForeground = [](HWND hwnd) {
            HWND fg = GetForegroundWindow();
            DWORD curTid = GetCurrentThreadId();
            DWORD fgTid = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
            if (fgTid && fgTid != curTid)
                AttachThreadInput(curTid, fgTid, TRUE);
            BringWindowToTop(hwnd);
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            SetActiveWindow(hwnd);
            SetFocus(hwnd);
            if (fgTid && fgTid != curTid)
                AttachThreadInput(curTid, fgTid, FALSE);
        };
        if (variables::menuOpen) {
            SetWindowLong(windowHandle, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
            if (!prevMenuOpen)
                forceForeground(windowHandle);
        } else {
            SetWindowLong(windowHandle, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
            if (prevMenuOpen) {
                HWND rbx = FindWindowW(nullptr, L"Roblox");
                if (rbx)
                    forceForeground(rbx);
            }
        }
        prevMenuOpen = variables::menuOpen;
    }
    if (variables::Misc::streamProof) {
        SetWindowDisplayAffinity(windowHandle, WDA_EXCLUDEFROMCAPTURE);
    } else {
        SetWindowDisplayAffinity(windowHandle, WDA_NONE);
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

namespace {
constexpr const char* kTabs[] = {"Rage", "Movement", "Visuals", "Player", "Misc", "World"};
constexpr int kTabCount = IM_ARRAYSIZE(kTabs);
bool g_clientOpen = false;

void DrawPanel(const char* id, const ImVec2& pos, const ImVec2& size) {
    ImGui::PushID(id);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 min = ImVec2(std::floor(winPos.x + pos.x), std::floor(winPos.y + pos.y));
    const ImVec2 max = ImVec2(min.x + std::floor(size.x), min.y + std::floor(size.y));
    draw->AddRectFilled(min, max, imGuiCustom::ColorU32(imGuiCustom::GetTheme().CardBg), imGuiCustom::kCardRounding);
    draw->AddRect(min, max, imGuiCustom::Hairline(), imGuiCustom::kCardRounding, 0, 1.0f);
    ImGui::PopID();
}

int DrawSidebar(const char* const labels[], int count, int current, const ImVec2& pos, float width) {
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 min = ImVec2(std::floor(winPos.x + pos.x), std::floor(winPos.y + pos.y));
    const float row_h = 32.0f;
    const float row_step = row_h + 2.0f;
    const imGuiCustom::Theme& theme = imGuiCustom::GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const bool in_list = current >= 0 && current < count;
    static float pill = in_list ? (float)current : 0.0f;
    static float pill_alpha = in_list ? 1.0f : 0.0f;
    const float anim_step = ImClamp(imGuiCustom::AnimDelta() * 14.0f, 0.0f, 1.0f);
    if (in_list)
        pill = (pill_alpha < 0.05f) ? (float)current : ImLerp(pill, (float)current, anim_step);
    pill_alpha = ImLerp(pill_alpha, in_list ? 1.0f : 0.0f, anim_step);
    const ImVec2 pill_min(min.x, min.y + row_step * pill);
    draw->AddRectFilled(pill_min, pill_min + ImVec2(width, row_h), imGuiCustom::ColorU32(theme.ControlInactive, pill_alpha), 8.0f);
    const imGuiCustom::Fonts& fonts = imGuiCustom::GetFonts();
    ImFont* font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
    const float font_size = imGuiCustom::FontLabel();
    int result = current;
    for (int i = 0; i < count; ++i) {
        const ImVec2 row_min(min.x, min.y + row_step * (float)i);
        ImGui::PushID(labels[i]);
        ImGui::SetCursorScreenPos(row_min);
        if (ImGui::InvisibleButton("##tab", ImVec2(width, row_h)))
            result = i;
        const float hover = imGuiCustom::AnimateFloat(ImGui::GetItemID(), ImGui::IsItemHovered(), 14.0f);
        const float selected = ImClamp(1.0f - std::fabs(pill - (float)i), 0.0f, 1.0f) * pill_alpha;
        if (hover * (1.0f - selected) > 0.01f)
            draw->AddRectFilled(row_min, row_min + ImVec2(width, row_h), imGuiCustom::ColorU32(theme.ControlBg, hover * (1.0f - selected)), 8.0f);
        const ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, labels[i]);
        draw->AddText(font, font_size, ImVec2(row_min.x + 12.0f, std::floor(row_min.y + (row_h - text_size.y) * 0.5f)), imGuiCustom::ColorU32(imGuiCustom::LerpColor(theme.Text, theme.TextBright, ImMax(selected, hover * 0.6f))), labels[i]);
        ImGui::PopID();
    }
    return result;
}

void DrawGear(ImDrawList* draw, const ImVec2& center, float radius, ImU32 col, ImU32 hole_col) {
    const int teeth = 8;
    const float inner = radius * 0.74f;
    const float step = IM_PI * 2.0f / (float)teeth;
    ImVec2 pts[teeth * 4];
    for (int i = 0; i < teeth; ++i) {
        const float a = step * (float)i;
        const float offs[4] = {-0.30f, -0.17f, 0.17f, 0.30f};
        const float radii[4] = {inner, radius, radius, inner};
        for (int k = 0; k < 4; ++k)
            pts[i * 4 + k] = ImVec2(center.x + std::cos(a + step * offs[k]) * radii[k], center.y + std::sin(a + step * offs[k]) * radii[k]);
    }
    draw->AddConcavePolyFilled(pts, teeth * 4, col);
    draw->AddCircleFilled(center, radius * 0.34f, hole_col, 24);
}

bool DrawGearButton(const ImVec2& pos, float size, bool active) {
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 min = ImVec2(std::floor(winPos.x + pos.x), std::floor(winPos.y + pos.y));
    const imGuiCustom::Theme& theme = imGuiCustom::GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(min);
    const bool pressed = ImGui::InvisibleButton("##client_info", ImVec2(size, size));
    const ImGuiID id = ImGui::GetItemID();
    const float hover = imGuiCustom::AnimateFloat(id, ImGui::IsItemHovered(), 14.0f);
    const float selected = imGuiCustom::AnimateFloat(id + 1, active, 14.0f);
    const ImVec4 bg = imGuiCustom::LerpColor(imGuiCustom::LerpColor(theme.WindowBg, theme.ControlBg, hover), theme.ControlInactive, selected);
    draw->AddRectFilled(min, min + ImVec2(size, size), imGuiCustom::ColorU32(bg), 8.0f);
    const ImVec4 icon = imGuiCustom::LerpColor(imGuiCustom::LerpColor(theme.Text, theme.TextBright, hover * 0.6f), theme.Accent, selected);
    DrawGear(draw, min + ImVec2(size * 0.5f, size * 0.5f), size * 0.29f, imGuiCustom::ColorU32(icon), imGuiCustom::ColorU32(bg));
    return pressed;
}

void DrawClientWindow(const ImVec2& menu_origin, float menu_fade) {
    static float anim = 0.0f;
    static float height = 380.0f;
    anim = ImLerp(anim, g_clientOpen ? 1.0f : 0.0f, ImClamp(imGuiCustom::AnimDelta() * 12.0f, 0.0f, 1.0f));
    if (anim < 0.01f)
        return;
    namespace L = AllSetting::Layout;
    const float width = 300.0f;
    const float gap = 12.0f;
    const bool on_right = menu_origin.x + L::kWindowW + gap + width <= ImGui::GetIO().DisplaySize.x;
    const float x = on_right ? menu_origin.x + L::kWindowW + gap : menu_origin.x - gap - width;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menu_fade * anim);
    ImGui::SetNextWindowPos(ImVec2(std::floor(x), menu_origin.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::Begin("##client", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
    const imGuiCustom::Theme& theme = imGuiCustom::GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImVec2(std::floor(ImGui::GetWindowPos().x), std::floor(ImGui::GetWindowPos().y));
    const ImVec2 win_max = origin + ImVec2(width, height);
    imGuiCustom::DrawShadow(ImGui::GetBackgroundDrawList(), origin, win_max, imGuiCustom::kWindowRounding, 1.5f);
    draw->AddRectFilled(origin, win_max, imGuiCustom::ColorU32(theme.WindowBg), imGuiCustom::kWindowRounding);
    draw->AddRect(origin, win_max, imGuiCustom::Hairline(), imGuiCustom::kWindowRounding, 0, 1.0f);
    const imGuiCustom::Fonts& fonts = imGuiCustom::GetFonts();
    ImFont* font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
    const float title_fs = std::round(20.0f * imGuiCustom::g_fontScale);
    const ImVec2 title_sz = font->CalcTextSizeA(title_fs, FLT_MAX, 0.0f, variables::Client::name);
    draw->AddText(font, title_fs, origin + ImVec2(std::floor((width - title_sz.x) * 0.5f), std::floor((56.0f - title_sz.y) * 0.5f)), imGuiCustom::ColorU32(theme.TextBright), variables::Client::name);
    height = Settings::RenderClientMenu(width);
    ImGui::End();
    ImGui::PopStyleVar();
}
}

void OverlayWindow::RenderMenu() {
    float dt = ImGui::GetIO().DeltaTime;
    if (dt <= 0.0f || dt > 0.1f)
        dt = 0.016f;
    dt *= imGuiCustom::g_animSpeed;
    static float menuFade = 0.0f;
    static float tabAnim = 1.0f;
    static int prevTab = 0;
    if (variables::menuOpen) {
        menuFade += dt * 8.0f;
        if (menuFade > 1.0f)
            menuFade = 1.0f;
    } else {
        menuFade -= dt * 8.0f;
        if (menuFade < 0.0f)
            menuFade = 0.0f;
    }
    if (!variables::menuOpen && menuFade <= 0.001f)
        return;
    if (variables::selectedTab != prevTab) {
        prevTab = variables::selectedTab;
        tabAnim = 0.0f;
    }
    tabAnim = (std::min)(1.0f, tabAnim + dt * 7.0f);
    const float eased = tabAnim * tabAnim * (3.0f - 2.0f * tabAnim);
    namespace L = AllSetting::Layout;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuFade);
    ImGui::SetNextWindowSize(ImVec2(L::kWindowW, L::kWindowH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(80.0f, 80.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("jatos", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    const imGuiCustom::Theme& theme = imGuiCustom::GetTheme();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImVec2(std::floor(ImGui::GetWindowPos().x), std::floor(ImGui::GetWindowPos().y));
    const ImVec2 winMax = origin + ImVec2(L::kWindowW, L::kWindowH);
    imGuiCustom::DrawShadow(ImGui::GetBackgroundDrawList(), origin, winMax, imGuiCustom::kWindowRounding, 1.5f);
    draw->AddRectFilled(origin, winMax, imGuiCustom::ColorU32(theme.WindowBg), imGuiCustom::kWindowRounding);
    draw->AddRect(origin, winMax, imGuiCustom::Hairline(), imGuiCustom::kWindowRounding, 0, 1.0f);
    const imGuiCustom::Fonts& fonts = imGuiCustom::GetFonts();
    ImFont* title_font = fonts.CascadiaMonoBL ? fonts.CascadiaMonoBL : ImGui::GetFont();
    if (g_logoTexture) {
        const ImVec2 logo_min = origin + ImVec2(std::floor((L::kSidebarW - g_logoSize.x) * 0.5f), 16.0f);
        const ImTextureRef logo_tex((ImTextureID)(intptr_t)g_logoTexture);
        if (g_logoGlowTexture) {
            const ImVec2 glow_pad((float)kLogoGlowPad, (float)kLogoGlowPad);
            draw->AddImage(ImTextureRef((ImTextureID)(intptr_t)g_logoGlowTexture), logo_min - glow_pad, logo_min + g_logoSize + glow_pad, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), imGuiCustom::ColorU32(theme.Accent, kLogoGlowAlpha));
        }
        draw->AddImage(logo_tex, logo_min, logo_min + g_logoSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), imGuiCustom::ColorU32(theme.Accent));
    } else {
        draw->AddText(title_font, std::round(18.0f * imGuiCustom::g_fontScale), origin + ImVec2(20.0f, 16.0f), imGuiCustom::ColorU32(theme.TextBright), "jatos");
    }
    draw->AddLine(origin + ImVec2(L::kSidebarW + 0.5f, 12.0f), origin + ImVec2(L::kSidebarW + 0.5f, L::kWindowH - 12.0f), imGuiCustom::Hairline(), 1.0f);
    variables::selectedTab = DrawSidebar(kTabs, kTabCount, variables::selectedTab, ImVec2(8.0f, 16.0f + (float)kLogoHeight + 22.0f), L::kSidebarW - 16.0f);
    if (DrawGearButton(ImVec2(L::kPanelX[1] + L::kPanelW - 32.0f, std::floor((L::kPanelY - 32.0f) * 0.5f)), 32.0f, g_clientOpen))
        g_clientOpen = !g_clientOpen;
    variables::selectedTab = ImClamp(variables::selectedTab, 0, kTabCount - 1);
    const char* tab_title = kTabs[variables::selectedTab];
    draw->AddText(title_font, std::round(20.0f * imGuiCustom::g_fontScale), origin + ImVec2(L::kPanelX[0] + 4.0f, 14.0f), imGuiCustom::ColorU32(theme.TextBright), tab_title);
    DrawPanel("panel_left", ImVec2(L::kPanelX[0], L::kPanelY), ImVec2(L::kPanelW, L::kPanelH));
    DrawPanel("panel_right", ImVec2(L::kPanelX[1], L::kPanelY), ImVec2(L::kPanelW, L::kPanelH));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuFade * eased);
    draw->PushClipRect(origin + ImVec2(L::kPanelX[0], L::kPanelY), origin + ImVec2(L::kPanelX[1] + L::kPanelW, L::kPanelY + L::kPanelH), true);
    if (variables::selectedTab == 0) {
        Settings::RenderAimMenu();
    } else if (variables::selectedTab == 2) {
        Settings::RenderVisualMenu();
    } else if (variables::selectedTab == 3) {
        Mics::RenderLocalMenu();
    } else if (variables::selectedTab == 4) {
        Mics::RenderMiscMenu();
    } else if (variables::selectedTab == 5) {
        World::RenderMenu();
    }
    draw->PopClipRect();
    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar();
    DrawClientWindow(origin, menuFade);
}

void OverlayWindow::render(ImDrawList* drawList) {
    if (variables::Aimbot::enabled && variables::Aimbot::showFOV) {
        POINT p;
        GetCursorPos(&p);
        ImVec2 center = ImVec2(static_cast<float>(p.x), static_cast<float>(p.y));
        const ImU32 fc = imGuiCustom::ColorU32(variables::Aimbot::fovColor);
        drawList->AddCircle(center, variables::Aimbot::fovRadius, IM_COL32(0, 0, 0, 255), 64, 2.0f);
        drawList->AddCircle(center, variables::Aimbot::fovRadius, fc, 64, 1.0f);
    }
    if (variables::Misc::keybinds) {
        struct KeyRow {
            const char* name;
            bool on;
            int key;
        };
        KeyRow rows[] = {
            {"Aimbot", variables::Aimbot::enabled, variables::Aimbot::aimbotKey},
            {"Triggerbot", variables::Aimbot::triggerbot, variables::Aimbot::triggerKey},
            {"FOV", variables::Movement::fov, variables::Movement::fovKey},
            {"Fly", variables::Movement::fly, variables::Movement::flyKey},
            {"Noclip", variables::Movement::noclip, variables::Movement::noclipKey},
        };
        int shown = 0;
        for (auto& r : rows) {
            if (r.on)
                ++shown;
        }
        if (shown > 0) {
            ImGui::SetNextWindowPos(ImVec2(12.0f, 120.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(224.0f, 0.0f), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.85f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, imGuiCustom::ColorU32(imGuiCustom::GetTheme().Text));
            ImGui::Begin("keybinds", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
            ImFont* kfont = Visuals::EspFont();
            const float kfs = 15.0f;
            ImDrawList* kdl = ImGui::GetWindowDrawList();
            const ImVec2 kpos = ImGui::GetWindowPos();
            const ImVec2 ksize = ImGui::GetWindowSize();
            kdl->AddRectFilled(kpos, kpos + ksize, imGuiCustom::ColorU32(imGuiCustom::GetTheme().CardBg, 0.85f), imGuiCustom::kCardRounding);
            kdl->AddRect(kpos, kpos + ksize, imGuiCustom::Hairline(), imGuiCustom::kCardRounding, 0, 1.0f);
            ImVec2 cur = kpos + ImVec2(12.0f, 10.0f);
            kdl->AddText(kfont, kfs, cur, imGuiCustom::ColorU32(imGuiCustom::GetTheme().TextBright), "Keybinds");
            cur.y += 24.0f;
            for (auto& r : rows) {
                if (!r.on)
                    continue;
                const bool down = (r.key == 0) ? true : Aimbot::IsAimKeyDown(r.key);
                const char* keyName = (r.key == 0) ? "Always" : imGuiCustom::KeyName(r.key);
                const char* state = down ? "on" : "off";
                ImU32 stateCol = down ? imGuiCustom::ColorU32(ImVec4(0.1882f, 0.8196f, 0.3451f, 1.0f)) : imGuiCustom::ColorU32(ImVec4(0.5569f, 0.5569f, 0.5765f, 1.0f));
                kdl->AddText(kfont, kfs, cur, imGuiCustom::ColorU32(imGuiCustom::GetTheme().Text), r.name);
                ImVec2 st = kfont->CalcTextSizeA(kfs, FLT_MAX, 0.0f, state);
                ImVec2 kn = kfont->CalcTextSizeA(kfs, FLT_MAX, 0.0f, keyName);
                kdl->AddText(kfont, kfs, ImVec2(kpos.x + ksize.x - 12.0f - st.x, cur.y), stateCol, state);
                kdl->AddText(kfont, kfs, ImVec2(kpos.x + ksize.x - 12.0f - st.x - 6.0f - kn.x, cur.y), imGuiCustom::ColorU32(imGuiCustom::GetTheme().TextBright), keyName);
                cur.y += 20.0f;
            }
            ImGui::Dummy(ImVec2(208.0f, (float)shown * 20.0f + 26.0f));
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }
    }
    if (!variables::Misc::watermark)
        return;
    static auto lastTime = std::chrono::steady_clock::now();
    static int frameCount = 0;
    static int fps = 0;
    ++frameCount;
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() >= 100) {
        fps = frameCount * 10;
        frameCount = 0;
        lastTime = now;
    }
    char clock[16] = "--:--:--";
    {
        const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm{};
        localtime_s(&tm, &t);
        strftime(clock, sizeof(clock), "%H:%M:%S", &tm);
    }
    const int ping = Ping::GetMs();
    char text[192];
    const int hs = Aimbot::g_hookStatus.load();
    char hookStr[16];
    if (hs < 0) std::snprintf(hookStr, sizeof(hookStr), "hook:err%d", -hs);
    else std::snprintf(hookStr, sizeof(hookStr), "hook:m%dk%d", (hs & 1) ? 1 : 0, (hs & 2) ? 1 : 0);
    const bool mR = Aimbot::g_ms_R.load();
    const bool mL = Aimbot::g_ms_L.load();
    if (ping >= 0)
        std::snprintf(text, sizeof(text), "jatos | %d fps | %s | players %d | %dms | %s | %s L=%d R=%d", fps, Offsets::ClientVersion.c_str(), (int)PlayerCache::players.size(), ping, clock, hookStr, mL, mR);
    else
        std::snprintf(text, sizeof(text), "jatos | %d fps | %s | players %d | -- | %s | %s L=%d R=%d", fps, Offsets::ClientVersion.c_str(), (int)PlayerCache::players.size(), clock, hookStr, mL, mR);
    ImFont* font = Visuals::EspFont();
    const float fontSize = 15.0f;
    const ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
    const ImVec2 pad(12.0f, 7.0f);
    const float w = ts.x + pad.x * 2.0f;
    const float h = ts.y + pad.y * 2.0f;
    const float sw = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
    const ImVec2 wmin(std::floor(sw - w - 10.0f), 10.0f);
    const ImVec2 wmax(std::floor(wmin.x + w), std::floor(wmin.y + h));
    drawList->AddRectFilled(wmin, wmax, imGuiCustom::ColorU32(imGuiCustom::GetTheme().CardBg, 0.9f), h * 0.5f);
    drawList->AddRect(wmin, wmax, imGuiCustom::Hairline(), h * 0.5f, 0, 1.0f);
    drawList->AddText(font, fontSize, ImVec2(std::floor(wmin.x + pad.x), std::floor(wmin.y + pad.y)), imGuiCustom::ColorU32(imGuiCustom::GetTheme().TextBright), text);
}

void OverlayWindow::EndFrame() {
    ImGui::Render();
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    d3dContext->OMSetRenderTargets(1, &renderTarget, nullptr);
    d3dContext->ClearRenderTargetView(renderTarget, clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swapChain->Present(variables::Misc::vsync ? 1 : 0, 0);
}

void OverlayWindow::Cleanup() {
    if (g_logoTexture) { g_logoTexture->Release(); g_logoTexture = nullptr; }
    if (g_logoGlowTexture) { g_logoGlowTexture->Release(); g_logoGlowTexture = nullptr; }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupD3D11();
    if (windowHandle) {
        DestroyWindow(windowHandle);
        windowHandle = nullptr;
    }
    UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
}

HWND OverlayWindow::GetWindowHandle() const {
    return windowHandle;
}
