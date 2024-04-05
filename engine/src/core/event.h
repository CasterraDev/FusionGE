#pragma once

#include "defines.h"

typedef struct eventContext {
    // 128 bytes
    union {
        /** @brief An array of 2 64-bit signed integers. */
        i64 i64[2];
        /** @brief An array of 2 64-bit unsigned integers. */
        u64 u64[2];

        /** @brief An array of 2 64-bit floating-point numbers. */
        f64 f64[2];

        /** @brief An array of 4 32-bit signed integers. */
        i32 i32[4];
        /** @brief An array of 4 32-bit unsigned integers. */
        u32 u32[4];
        /** @brief An array of 4 32-bit floating-point numbers. */
        f32 f32[4];

        /** @brief An array of 8 16-bit signed integers. */
        i16 i16[8];

        /** @brief An array of 8 16-bit unsigned integers. */
        u16 u16[8];

        /** @brief An array of 16 8-bit signed integers. */
        i8 i8[16];
        /** @brief An array of 16 8-bit unsigned integers. */
        u8 u8[16];

        /** @brief An array of 16 characters. */
        char c[16];
    } data;
} eventContext;

/**
 * @brief A function pointer typedef which is used for event subscriptions by the subscriber.
 * @param eventCode The event code to be sent.
 * @param sender A pointer to the sender of the event. Can be 0/NULL.
 * @param listenerInstance A pointer to the listener of the event. Can be 0/NULL.
 * @param data The event context to pass to the event that was fired.
 * @returns True if the message is considered handled otherwise false.
 */
typedef b8 (*PF_on_event)(u16 eventCode, void* sender, void* listenerInstance, eventContext data);

b8 eventInit(u64* memoryRequirement, void* state);
void eventShutdown();

/**
 * Register to listen for when events are sent with the provided code. Events with duplicate
 * listener/callback combos will not be registered again and will cause this to return false.
 * @param code The event code to listen for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function pointer to be invoked when the event code is fired.
 * @returns true if the event is successfully registered; otherwise false.
 */
FSNAPI b8 eventRegister(u16 code, void* listener, PF_on_event on_event);

/**
 * Unregister from listening for when events are sent with the provided code. If no matching
 * registration is found, this function returns false.
 * @param code The event code to stop listening for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function pointer to be unregistered.
 * @returns true if the event is successfully unregistered; otherwise false.
 */
FSNAPI b8 eventUnregister(u16 code, void* listener, PF_on_event on_event);

/**
 * Fires an event to listeners of the given code. If an event handler returns 
 * true, the event is considered handled and is not passed on to any more listeners.
 * @param code The event code to fire.
 * @param sender A pointer to the sender. Can be 0/NULL.
 * @param data The event data.
 * @returns true if handled, otherwise false.
 */
FSNAPI b8 eventFire(u16 code, void* sender, eventContext context);


typedef enum system_event_code {
    /** @brief Shuts the application down on the next frame. */
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    /** @brief Keyboard key pressed.
     * Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_PRESSED = 0x02,

    /** @brief Keyboard key released.
     * Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_RELEASED = 0x03,

    /** @brief Mouse button pressed.
     * Context usage:
     * u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_PRESSED = 0x04,

    /** @brief Mouse button released.
     * Context usage:
     * u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_RELEASED = 0x05,

    /** @brief Mouse moved.
     * Context usage:
     * u16 x = data.data.i16[0];
     * u16 y = data.data.i16[1];
     */
    EVENT_CODE_MOUSE_MOVED = 0x06,

    /** @brief Mouse moved.
     * Context usage:
     * ui z_delta = data.data.i8[0];
     */
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    /** @brief Resized/resolution changed from the OS.
     * Context usage:
     * u16 width = data.data.u16[0];
     * u16 height = data.data.u16[1];
     */
    EVENT_CODE_RESIZED = 0x08,

    // Change the render mode for debugging purposes.
    /* Context usage:
     * i32 mode = context.data.i32[0];
     */
    EVENT_CODE_SET_RENDER_MODE = 0x0A,

    /** @brief Special-purpose debugging event. Context will vary over time. */
    EVENT_CODE_DEBUG0 = 0x10,
    /** @brief Special-purpose debugging event. Context will vary over time. */
    EVENT_CODE_DEBUG1 = 0x11,
    /** @brief Special-purpose debugging event. Context will vary over time. */
    EVENT_CODE_DEBUG2 = 0x12,
    /** @brief Special-purpose debugging event. Context will vary over time. */
    EVENT_CODE_DEBUG3 = 0x13,
    /** @brief Special-purpose debugging event. Context will vary over time. */
    EVENT_CODE_DEBUG4 = 0x14,

    /** @brief The hovered-over object id, if there is one.
     * Context usage:
     * i32 id = context.data.u32[0]; - will be INVALID ID if nothing is hovered over.
     */
    EVENT_CODE_OBJECT_HOVER_ID_CHANGED = 0x15,

    /** 
     * @brief An event fired by the renderer backend to indicate when any render targets
     * associated with the default window resources need to be refreshed (i.e. a window resize)
     */
    EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED = 0x16,

    /**
     * @brief An event fired by the kvar system when a kvar has been updated.
     */
    EVENT_CODE_KVAR_CHANGED = 0x17,

    /**
     * @brief An event fired when a watched file has been written to.
     * Context usage:
     * u32 watch_id = context.data.u32[0];
     */
    EVENT_CODE_WATCHED_FILE_WRITTEN = 0X18,

    /**
     * @brief An event fired when a watched file has been removed.
     * Context usage:
     * u32 watch_id = context.data.u32[0];
     */
    EVENT_CODE_WATCHED_FILE_DELETED = 0x19,

    /** @brief Keyboard key up/not held down.
     * Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_UP = 0x20,

    /** @brief Keyboard key held down.
     * Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_DOWN = 0x21,

    /** @brief Mouse button held down 
     * Context usage:
     * u16 button = data.data.u16[0];
    */
    EVENT_CODE_BUTTON_DOWN = 0x22,

    /** @brief The maximum event code that can be used internally. */
    MAX_EVENT_CODE = 0xFF
} system_event_code;
