#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>

namespace {
constexpr const wchar_t* kDefaultProcess = L"RobloxPlayerBeta.exe";

DWORD FindPidByName(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    DWORD found = 0;
    if (Process32FirstW(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, name) == 0) {
                found = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
    return found;
}

bool InjectDll(DWORD pid, const std::wstring& dllPath) {
    HANDLE hProc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!hProc) {
        wprintf(L"[!] OpenProcess failed: %lu\n", GetLastError());
        return false;
    }

    const SIZE_T sz = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProc, nullptr, sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        wprintf(L"[!] VirtualAllocEx failed: %lu\n", GetLastError());
        CloseHandle(hProc);
        return false;
    }

    SIZE_T written = 0;
    if (!WriteProcessMemory(hProc, remoteMem, dllPath.c_str(), sz, &written) || written != sz) {
        wprintf(L"[!] WriteProcessMemory failed: %lu\n", GetLastError());
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel) {
        wprintf(L"[!] GetModuleHandle(kernel32) failed\n");
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }
    LPTHREAD_START_ROUTINE loadLib =
        reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(hKernel, "LoadLibraryW"));
    if (!loadLib) {
        wprintf(L"[!] GetProcAddress(LoadLibraryW) failed\n");
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, loadLib, remoteMem, 0, nullptr);
    if (!hThread) {
        wprintf(L"[!] CreateRemoteThread failed: %lu\n", GetLastError());
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    wprintf(L"[+] Remote thread created, waiting for LoadLibraryW to return...\n");
    WaitForSingleObject(hThread, 10000);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProc);

    if (exitCode == 0) {
        wprintf(L"[!] LoadLibraryW returned NULL — DLL failed to load (bad path, arch mismatch, or blocked by anti-cheat)\n");
        return false;
    }
    wprintf(L"[+] DLL loaded at remote HMODULE 0x%llX\n", static_cast<unsigned long long>(exitCode));
    return true;
}

void PrintUsage(const wchar_t* self) {
    wprintf(L"usage: %s <path-to-dll> [process-name]\n", self);
    wprintf(L"  default process: RobloxPlayerBeta.exe\n");
    wprintf(L"  DLL must be x64.\n");
}
}

int wmain(int argc, wchar_t** argv) {
    SetConsoleTitleW(L"jatos injector");
    wprintf(L"jatos injector\n");
    wprintf(L"------------------------------------------------------------\n");
    wprintf(L"NOTE: Roblox with Hyperion/Byfron blocks CreateRemoteThread\n");
    wprintf(L"      injection. This will fail on protected clients. Use\n");
    wprintf(L"      only against unprotected processes or after a bypass.\n");
    wprintf(L"------------------------------------------------------------\n\n");

    if (argc < 2) {
        PrintUsage(argv[0]);
        system("pause");
        return 1;
    }

    std::filesystem::path dllPath{argv[1]};
    std::error_code ec;
    dllPath = std::filesystem::absolute(dllPath, ec);
    if (ec || !std::filesystem::exists(dllPath)) {
        wprintf(L"[!] DLL not found: %s\n", dllPath.c_str());
        system("pause");
        return 2;
    }
    wprintf(L"[*] DLL: %s\n", dllPath.c_str());

    const wchar_t* procName = (argc >= 3) ? argv[2] : kDefaultProcess;
    wprintf(L"[*] Target: %s\n", procName);

    DWORD pid = FindPidByName(procName);
    if (!pid) {
        wprintf(L"[*] Process not running. Waiting...\n");
        while (!pid) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            pid = FindPidByName(procName);
        }
    }
    wprintf(L"[+] PID %lu\n", pid);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    const bool ok = InjectDll(pid, dllPath.wstring());
    if (!ok) {
        wprintf(L"\n[!] Injection failed.\n");
        system("pause");
        return 3;
    }
    wprintf(L"\n[+] Injection succeeded.\n");
    system("pause");
    return 0;
}
