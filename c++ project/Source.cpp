#define _CRT_SECURE_NO_WARNINGS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include <iostream>
#include <string>

#include "bass/basswasapi.h"
#include "bass/bass.h"
#include <vector>
#include <thread>
#pragma comment(lib, "Winmm.lib")

/*

в коде куча говна, которое при желании вы можете пофиксить
например Sleep(19) находишь, и оно делает минус фпс в гуи (мне похуй). 
создаешь отдельный поток для передачи данных в ком порт и радуешься 60 фпс в гуи

кто дофиксит это говно до красоты тот молодец, мне и так норм

*/

HANDLE hSerial;

bool com_connected = false;
const char* com_error_msg;

bool ConnectCOM() {
    hSerial = ::CreateFileA("COM3", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        com_error_msg = "Serial port is busy";
        return false;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        com_error_msg = "getting state error";
        return false;
    }
    dcbSerialParams.BaudRate = 2000000;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        com_error_msg = "error setting serial port state";
        return false;
    }
    return true;
}

void isOK() {
    DWORD iSize;
    char cReceived;
    while(true) {
        ReadFile(hSerial, &cReceived, 1, &iSize, 0);
        if (iSize > 0 && cReceived == 'k')
            break;
    }
}

bool WriteABuffer(char* lpBuf, DWORD dwToWrite) {
    DWORD dwBytesWritten;
    if (!WriteFile(hSerial, lpBuf, dwToWrite, &dwBytesWritten, NULL)) {
        std::cout << "error write a buffer" << std::endl;
        return false;
    }
    else {
        std::cout << dwToWrite << " Bytes in string. " << dwBytesWritten << " Bytes sended. " << std::endl;
        return true;
    }
}

bool music = false;
bool music_analyzer_type = false;
int bands = 1;
DWORD timer;

// WASAPI callback - not doing anything with the data
DWORD CALLBACK DuffRecording(void* buffer, DWORD length, void* user) {
    return TRUE; // continue recording
}

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX9 Example"), WS_POPUP, 100, 100, 400, 300, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // init BASS
    if (!BASS_Init(0, 44100, 0, hwnd, NULL)) {
        return -1;
    }

    // initialize default loopback input device (or default input device if no loopback)
    if (!BASS_WASAPI_Init(-3, 0, 0, BASS_WASAPI_BUFFER, 1, 0.1, &DuffRecording, NULL)
        && !BASS_WASAPI_Init(-2, 0, 0, BASS_WASAPI_BUFFER, 1, 0.1, &DuffRecording, NULL)) {
        return -1;
    }

    BASS_WASAPI_Start();

    // Main loop

    bool opened = true;

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT && opened)
    {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(400, 300));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGui::Begin("Arduino Hub", &opened, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

        // ауе
        if (com_connected) {
            if (ImGui::Button("Disconnect COM3")) {
                CloseHandle(hSerial);
                com_connected = false;
            }
        }
        else {
            if (ImGui::Button("Connect to COM3")) {
                if (ConnectCOM())
                    com_connected = true;
            }
        }

        // if error message showed
        if (com_error_msg) {
            if (com_connected) com_error_msg = ""; // if success connect - disable msg
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), com_error_msg);
        }

        ImGui::Checkbox("Color music", &music);
        ImGui::Checkbox("Switch type ( work only if bands > 1 )", &music_analyzer_type);
        if (music)
        {
            HDC dc;
            int x, y;

            float fft[1024];
            BASS_WASAPI_GetData(fft, BASS_DATA_FFT2048); // get the FFT data

            int b0 = 0;

            static std::vector<int> toSend;

            for (x = 0; x < bands; x++) {
                float peak = 0;
                int b1 = pow(2, x * 10.0 / (bands - 1));
                if (b1 > 1023) b1 = 1023;
                if (b1 <= b0) b1 = b0 + 1; // make sure it uses at least 1 FFT bin
                for (; b0 < b1; b0++)
                    if (peak < fft[1 + b0]) peak = fft[1 + b0];

                y = (int)(sqrt(peak) * 3 * 255 - 4);
                if (y > 255) y = 255;
                if (y < 0) y = 0;

                toSend.push_back((byte)y);
            }

            int level = BASS_WASAPI_GetLevel();
            int l = LOWORD(level);
            int r = HIWORD(level);

            DWORD dwBytesWritten;
            if (com_connected) {
                Sleep(19); // подобрать наименьшее значение, с которым работает подсветка (я не математик формулы считать вам)
                if (!WriteFile(hSerial, toSend.data(), toSend.size(), &dwBytesWritten, NULL)) {
                    MessageBoxA(0, "cant send COM", "ice morgnehstern", 0);
                    CloseHandle(hSerial);
                    com_connected = false;
                }
            }

            ImGui::Separator();
            ImGui::VSliderInt("##L", ImVec2(18, 80), &l, 0, 65535, "L"); // бас левое ухо
            ImGui::SameLine(); 

            std::vector<float> vecFloat(toSend.begin(), toSend.end());
            if (music_analyzer_type && bands > 1)
                ImGui::PlotLines("##graphic", vecFloat.data(), vecFloat.size(), 0, 0, 0.f, 255.0f, ImVec2(0.f, 80.0f));
            else ImGui::PlotHistogram("##values", vecFloat.data(), vecFloat.size(), 0, NULL, 0.0f, 255.0f, ImVec2(0.f, 80.f));
            vecFloat.clear();

            ImGui::SameLine();
            ImGui::VSliderInt("##R", ImVec2(18, 80), &r, 0, 65535, "R"); // правое ухо басососа
            ImGui::Separator();

            ImGui::SliderInt("Bands", &bands, 1, 128); // кол-во полос (не делайте больше одной, или ардуино умрет от кол-ва передаваемых данных)

            toSend.clear();
        }

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::End();

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    // free BASS.dll
    BASS_WASAPI_Free();
    BASS_Free();

    // shutdown ImGui
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

POINTS m_Pos; // store user click pos


// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        m_Pos = MAKEPOINTS(lParam); // set click points
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (wParam == MK_LBUTTON)
        {
            POINTS p = MAKEPOINTS(lParam); // get cur mousemove click points
            RECT rect;
            GetWindowRect(hWnd, &rect);
            rect.left += p.x - m_Pos.x; // get xDelta
            rect.top += p.y - m_Pos.y;  // get yDelta
            if (m_Pos.x >= 0 && m_Pos.x <= 400 - 20 /* cuz 20px - close btn */ && m_Pos.y >= 0 && m_Pos.y <= ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f)
                SetWindowPos(hWnd, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;
    }
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}