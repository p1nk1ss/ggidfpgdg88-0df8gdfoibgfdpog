#include "app.h"
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <chrono>
#include "../../memory/memory.h"
#include "../../sdk/offsets.h"
#include "../../sdk/sdk.h"
#include "../cache/cache.h"
#include "../globals/globals.h"
#include "../tp_handler/tp_handler.h"
#include "../functions/aim/aim.h"
#include "../functions/visual/visual.h"
#include "../functions/mics/mics.h"
#include "../functions/movement/movement.h"
#include "../functions/wallshot/wallshot.h"
#include "../functions/world/world.h"
#include "../cache/workspace.h"
#include "../net/ping.h"
#include "../../render/render.h"

#pragma comment(lib, "winmm.lib")

namespace {
constexpr const char* kProc = "RobloxPlayerBeta.exe";
constexpr const wchar_t* kTitle = L"Roblox";
}

namespace App {
bool game_open() {
    return FindWindowW(nullptr, kTitle) != nullptr;
}

bool init() {
    if (!memory->find_process_id(kProc)) {
        printf("unable to get pid.\nmake sure roblox is running.\n");
        system("pause");
        return false;
    }
    if (!memory->attach_to_process(kProc)) {
        printf("unable to attach to roblox.");
        return false;
    }
    if (!memory->find_module_address(kProc)) {
        printf("unable to find main module address!");
        return false;
    }
    const auto base = memory->get_module_address();
    if (!base) {
        printf("base address is null.");
        return false;
    }
    std::uintptr_t fake = 0, dm = 0, ve = 0;
    printf("base=0x%llX  FakeDMPtr RVA=0x%llX -> abs=0x%llX\n",
        (unsigned long long)base,
        (unsigned long long)Offsets::FakeDataModel::Pointer,
        (unsigned long long)(base + Offsets::FakeDataModel::Pointer));
    // Enumerate modules of the process
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, memory->get_process_id());
        if (snap != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W me{};
            me.dwSize = sizeof(me);
            printf("--- modules ---\n");
            if (Module32FirstW(snap, &me)) {
                int shown = 0;
                do {
                    wprintf(L"  %-40s base=0x%llX size=0x%X\n", me.szModule,
                        (unsigned long long)(uintptr_t)me.modBaseAddr, (unsigned int)me.modBaseSize);
                    if (++shown >= 25) break;
                } while (Module32NextW(snap, &me));
            }
            printf("---------------\n");
            CloseHandle(snap);
        }
    }
    // Probe RPM: does it actually succeed, or return zeros with error?
    {
        std::uint8_t probe[8]{};
        SIZE_T got = 0;
        BOOL ok = ReadProcessMemory(memory->get_process_handle(),
            reinterpret_cast<LPCVOID>(base + Offsets::FakeDataModel::Pointer), probe, 8, &got);
        printf("probe RPM: ok=%d got=%zu err=%lu bytes=%02X%02X%02X%02X%02X%02X%02X%02X\n",
            ok ? 1 : 0, got, GetLastError(),
            probe[7], probe[6], probe[5], probe[4], probe[3], probe[2], probe[1], probe[0]);
        // Also probe first bytes of module for sanity
        std::uint8_t hdr[8]{};
        ReadProcessMemory(memory->get_process_handle(), reinterpret_cast<LPCVOID>(base), hdr, 8, &got);
        printf("module hdr[0..7]=%02X%02X%02X%02X%02X%02X%02X%02X (should start 4D5A = MZ)\n",
            hdr[0], hdr[1], hdr[2], hdr[3], hdr[4], hdr[5], hdr[6], hdr[7]);
    }
    printf("waiting for place to load (join a game)...\n");
    int diagCount = 0;
    for (int i = 0; i < 600; ++i) {
        fake = memory->read<std::uintptr_t>(base + Offsets::FakeDataModel::Pointer);
        if (fake) {
            dm = memory->read<std::uintptr_t>(fake + Offsets::FakeDataModel::RealDataModel);
            ve = memory->read<std::uintptr_t>(base + Offsets::VisualEngine::Pointer);
            if (dm && ve)
                break;
        }
        if (++diagCount % 25 == 0) {
            std::uint8_t peek[8]{};
            memory->read_raw(base + Offsets::FakeDataModel::Pointer, peek, 8);
            printf("[%ds] fake=0x%llX ve=0x%llX raw=%02X%02X%02X%02X%02X%02X%02X%02X\n",
                diagCount / 5, (unsigned long long)fake, (unsigned long long)ve,
                peek[7], peek[6], peek[5], peek[4], peek[3], peek[2], peek[1], peek[0]);
        }
        if (!game_open()) {
            printf("roblox closed while waiting.\n");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (!fake) {
        printf("fake datamodel pointer is null after 120s.\nmake sure you joined a game and re-run.\n");
        system("pause");
        return false;
    }
    if (!dm) {
        printf("datamodel pointer is null.\n");
        system("pause");
        return false;
    }
    if (!ve) {
        printf("visualengine pointer is null.\n");
        system("pause");
        return false;
    }
    Globals::dataModel = RBX::RbxInstance{dm};
    Globals::renderEngine = RBX::RenderEngine{ve};
    Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
    Globals::players = Globals::dataModel.FindChildByClass("Players");
    Globals::camera = Globals::workspace.FindChildByClass("Camera");
    const auto local = memory->read<std::uintptr_t>(Globals::players.Addr + Offsets::Player::LocalPlayer);
    Globals::localPlayer = RBX::RbxInstance{local};
    system("cls");
    return true;
}

std::int32_t Run() {
    if (!init())
        return 1;
    OverlayWindow overlay;
    if (!overlay.Initialize()) {
        std::cout << "[!] failed to initialize overlay\n";
        return -1;
    }
    std::cout << "[+] overlay initialized\n[*] press insert to toggle menu\n\n";
    Aimbot::InstallMouseHook();
    timeBeginPeriod(1);
    std::thread tpThread(Core::tp_handler::thread);
    std::thread localThread(Mics::Loop);
    std::thread wsThread(WorkspaceCache::Loop);
    std::thread pingThread(Ping::Loop);
    std::thread moveThread(Movement::Loop);
    std::thread wallshotThread(Wallshot::Loop);
    std::thread aimThread(Aimbot::AimLoop);
    std::thread worldThread(World::Loop);
    int frame = 0;
    while (memory->IsConnected() && Globals::running) {
        const auto frameStart = std::chrono::steady_clock::now();
        if (!game_open())
            break;
        if (GetAsyncKeyState(VK_INSERT) & 1)
            variables::menuOpen = !variables::menuOpen;
        if (!Globals::renderEngine.Addr || !Globals::players.Addr || !Globals::localPlayer.Addr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if (frame % 3 == 0)
            PlayerCache::updateplayers();
        ++frame;
        overlay.BeginFrame();
        overlay.RenderMenu();
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        overlay.render(dl);
        const auto vm = Globals::renderEngine.GetViewMat();
        Aimbot::RunAimbot(vm);
        Visuals::RenderESP(dl, vm);
        overlay.EndFrame();
        const int limit = variables::Misc::fpsLimit;
        if (limit >= 60) {
            const auto budget = std::chrono::microseconds(1000000 / limit);
            auto spent = std::chrono::steady_clock::now() - frameStart;
            if (spent < budget) {
                auto remain = budget - spent;
                if (remain > std::chrono::milliseconds(2))
                    std::this_thread::sleep_for(remain - std::chrono::milliseconds(1));
                while (std::chrono::steady_clock::now() - frameStart < budget) {
                }
            }
        }
    }
    timeEndPeriod(1);
    Globals::running = false;
    if (tpThread.joinable())
        tpThread.join();
    if (localThread.joinable())
        localThread.join();
    if (wsThread.joinable())
        wsThread.join();
    if (pingThread.joinable())
        pingThread.join();
    if (moveThread.joinable())
        moveThread.join();
    if (wallshotThread.joinable())
        wallshotThread.join();
    if (aimThread.joinable())
        aimThread.join();
    if (worldThread.joinable())
        worldThread.join();
    overlay.Cleanup();
    return 0;
}
}
