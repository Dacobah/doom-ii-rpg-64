#ifndef __N64_SDL_COMPAT_H__
#define __N64_SDL_COMPAT_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;

typedef int8_t   Sint8;
typedef int16_t  Sint16;
typedef int32_t  Sint32;
typedef int64_t  Sint64;

#define SDL_arraysize(array) (sizeof(array) / sizeof((array)[0]))
#define SDL_memcpy memcpy
#define SDL_strcasecmp strcasecmp

// N64 is Big-Endian (MIPS): swap bytes when reading Little-Endian data
#define SDL_SwapLE16(x) __builtin_bswap16((uint16_t)(x))
#define SDL_SwapLE32(x) __builtin_bswap32((uint32_t)(x))
#define SDL_SwapBE16(x) ((uint16_t)(x))
#define SDL_SwapBE32(x) ((uint32_t)(x))

typedef enum {
    SDL_FALSE = 0,
    SDL_TRUE = 1
} SDL_bool;

typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;
typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Joystick SDL_Joystick;
typedef void SDL_Haptic;
typedef int SDL_JoystickID;

// Minimal event structures
typedef struct SDL_CommonEvent {
    Uint32 type;
    Uint32 timestamp;
} SDL_CommonEvent;

typedef struct SDL_KeyboardEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 state;
    Uint8 repeat;
    Uint8 padding2;
    Uint8 padding3;
    struct {
        int scancode;
        int sym;
        Uint16 mod;
        Uint32 unused;
    } keysym;
} SDL_KeyboardEvent;

typedef struct SDL_MouseMotionEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint32 state;
    Sint32 x;
    Sint32 y;
    Sint32 xrel;
    Sint32 yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 clicks;
    Uint8 padding1;
    Sint32 x;
    Sint32 y;
} SDL_MouseButtonEvent;

typedef struct SDL_ControllerAxisEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 axis;
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint16 value;
    Uint16 padding4;
} SDL_ControllerAxisEvent;

typedef struct SDL_ControllerButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 button;
    Uint8 state;
    Uint8 padding1;
    Uint8 padding2;
} SDL_ControllerButtonEvent;

typedef struct SDL_JoyAxisEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 axis;
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint16 value;
    Uint16 padding4;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 button;
    Uint8 state;
    Uint8 padding1;
    Uint8 padding2;
} SDL_JoyButtonEvent;

typedef struct SDL_JoyDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
} SDL_JoyDeviceEvent;

typedef struct SDL_ControllerDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
} SDL_ControllerDeviceEvent;

typedef union SDL_Event {
    Uint32 type;
    SDL_CommonEvent common;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_JoyAxisEvent jaxis;
    SDL_JoyButtonEvent jbutton;
    SDL_JoyDeviceEvent jdevice;
    SDL_ControllerAxisEvent caxis;
    SDL_ControllerButtonEvent cbutton;
    SDL_ControllerDeviceEvent cdevice;
} SDL_Event;

// Event types
enum {
    SDL_QUIT = 0x100,
    SDL_WINDOWEVENT = 0x200,
    SDL_KEYDOWN = 0x300,
    SDL_KEYUP,
    SDL_MOUSEMOTION = 0x400,
    SDL_MOUSEBUTTONDOWN,
    SDL_MOUSEBUTTONUP,
    SDL_JOYAXISMOTION = 0x600,
    SDL_JOYBUTTONDOWN = 0x603,
    SDL_JOYBUTTONUP,
    SDL_JOYDEVICEADDED,
    SDL_JOYDEVICEREMOVED,
    SDL_CONTROLLERAXISMOTION = 0x650,
    SDL_CONTROLLERBUTTONDOWN,
    SDL_CONTROLLERBUTTONUP,
    SDL_CONTROLLERDEVICEADDED,
    SDL_CONTROLLERDEVICEREMOVED,
    SDL_CONTROLLERDEVICEREMAPPED
};

#define SDL_BUTTON_LMASK 1

// Controller axes and buttons
enum {
    SDL_CONTROLLER_AXIS_LEFTX = 0,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT
};

enum {
    SDL_CONTROLLER_BUTTON_A = 0,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

// Scancodes
typedef enum {
    SDL_SCANCODE_UNKNOWN = 0,
    SDL_SCANCODE_A = 4,
    SDL_SCANCODE_B = 5,
    SDL_SCANCODE_C = 6,
    SDL_SCANCODE_D = 7,
    SDL_SCANCODE_I = 12,
    SDL_SCANCODE_O = 18,
    SDL_SCANCODE_P = 19,
    SDL_SCANCODE_S = 22,
    SDL_SCANCODE_W = 26,
    SDL_SCANCODE_X = 27,
    SDL_SCANCODE_Z = 29,
    SDL_SCANCODE_1 = 30,
    SDL_SCANCODE_0 = 39,
    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80,
    SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,
    SDL_SCANCODE_KP_1 = 89,
    SDL_SCANCODE_KP_0 = 98,
    SDL_SCANCODE_F1 = 58,
    SDL_NUM_SCANCODES = 512
} SDL_Scancode;

// MessageBox stubs
typedef struct SDL_MessageBoxButtonData {
    Uint32 flags;
    int buttonid;
    const char* text;
} SDL_MessageBoxButtonData;

typedef struct SDL_MessageBoxColor {
    Uint8 r, g, b;
} SDL_MessageBoxColor;

typedef struct SDL_MessageBoxColorScheme {
    SDL_MessageBoxColor colors[5];
} SDL_MessageBoxColorScheme;

typedef struct SDL_MessageBoxData {
    Uint32 flags;
    SDL_Window* window;
    const char* title;
    const char* message;
    int numbuttons;
    const SDL_MessageBoxButtonData* buttons;
    const SDL_MessageBoxColorScheme* colorScheme;
} SDL_MessageBoxData;

#define SDL_MESSAGEBOX_ERROR 0x00000010

// Stubs for functions
static inline int SDL_Init(Uint32 flags) { (void)flags; return 0; }
static inline void SDL_Quit(void) {}
static inline const char* SDL_GetError(void) { return ""; }
#ifdef N64
#include <libdragon.h>
static inline Uint32 SDL_GetTicks(void) {
    return (Uint32)get_ticks_ms();
}
#else
static inline Uint32 SDL_GetTicks(void) { return 0; }
#endif
static inline int SDL_PollEvent(SDL_Event* event) { (void)event; return 0; }
static inline void SDL_GetWindowSize(SDL_Window* window, int* w, int* h) {
    (void)window;
    if (w) *w = 320;
    if (h) *h = 240;
}
static inline int SDL_ShowMessageBox(const SDL_MessageBoxData* data, int* buttonid) {
    (void)data; (void)buttonid;
    return 0;
}
static inline const char* SDL_GetScancodeName(SDL_Scancode scancode) {
    (void)scancode;
    return "KEY";
}
static inline void dummy_glActiveTexture(uint32_t texture) { (void)texture; }
static inline void* SDL_GL_GetProcAddress(const char* proc) {
    (void)proc;
    return (void*)dummy_glActiveTexture;
}
static inline int SDL_SetWindowFullscreen(SDL_Window* window, Uint32 flags) {
    (void)window;
    (void)flags;
    return 0;
}
static inline int SDL_JoystickNumAxes(SDL_Joystick* joystick) { (void)joystick; return 0; }
static inline void SDL_GameControllerClose(SDL_GameController* controller) { (void)controller; }
static inline void SDL_JoystickClose(SDL_Joystick* joystick) { (void)joystick; }
static inline void SDL_HapticClose(SDL_Haptic* haptic) { (void)haptic; }
static inline int SDL_JoystickGetAttached(SDL_Joystick* joystick) { (void)joystick; return 0; }
static inline int SDL_NumJoysticks(void) { return 0; }
static inline int SDL_IsGameController(int joyIdx) { (void)joyIdx; return 0; }
static inline SDL_GameController* SDL_GameControllerOpen(int joyIdx) { (void)joyIdx; return NULL; }
static inline SDL_Joystick* SDL_GameControllerGetJoystick(SDL_GameController* controller) { (void)controller; return NULL; }
static inline SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick* joystick) { (void)joystick; return 0; }
static inline int SDL_GameControllerHasRumble(SDL_GameController* controller) { (void)controller; return 0; }
static inline SDL_Joystick* SDL_JoystickOpen(int joyIdx) { (void)joyIdx; return NULL; }
static inline int SDL_JoystickIsHaptic(SDL_Joystick* joystick) { (void)joystick; return 0; }
static inline SDL_Haptic* SDL_HapticOpenFromJoystick(SDL_Joystick* joystick) { (void)joystick; return NULL; }
static inline int SDL_HapticRumbleInit(SDL_Haptic* haptic) { (void)haptic; return 0; }
static inline int SDL_GameControllerRumble(SDL_GameController* controller, Uint16 low, Uint16 high, Uint32 duration) { (void)controller; (void)low; (void)high; (void)duration; return 0; }
static inline int SDL_HapticRumbleStop(SDL_Haptic* haptic) { (void)haptic; return 0; }
static inline int SDL_HapticRumblePlay(SDL_Haptic* haptic, float strength, Uint32 length) { (void)haptic; (void)strength; (void)length; return 0; }
static inline Uint32 SDL_GetMouseState(int* x, int* y) { if (x) *x = 0; if (y) *y = 0; return 0; }
static inline void SDL_GL_SwapWindow(SDL_Window* window) { (void)window; }

#ifdef __cplusplus
}
#endif

#endif // __N64_SDL_COMPAT_H__
