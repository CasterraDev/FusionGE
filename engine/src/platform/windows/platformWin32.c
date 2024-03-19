#include "platform/platform.h"

//Windows platform layer.
#if FSNPLATFORM_WINDOWS

#include "core/logger.h"
#include "core/input.h"
#include "helpers/dinoArray.h"
#include "renderer/vulkan/vulkanPlatform.h"
#include "core/event.h"

#include <windows.h>
#include <windowsx.h>  //param input extraction
#include <stdlib.h>

// For surface creation
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "renderer/vulkan/vulkanHeader.h"

typedef struct platformState {
    HINSTANCE hInstance; //A handle to the instance that contains the window procedure for the class.
    HWND hwnd;
    VkSurfaceKHR surface;
    f64 clockFreq;
    LARGE_INTEGER startTime;
} platformState;

static platformState* systemPtr;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

b8 platformStartup(
    u64* memoryRequirement,
    void* state,
    const char *appName,
    i32 x,
    i32 y,
    i32 width,
    i32 height) {
    *memoryRequirement = sizeof(platformState);
    if (state == 0){
        return true;
    }
    systemPtr = state;
    systemPtr->hInstance = GetModuleHandleA(0);

    //Setup and register window class.
    HICON icon = LoadIcon(systemPtr->hInstance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc)); //Zero out the memory
    wc.style = CS_DBLCLKS;  //Get double-clicks
    wc.lpfnWndProc = win32_process_message;//Pointer to Window Procedure
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = systemPtr->hInstance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  //NULL; // Manage the cursor manually
    wc.hbrBackground = NULL;                   //Transparent
    wc.lpszClassName = "Fusion_window_class";

    //If anything fails
    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // NOTE: Theres two area in a window
    // The "window" area which is the whole entire window including the app name, menus(File, edit, view) and border
    // And the "client" area which is the area the a the user will be able to interact with and manipulate.

    //Create window
    u32 client_x = x;
    u32 client_y = y;
    u32 client_width = width;
    u32 client_height = height;

    //TODO: Make the window a tiny bit bigger to include the border
    u32 window_x = client_x;
    u32 window_y = client_y;
    u32 window_width = client_width;
    u32 window_height = client_height;

    //Windows Styling
    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    //Obtain the size of the border.
    RECT border_rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    //In this case, the border rectangle is negative.
    window_x += border_rect.left;
    window_y += border_rect.top;

    //Grow by the size of the OS border.
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    //Create the window
    HWND handle = CreateWindowExA(
        window_ex_style, "Fusion_window_class", appName,
        window_style, window_x, window_y, window_width, window_height,
        0, 0, systemPtr->hInstance, 0);

    if (handle == 0) {
        MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        FFATAL("Window creation failed!");
        return false;
    } else {
        systemPtr->hwnd = handle;
    }

    //Show the window
    b32 should_activate = 1;  // TODO: if the window should not accept input, this should be false.
    i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    //If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    //If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(systemPtr->hwnd, show_window_command_flags);

    //Clock setup
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    systemPtr->clockFreq = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&systemPtr->startTime);

    return true;
}

void platformShutdown() {
    if (systemPtr && systemPtr->hwnd) {
        DestroyWindow(systemPtr->hwnd);
        systemPtr->hwnd = 0;
    }
}

b8 platformPumpMessages() {
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return true;
}

void *platformAllocate(u64 size, b8 aligned) {
    return malloc(size);
}

void platformFree(void *block, b8 aligned) {
    free(block);
}

void *platformZeroMemory(void *block, u64 size) {
    return memset(block, 0, size);
}

void *platformCopyMemory(void *dest, const void *source, u64 size) {
    return memcpy(dest, source, size);
}

void *platformSetMemory(void *dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

void platformConsoleWrite(const char *message, u8 color) {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[color]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
}

void platformConsoleWriteError(const char *message, u8 color) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    //FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[color]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

f64 platformGetAbsoluteTime() {
    if (systemPtr && !systemPtr->clockFreq){
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        systemPtr->clockFreq = 1.0 / (f64)frequency.QuadPart;
        QueryPerformanceCounter(&systemPtr->startTime);
    }
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * systemPtr->clockFreq;
}

void platformSleep(u64 ms) {
    Sleep(ms);
}

void platformGetRequiredExts(const char*** array){
    dinoPush(*array,&"VK_KHR_win32_surface");
}

// Surface creation for Vulkan
b8 platformCreateVulkanSurface(vulkanHeader *header) {
    VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    createInfo.hinstance = systemPtr->hInstance;
    createInfo.hwnd = systemPtr->hwnd;

    VkResult result = vkCreateWin32SurfaceKHR(header->instance, &createInfo, header->allocator, &systemPtr->surface);
    if (result != VK_SUCCESS) {
        FFATAL("Vulkan surface creation failed.");
        return false;
    }

    header->surface = systemPtr->surface;
    return true;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_ERASEBKGND:
            //Notify the OS that erasing will be handled by the application to prevent flicker.
            return 1;
        case WM_CLOSE:
            eventContext data = {};
            eventFire(EVENT_CODE_APPLICATION_QUIT,0,data);
            return true;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            //Get the updated size.
            RECT r;
            GetClientRect(hwnd, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;

            //Fire an event. the app layer should pick this up, but not handle it
            // as it shouldnt be visible to other parts of the app
            eventContext ec;
            ec.data.u16[0] = (u16)width;
            ec.data.u16[1] = (u16)height;
            eventFire(EVENT_CODE_RESIZED, 0, ec);
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            //Key pressed/released
            b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            keys key = (u16)w_param;

            if (w_param == VK_MENU){ //Alt key
                if (GetKeyState(VK_RMENU) & 0x8000){
                    key = KEY_RALT;
                }else if (GetKeyState(VK_LMENU) & 0x8000){
                    key = KEY_LALT;
                }
            }else if (w_param == VK_SHIFT){ //Shift key
                if (GetKeyState(VK_RSHIFT) & 0x8000){
                    key = KEY_RSHIFT;
                }else if (GetKeyState(VK_LSHIFT) & 0x8000){
                    key = KEY_LSHIFT;
                }
            }else if (w_param == VK_CONTROL){ //Ctrl key
                if (GetKeyState(VK_RCONTROL) & 0x8000){
                    key = KEY_RCONTROL;
                }else if (GetKeyState(VK_LCONTROL) & 0x8000){
                    key = KEY_RCONTROL;
                }
            }

            inputProcessKey(key,pressed);
        } break;
        case WM_MOUSEMOVE: {
            //Mouse move
            i32 xPos = GET_X_LPARAM(l_param);
            i32 yPos = GET_Y_LPARAM(l_param);

            inputProcessMouseMove(xPos,yPos);
        } break;
        case WM_MOUSEWHEEL: {
            i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (z_delta != 0) {
                // Flatten the input to an OS-independent (-1, 1)
                z_delta = (z_delta < 0) ? -1 : 1;
                inputProcessMouseWheel(z_delta);
            }
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            b8 pressed = true;
            buttons mouseBtn = BUTTON_MAX_BUTTONS;
            switch (msg) {
                case WM_LBUTTONDOWN:
                    mouseBtn = BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                    mouseBtn = BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                    mouseBtn = BUTTON_RIGHT;
                    break;
            }
            if (mouseBtn != BUTTON_MAX_BUTTONS){
                inputProcessButton(mouseBtn,pressed);
            }
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            b8 pressed = false;
            buttons mouseBtn = BUTTON_MAX_BUTTONS;
            switch (msg) {
                case WM_LBUTTONUP:
                    mouseBtn = BUTTON_LEFT;
                    break;
                case WM_MBUTTONUP:
                    mouseBtn = BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONUP:
                    mouseBtn = BUTTON_RIGHT;
                    break;
            }
            if (mouseBtn != BUTTON_MAX_BUTTONS){
                inputProcessButton(mouseBtn,pressed);
            }
        } break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif //FSNPLATFORM_WINDOWS