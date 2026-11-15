#include "SDL3/SDL_clipboard.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"
#include "platform/pc/PcGameIcon.hpp"
#include "platform/pc/PcOs.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cassert>
#include <cstdio>
#include <deque>

namespace
{
SDL_Window* s_window;
SDL_GLContext s_glContext;
bool s_fullscreen = false;
bool s_mouseCaptured = false;
std::deque<PcOsWindowEvent> s_events;

enum
{
    kVkBack = 0x08,
    kVkTab = 0x09,
    kVkClear = 0x0C,
    kVkReturn = 0x0D,
    kVkShift = 0x10,
    kVkControl = 0x11,
    kVkMenu = 0x12,
    kVkPause = 0x13,
    kVkCapital = 0x14,
    kVkEscape = 0x1B,
    kVkSpace = 0x20,
    kVkPrior = 0x21,
    kVkNext = 0x22,
    kVkEnd = 0x23,
    kVkHome = 0x24,
    kVkLeft = 0x25,
    kVkUp = 0x26,
    kVkRight = 0x27,
    kVkDown = 0x28,
    kVkSnapshot = 0x2C,
    kVkInsert = 0x2D,
    kVkDelete = 0x2E,
    kVkLWin = 0x5B,
    kVkRWin = 0x5C,
    kVkApps = 0x5D,
    kVkNumpad0 = 0x60,
    kVkMultiply = 0x6A,
    kVkAdd = 0x6B,
    kVkSubtract = 0x6D,
    kVkDecimal = 0x6E,
    kVkDivide = 0x6F,
    kVkF1 = 0x70,
    kVkNumLock = 0x90,
    kVkScroll = 0x91,
    kVkOem1 = 0xBA,
    kVkOemPlus = 0xBB,
    kVkOemComma = 0xBC,
    kVkOemMinus = 0xBD,
    kVkOemPeriod = 0xBE,
    kVkOem2 = 0xBF,
    kVkOem3 = 0xC0,
    kVkOem4 = 0xDB,
    kVkOem5 = 0xDC,
    kVkOem6 = 0xDD,
    kVkOem7 = 0xDE,
    kVkOem102 = 0xE2
};

int SdlToVk(SDL_Keycode key, SDL_Scancode sc)
{
    if (key >= SDLK_A && key <= SDLK_Z)
        return 'A' + (key - SDLK_A);
    if (key >= SDLK_0 && key <= SDLK_9 && sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_0)
        return key;

    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return kVkF1 + (sc - SDL_SCANCODE_F1);
    if (sc >= SDL_SCANCODE_F13 && sc <= SDL_SCANCODE_F24)
        return kVkF1 + 12 + (sc - SDL_SCANCODE_F13);
    if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_9)
        return kVkNumpad0 + 1 + (sc - SDL_SCANCODE_KP_1);

    switch (sc)
    {
    case SDL_SCANCODE_BACKSPACE:
        return kVkBack;
    case SDL_SCANCODE_TAB:
        return kVkTab;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        return kVkReturn;
    case SDL_SCANCODE_ESCAPE:
        return kVkEscape;
    case SDL_SCANCODE_SPACE:
        return kVkSpace;
    case SDL_SCANCODE_PAUSE:
        return kVkPause;
    case SDL_SCANCODE_CAPSLOCK:
        return kVkCapital;
    case SDL_SCANCODE_PAGEUP:
        return kVkPrior;
    case SDL_SCANCODE_PAGEDOWN:
        return kVkNext;
    case SDL_SCANCODE_END:
        return kVkEnd;
    case SDL_SCANCODE_HOME:
        return kVkHome;
    case SDL_SCANCODE_LEFT:
        return kVkLeft;
    case SDL_SCANCODE_UP:
        return kVkUp;
    case SDL_SCANCODE_RIGHT:
        return kVkRight;
    case SDL_SCANCODE_DOWN:
        return kVkDown;
    case SDL_SCANCODE_PRINTSCREEN:
        return kVkSnapshot;
    case SDL_SCANCODE_INSERT:
        return kVkInsert;
    case SDL_SCANCODE_DELETE:
        return kVkDelete;
    case SDL_SCANCODE_LGUI:
        return kVkLWin;
    case SDL_SCANCODE_RGUI:
        return kVkRWin;
    case SDL_SCANCODE_APPLICATION:
        return kVkApps;
    case SDL_SCANCODE_NUMLOCKCLEAR:
        return kVkNumLock;
    case SDL_SCANCODE_SCROLLLOCK:
        return kVkScroll;

    // Win32 WM_KEYDOWN gives generic SHIFT/CTRL/MENU, not L/R
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        return kVkShift;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
        return kVkControl;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
        return kVkMenu;

    case SDL_SCANCODE_KP_0:
        return kVkNumpad0;
    case SDL_SCANCODE_KP_MULTIPLY:
        return kVkMultiply;
    case SDL_SCANCODE_KP_PLUS:
        return kVkAdd;
    case SDL_SCANCODE_KP_MINUS:
        return kVkSubtract;
    case SDL_SCANCODE_KP_PERIOD:
        return kVkDecimal;
    case SDL_SCANCODE_KP_DIVIDE:
        return kVkDivide;

    case SDL_SCANCODE_SEMICOLON:
        return kVkOem1;
    case SDL_SCANCODE_EQUALS:
        return kVkOemPlus;
    case SDL_SCANCODE_COMMA:
        return kVkOemComma;
    case SDL_SCANCODE_MINUS:
        return kVkOemMinus;
    case SDL_SCANCODE_PERIOD:
        return kVkOemPeriod;
    case SDL_SCANCODE_SLASH:
        return kVkOem2;
    case SDL_SCANCODE_GRAVE:
        return kVkOem3;
    case SDL_SCANCODE_LEFTBRACKET:
        return kVkOem4;
    case SDL_SCANCODE_BACKSLASH:
        return kVkOem5;
    case SDL_SCANCODE_RIGHTBRACKET:
        return kVkOem6;
    case SDL_SCANCODE_APOSTROPHE:
        return kVkOem7;
    case SDL_SCANCODE_NONUSHASH:
        return kVkOem7;
    case SDL_SCANCODE_NONUSBACKSLASH:
        return kVkOem102;
    default:
        return 0;
    }
}

PcOsWindowEvent MakeEvent(PcOsWindowEventType type)
{
    PcOsWindowEvent event;
    event.type = type;
    event.key = 0;
    event.down = false;
    event.x = 0;
    event.y = 0;
    event.delta = 0;
    event.character = 0;
    return event;
}
void PushMouseButton(PcOsMouseButton button, bool down, SDL_MouseButtonEvent* sdl_event)
{
    PcOsWindowEvent event = MakeEvent(ePcOsWindowEvent_MouseButton);
    event.key = button;
    event.down = down;
    event.x = sdl_event->x;
    event.y = sdl_event->y;
    s_events.push_back(event);
}

void AddTextEvent(unsigned int character)
{
    PcOsWindowEvent event = MakeEvent(ePcOsWindowEvent_Text);
    event.character = character;
    s_events.push_back(event);
}

bool EmitControlChar(const SDL_KeyboardEvent& k)
{
    bool ctrl = (k.mod & SDL_KMOD_CTRL) != 0;
    bool alt = (k.mod & SDL_KMOD_ALT) != 0;
    SDL_Keycode key = k.key;

    if (ctrl && !alt)
    {
        if (key == SDLK_BACKSPACE)
        {
            AddTextEvent(0x7F);
            return true;
        }
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
        {
            AddTextEvent(0x0A);
            return true;
        }
        if (key >= SDLK_A && key <= SDLK_Z)
        {
            AddTextEvent(1 + (key - SDLK_A));
            return true;
        }
        if (key == SDLK_LEFTBRACKET)
        {
            AddTextEvent(0x1B);
            return true;
        }
        if (key == SDLK_BACKSLASH)
        {
            AddTextEvent(0x1C);
            return true;
        }
        if (key == SDLK_RIGHTBRACKET)
        {
            AddTextEvent(0x1D);
            return true;
        }
        return false;
    }
    switch (key)
    {
    case SDLK_BACKSPACE:
        AddTextEvent(0x08);
        return true;
    case SDLK_TAB:
        AddTextEvent(0x09);
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        AddTextEvent(0x0D);
        return true;
    case SDLK_ESCAPE:
        AddTextEvent(0x1B);
        return true;
    default:
        return false;
    }
}

void HandleEvent(SDL_Event* sdl_event)
{
    switch (sdl_event->type)
    {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        s_events.push_back(MakeEvent(ePcOsWindowEvent_Close));
        return;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: // TODO: Some weird unicode conversion going on here, could probably change APIs using the characters to UTF-8 instead
    {
        PcOsWindowEvent event = MakeEvent(ePcOsWindowEvent_Key);
        event.key = SdlToVk(sdl_event->key.key,
                            sdl_event->key.scancode); // FIXME: Should change code to use SDL codes not VK
        event.down = sdl_event->key.down;
        s_events.push_back(event);
        bool firstPress = event.down && !sdl_event->key.repeat;
        bool isEnter = (sdl_event->key.key == SDLK_RETURN || sdl_event->key.key == SDLK_KP_ENTER);
        bool isAltPressed = (sdl_event->key.mod & SDL_KMOD_ALT) != 0;
        bool altEnter = isEnter && isAltPressed;

        if (firstPress && (sdl_event->key.key == SDLK_F11 || altEnter))
        {
            PcOsWindowSetFullscreen(!s_fullscreen);
            return;
        }
        /*if (wParam == VK_F10 || wParam == VK_MENU) // Not sure this would have any effect under SDL
            return;*/
        // Control keys that SDL won't put in text input
        if (event.down)
        {
            EmitControlChar(sdl_event->key);
        }

        return;
    }
    case SDL_EVENT_TEXT_INPUT:
    {
        const char* text = sdl_event->text.text;
        while (text)
        {
            Uint32 cp;
            int len;

            if ((text[0] & 0x80) == 0x00)
            {
                cp = text[0];
                len = 1;
            }
            else if ((text[0] & 0xE0) == 0xC0)
            {
                cp = ((text[0] & 0x1F) << 6) | (text[1] & 0x3F);
                len = 2;
            }
            else if ((text[0] & 0xF0) == 0xE0)
            {
                cp = ((text[0] & 0x0F) << 12) | ((text[1] & 0x3F) << 6) | (text[2] & 0x3F);
                len = 3;
            }
            else
            {
                cp = ((text[0] & 0x07) << 18) | ((text[1] & 0x3F) << 12) | ((text[2] & 0x3F) << 6) | (text[3] & 0x3F);
                len = 4;
            }
            if (cp <= 0xFFFF)
            {
                AddTextEvent(static_cast<unsigned int>(cp));
            }
            else
            {
                cp -= 0x10000;
                unsigned int hi = static_cast<unsigned int>(0xD800 + (cp >> 10));
                unsigned int lo = static_cast<unsigned int>(0xDC00 + (cp & 0x3FF));
                AddTextEvent(hi);
                AddTextEvent(lo);
            }
        }
        return;
    }
    case SDL_EVENT_MOUSE_MOTION:
    {
        PcOsWindowEvent event = MakeEvent(ePcOsWindowEvent_MouseMove);
        if (s_mouseCaptured)
        {
            event.x = sdl_event->motion.xrel;
            event.y = sdl_event->motion.yrel;
        }
        else
        {
            event.x = sdl_event->motion.x;
            event.y = sdl_event->motion.y;
        }
        s_events.push_back(event);
        return;
    }
        /*    case WM_INPUT:
    {
        RAWINPUT input;
        UINT size = sizeof(input);
        if (s_mouseCaptured
            && GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER))
                   != static_cast<UINT>(-1)
            && input.header.dwType == RIM_TYPEMOUSE && (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
            PcOsWindowEvent event = MakeEvent(ePcOsWindowEvent_MouseMove);
            event.x = input.data.mouse.lLastX;
            event.y = input.data.mouse.lLastY;
            event.down = true;
            s_events.push_back(event);
            RecentreCursor();
        }
        break;
    }*/
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        PcOsMouseButton button;
        switch (sdl_event->button.button)
        {
        case SDL_BUTTON_LEFT:
            button = ePcOsMouseButton_Left;
            break;
        case SDL_BUTTON_RIGHT:
            button = ePcOsMouseButton_Right;
            break;
        case SDL_BUTTON_MIDDLE:
            button = ePcOsMouseButton_Middle;
            break;
        case SDL_BUTTON_X1:
            button = ePcOsMouseButton_X1;
            break;
        case SDL_BUTTON_X2:
            button = ePcOsMouseButton_X2;
            break;
        default:
            return;
        }

        PushMouseButton(button, sdl_event->type == SDL_EVENT_MOUSE_BUTTON_DOWN, &sdl_event->button);
        return;
    }

    case SDL_EVENT_MOUSE_WHEEL:
    {
        PcOsWindowEvent event = MakeEvent(ePcOsWindowEvent_MouseWheel);
        event.delta = sdl_event->wheel.y * 120; // FIXME:Replace data with exact SDL value, especially for trackpads
        s_events.push_back(event);
        return;
    }
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    {
        PcOsWindowEvent event = MakeEvent(ePcOsWindowEvent_Focus);
        event.down = sdl_event->type != SDL_EVENT_WINDOW_FOCUS_LOST;
        s_events.push_back(event);
        break;
    }
    /*case WM_SIZE:
    case WM_MOVE:
        if (s_mouseCaptured)
            ConfineCursor();
        break;
    case WM_SETCURSOR:
        if ((s_mouseCaptured || s_cursorHidden) && LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(0);
            return TRUE;
        }
        break;*/
    default:
        break;
    }
}
bool CreateGlContext()
{
    if (s_window == nullptr)
    {
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    s_glContext = SDL_GL_CreateContext(s_window);
    if (!s_glContext)
    {
        printf("Failed to create GLContext: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_GL_MakeCurrent(s_window, s_glContext))
    {
        printf("Failed to make OpenGL context current: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetSwapInterval(1);

    return true;
}
// TODO: Probably better to not use wstring with SDL
std::wstring utf8_to_wstring(const char* utf8_str)
{
    if (!utf8_str)
        return {};

    wchar_t* w = (wchar_t*)SDL_iconv_string("WCHAR_T", "UTF-8", utf8_str, SDL_strlen(utf8_str) + 1);
    if (!w)
        return {};

    std::wstring result(w);
    SDL_free(w);
    return result;
}
}
bool PcOsWindowCreate(int width, int height, const char* title, bool visible, bool maximized)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    s_window = SDL_CreateWindow(title, width, height,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | (maximized ? SDL_WINDOW_MAXIMIZED : 0));
    if (s_window == nullptr)
    {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    if (!SDL_StartTextInput(s_window))
    {
        printf("SDL_StartTextInput failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_IOStream* icon_io = SDL_IOFromConstMem(game_icon, game_icon_len);
    if (!icon_io)
    { // This should never happen, said every programmer ever
        printf("Failed to create IOStream from memory: %s\n", SDL_GetError());
    }

    SDL_Surface* icon = IMG_Load_IO(icon_io, true);
    if (!icon)
    {
        printf("Failed to load embedded icon: %s\n", SDL_GetError());
    }
    if (icon && !SDL_SetWindowIcon(s_window, icon))
    {
        printf("Failed to set window icon: %s\n", SDL_GetError());
    }
    if (icon)
    {
        SDL_DestroySurface(icon);
    }

    if (!CreateGlContext())
    {
        SDL_Quit();
        return false;
    }
    if (visible)
    {
        SDL_ShowWindow(s_window);
        SDL_UpdateWindowSurface(s_window);
    }
    return true;
}
bool PcOsWindowPollEvent(PcOsWindowEvent* event)
{
    if (s_events.empty())
    {
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            HandleEvent(&sdl_event);
        }
    }
    if (s_events.empty())
        return false;
    *event = s_events.front();
    s_events.pop_front();
    return true;
}

void PcOsWindowSwapBuffers()
{
    SDL_GL_SwapWindow(s_window);
}

void PcOsWindowGetClientSize(int* width, int* height)
{
    SDL_GetWindowSizeInPixels(s_window, width, height);
}

void PcOsWindowSetMouseCaptured(bool captured)
{
    SDL_SetWindowRelativeMouseMode(s_window, captured);
    s_mouseCaptured = captured;
}

void PcOsWindowSetFullscreen(bool fullscreen)
{
    if (s_window == nullptr || fullscreen == s_fullscreen)
        return;
    SDL_SetWindowFullscreen(s_window, fullscreen);
    s_fullscreen = fullscreen;
}

void PcOsWindowSetCursorHidden(bool hidden)
{
    if (hidden)
    {
        SDL_HideCursor();
    }
    else
    {
        SDL_ShowCursor();
    }
}

void PcOsWindowSetSwapInterval(int interval)
{
    SDL_GL_SetSwapInterval(interval);
}

std::wstring PcOsClipboardText()
{
    char* clipboard = SDL_GetClipboardText();
    std::wstring clipboard_w = utf8_to_wstring(clipboard);
    SDL_free(clipboard);
    return clipboard_w;
}

void* PcOsGlGetProcAddress(const char* name)
{
    SDL_FunctionPointer func = SDL_GL_GetProcAddress(name);
    void* voidPtr = reinterpret_cast<void*>(func); // FIXME: Should not require casting
    return voidPtr;
}
