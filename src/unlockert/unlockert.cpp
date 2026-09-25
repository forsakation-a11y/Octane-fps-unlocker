#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <cmath>

DWORD FindProcessId(const wchar_t* processName)
{
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (!_wcsicmp(entry.szExeFile, processName))
            {
                DWORD pid = entry.th32ProcessID;
                CloseHandle(snapshot);
                return pid;
            }

        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

void UnlockFPS(HANDLE process, double fpsCap)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORY_BASIC_INFORMATION memInfo;

    double target = 1.0 / 60.0;
    double unlocked = (fpsCap <= 0) ? 1.0 / 10000.0 : 1.0 / fpsCap;

    uintptr_t addr = (uintptr_t)sysInfo.lpMinimumApplicationAddress;

    while (addr < (uintptr_t)sysInfo.lpMaximumApplicationAddress)
    {
        if (VirtualQueryEx(process, (LPCVOID)addr, &memInfo, sizeof(memInfo)))
        {
            if (memInfo.State == MEM_COMMIT &&
                (memInfo.Protect & PAGE_READWRITE))
            {
                std::vector<char> buffer(memInfo.RegionSize);
                SIZE_T bytesRead;

                if (ReadProcessMemory(process, (LPCVOID)addr, buffer.data(), memInfo.RegionSize, &bytesRead))
                {
                    for (size_t i = 0; i < bytesRead - sizeof(double); i++)
                    {
                        double value = *(double*)&buffer[i];

                        if (fabs(value - target) < 0.000001)
                        {
                            uintptr_t writeAddr = addr + i;

                            WriteProcessMemory(
                                process,
                                (LPVOID)writeAddr,
                                &unlocked,
                                sizeof(double),
                                NULL
                            );

                            std::cout << "0x"
                                << std::hex << writeAddr << std::endl;
                        }
                    }
                }
            }

            addr += memInfo.RegionSize;
        }
    }

    std::cout << std::dec << "FPS cap set to: "
        << (fpsCap <= 0 ? 10000 : fpsCap)
        << std::endl;
}

int main()
{
    std::cout << "made by @FORSAKATION\n\n";


    std::cout << "client -  type 17+: ";

    std::string input;
    std::cin >> input;

    if (input == "17")
    {
        std::cout << "\ni said write 17+ fuckass\n";
        return 0;
    }

    if (input != "17+")
    {
        std::cout << "Invalid input.\n";
        return 0;
    }

    double fpsCap;

    std::cout << "\nEnter FPS cap (0 = uncapped): ";
    std::cin >> fpsCap;

    std::cout << "\nUnlocker running...\n";

    while (true)
    {
        std::cout << "\nWaiting for process\n";

        DWORD pid = 0;

        while (!pid)
        {
            pid = FindProcessId(L"OctanePlayer.exe");
            Sleep(1000);
        }

        std::cout << "Looking for process... PID = " << pid << std::endl;

        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

        if (!process)
        {
            std::cout << "Failed to open process\n";
            continue;
        }

        UnlockFPS(process, fpsCap);

        std::cout << "Watching process...\n";

        while (true)
        {
            DWORD exitCode;
            GetExitCodeProcess(process, &exitCode);

            if (exitCode != STILL_ACTIVE)
            {
                std::cout << "Client closed.\n";
                CloseHandle(process);
                break;
            }

            Sleep(2000);
        }
    }

    return 0;
}