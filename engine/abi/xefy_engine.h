/*
 * xefy_engine.h - Xefy native engine C ABI (DRAFT v0, no implementation yet).
 *
 * The Prismio framework drives layout/painting and receives input through this
 * single header. Platform embedders (macOS, Linux Wayland/X11/DRM, Windows)
 * and the Skia shim implement it. 3D / bgfx is intentionally NOT part of this
 * header; it will live in a separate optional header later.
 *
 * PULL MODEL: the framework's `main` owns the loop. It keeps all state in local
 * variables and asks the engine for the next event with xefy_app_poll_event()
 * (like SDL/GLFW polling). The engine never calls back into the framework.
 *
 * HARD ABI RULES (dictated by current Prismio `extern fn` limits)
 *  1. Plain C99; only <stdint.h> and <stddef.h>; extern "C" under C++.
 *  2. No struct is passed or returned by value. Structs cross as `const T*`
 *     (in) or `T*` (out).
 *  3. No integer parameter/return narrower than 32 bits. Booleans are int32_t
 *     0/1. Struct fields prefer 32-bit and carry explicit padding.
 *  4. Geometry is 32-bit `float`; time is int64_t nanoseconds.
 *  5. Strings are `const char* utf8` + `int32_t length_bytes`, never NUL
 *     dependent. Strings handed to the caller are copied into a caller buffer
 *     (pointer + capacity); the function returns the NEEDED byte length
 *     (>= 0), or a negative value = -(xefy_status). If needed > capacity the
 *     buffer holds a truncated prefix. Buffers are not NUL-terminated.
 *  6. v0 has no callbacks: no function pointers appear anywhere in this header.
 *     Everything the engine has to tell the framework arrives as an
 *     xefy_event_t from xefy_app_poll_event(). Callback delivery will return
 *     in a later MINOR, for platforms that own `main` (Android, iOS), once
 *     Prismio can store closures.
 *  7. Objects are opaque handles with explicit create/destroy. Every
 *     `*_create` / `*_decode` / `*_build` writes its handle to an out-pointer
 *     and the caller owns it until the matching `*_destroy`. Exceptions: the
 *     canvas is borrowed (valid only between xefy_window_begin_frame and
 *     xefy_window_end_frame); windows are owned by the app and are also
 *     destroyed when the app is destroyed. Handles that are used by other
 *     objects (font collection, image) are internally ref-counted, so
 *     destroying your handle early is always safe.
 *  8. Every function is `xefy_` prefixed, exported with XEFY_API, returns an
 *     int32_t status (XEFY_OK == 0) unless noted, and never unwinds
 *     exceptions/longjmp across the boundary.
 *  9. ABI versioning: XEFY_ENGINE_ABI_MAJOR/MINOR plus
 *     xefy_engine_abi_version(). Structs that may grow start with
 *     `uint32_t struct_size`; fill it with sizeof() and zero the rest.
 * 10. Threading: see the THREADING section below.
 *
 * THREADING (v0, single-thread pull model): the "app thread" is the thread that
 * called xefy_app_create. Every function must be called from it, including
 * xefy_app_poll_event and everything that draws. The only exceptions, callable
 * from any thread: xefy_engine_abi_version, xefy_app_wake, xefy_app_quit.
 * Wrong-thread calls return XEFY_ERR_WRONG_THREAD. Background work hands results
 * to the loop by storing them where the loop can see them and calling
 * xefy_app_wake(), which makes a blocked poll return a WAKE event.
 *
 * USAGE SKETCH (C99; on a FRAME event: begin, draw, end)
 *
 *   int main(void) {
 *     xefy_app_config_t cfg = { sizeof cfg, XEFY_ENGINE_ABI_VERSION, 0, 16, "dev.example.demo" };
 *     xefy_window_desc_t wd = { sizeof wd, XEFY_WINDOW_RESIZABLE, 800.0f, 600.0f, 4, 0, "Demo" };
 *     xefy_app_t* app = NULL;  xefy_window_t* win = NULL;  xefy_paint_t* paint = NULL;
 *     xefy_event_t ev;  int32_t running = 1;
 *     if (xefy_app_create(&cfg, &app) != XEFY_OK) return 1;
 *     xefy_window_create(app, &wd, &win);
 *     xefy_paint_create(&paint);
 *     xefy_paint_set_color(paint, 0xFF2080FFu);
 *     xefy_window_request_frame(win);
 *     while (running) {
 *       ev.struct_size = sizeof ev;
 *       if (xefy_app_poll_event(app, &ev, -1) != XEFY_OK) continue;
 *       switch (ev.kind) {
 *         case XEFY_EVENT_QUIT_REQUESTED:
 *         case XEFY_EVENT_WINDOW_CLOSE_REQUESTED: running = 0; break;
 *         case XEFY_EVENT_FRAME: {
 *           xefy_canvas_t* canvas = NULL;
 *           xefy_rect_t r = { 20.0f, 20.0f, 220.0f, 120.0f };
 *           if (xefy_window_begin_frame(win, &canvas) == XEFY_OK) {
 *             xefy_canvas_clear(canvas, 0xFFFFFFFFu);
 *             xefy_canvas_draw_rect(canvas, &r, paint);
 *             xefy_window_end_frame(win);
 *           }
 *           xefy_window_request_frame(win);
 *           break; }
 *         default: break;
 *       }
 *     }
 *     xefy_paint_destroy(paint);
 *     xefy_app_destroy(app);
 *     return 0;
 *   }
 */
#ifndef XEFY_ENGINE_H_
#define XEFY_ENGINE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  if defined(XEFY_ENGINE_BUILD)
#    define XEFY_API __declspec(dllexport)
#  else
#    define XEFY_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define XEFY_API __attribute__((visibility("default")))
#else
#  define XEFY_API
#endif

/* ---------------------------------------------------------------- version */

#define XEFY_ENGINE_ABI_MAJOR 0u /* bump on breaking change (0 = unstable draft) */
#define XEFY_ENGINE_ABI_MINOR 1u /* bump on additive change */
#define XEFY_ENGINE_ABI_VERSION ((XEFY_ENGINE_ABI_MAJOR << 16) | XEFY_ENGINE_ABI_MINOR)

/* Returns (major << 16) | minor of the engine that is actually loaded. Any thread. */
XEFY_API uint32_t xefy_engine_abi_version(void);

/* ----------------------------------------------------------- status codes */
/* Errors are 1..99. Codes >= 100 are non-error results (the call worked). */

enum {
  XEFY_OK = 0,
  XEFY_ERR_INVALID_ARGUMENT = 1, /* null/out-of-range/bad struct_size */
  XEFY_ERR_OUT_OF_MEMORY = 2,
  XEFY_ERR_UNSUPPORTED = 3,      /* not available on this platform/build */
  XEFY_ERR_WRONG_THREAD = 4,
  XEFY_ERR_WRONG_STATE = 5,      /* e.g. canvas used outside begin_frame/end_frame */
  XEFY_ERR_ABI_MISMATCH = 6,     /* major version differs */
  XEFY_ERR_PLATFORM = 7,         /* OS call failed */
  XEFY_ERR_IO = 8,               /* file not found/unreadable */
  XEFY_ERR_DECODE = 9,           /* bad image/font data */

  XEFY_NO_EVENT = 100            /* NOT an error: poll timed out with no event */
};

/* --------------------------------------------------------- opaque handles */

typedef struct xefy_app xefy_app_t;
typedef struct xefy_window xefy_window_t;
typedef struct xefy_canvas xefy_canvas_t;
typedef struct xefy_paint xefy_paint_t;
typedef struct xefy_path xefy_path_t;
typedef struct xefy_image xefy_image_t;
typedef struct xefy_font_collection xefy_font_collection_t;
typedef struct xefy_paragraph_builder xefy_paragraph_builder_t;
typedef struct xefy_paragraph xefy_paragraph_t;

/* --------------------------------------------------------------- geometry */
/* Logical pixels unless stated otherwise. Always passed by pointer. */

typedef struct xefy_point { float x, y; } xefy_point_t;
typedef struct xefy_rect { float left, top, right, bottom; } xefy_rect_t;
/* Circular per-corner radii (elliptical corners may be added in a later MINOR). */
typedef struct xefy_rrect {
  xefy_rect_t rect;
  float radius_top_left, radius_top_right, radius_bottom_right, radius_bottom_left;
} xefy_rrect_t;

/* ---------------------------------------------------------- app lifecycle */
/* The framework's `main` owns the loop (see PULL MODEL above). There is no
 * run function and no ready/quit hook: create the app, create windows, then
 * poll. Callback delivery (on_ready, per-event callbacks, a posted-task queue)
 * will return in a later MINOR for platforms that own `main` (Android, iOS),
 * once Prismio can store closures. */

typedef struct xefy_app_config {
  uint32_t struct_size;       /* sizeof(xefy_app_config_t) */
  uint32_t abi_version;       /* XEFY_ENGINE_ABI_VERSION the caller compiled against */
  uint32_t flags;             /* reserved, 0 */
  int32_t app_id_length;
  const char* app_id_utf8;    /* reverse-DNS id, copied during create */
} xefy_app_config_t;

/* Fails with XEFY_ERR_ABI_MISMATCH if config->abi_version's major differs.
 * The calling thread becomes the app thread. */
XEFY_API int32_t xefy_app_create(const xefy_app_config_t* config, xefy_app_t** out_app);
/* App thread. Destroys remaining windows. No other thread may be inside
 * xefy_app_wake / xefy_app_quit for this app while it runs. */
XEFY_API int32_t xefy_app_destroy(xefy_app_t* app);
/* Any thread. Queues a QUIT_REQUESTED event, as if the OS had asked the app to
 * quit (Cmd-Q, session end...). The engine never quits by itself: the loop
 * decides what to do and eventually stops polling. Idempotent while queued. */
XEFY_API int32_t xefy_app_quit(xefy_app_t* app);
/* Any thread. Makes a blocked xefy_app_poll_event return an event of kind WAKE
 * (if no poll is blocked, the next poll returns it). Several wakes before the
 * next poll may coalesce into one WAKE event. */
XEFY_API int32_t xefy_app_wake(xefy_app_t* app);

/* Placeholder for future raster/vsync threads. v0: returns XEFY_OK on the
 * app thread (no-op) and XEFY_ERR_UNSUPPORTED on any other thread. */
XEFY_API int32_t xefy_thread_attach(void);
XEFY_API int32_t xefy_thread_detach(void);

/* ---------------------------------------------------------------- events */

enum { /* modifiers bitmask */
  XEFY_MOD_SHIFT = 1 << 0, XEFY_MOD_CTRL = 1 << 1, XEFY_MOD_ALT = 1 << 2,
  XEFY_MOD_META = 1 << 3, /* Command / Windows / Super */
  XEFY_MOD_CAPS_LOCK = 1 << 4
};
enum { /* pointer buttons bitmask */
  XEFY_BUTTON_PRIMARY = 1 << 0, XEFY_BUTTON_SECONDARY = 1 << 1, XEFY_BUTTON_MIDDLE = 1 << 2,
  XEFY_BUTTON_BACK = 1 << 3, XEFY_BUTTON_FORWARD = 1 << 4
};
enum { XEFY_POINTER_DOWN = 0, XEFY_POINTER_UP = 1, XEFY_POINTER_MOVE = 2,
       XEFY_POINTER_CANCEL = 3, XEFY_POINTER_SCROLL = 4, XEFY_POINTER_HOVER = 5 };
enum { XEFY_DEVICE_MOUSE = 0, XEFY_DEVICE_TOUCH = 1, XEFY_DEVICE_STYLUS = 2 };
enum { XEFY_KEY_DOWN = 0, XEFY_KEY_UP = 1 }; /* auto-repeat is DOWN with is_repeat = 1 */

/* xefy_event_t::kind */
enum {
  XEFY_EVENT_NONE = 0,                  /* never returned with XEFY_OK; written on XEFY_NO_EVENT */
  XEFY_EVENT_QUIT_REQUESTED = 1,        /* app level (window_id 0): OS or xefy_app_quit asks to quit */
  XEFY_EVENT_WAKE = 2,                  /* app level (window_id 0): xefy_app_wake was called */
  XEFY_EVENT_WINDOW_CLOSE_REQUESTED = 3,/* user asked to close; the window is NOT closed for you:
                                           call xefy_window_destroy to close it, or ignore to veto */
  XEFY_EVENT_WINDOW_RESIZED = 4,        /* width, height (logical pixels) */
  XEFY_EVENT_WINDOW_SCALE_CHANGED = 5,  /* scale (device pixel ratio) */
  XEFY_EVENT_WINDOW_FOCUS = 6,          /* focused (0/1) */
  XEFY_EVENT_FRAME = 7,                 /* vsync tick for a window that requested a frame;
                                           timestamp_ns is the frame time */
  XEFY_EVENT_POINTER = 8,               /* pointer_* fields, x, y, scroll_*, pressure, modifiers */
  XEFY_EVENT_KEY = 9,                   /* key_* fields, is_repeat, modifiers */
  XEFY_EVENT_TEXT_INPUT = 10            /* text is fetched with xefy_app_get_text_input */
};

/*
 * One FLAT event record. No unions, no nested structs, no arrays, no pointers:
 * Prismio reads it through a repr(C) struct and has no unions. Every field is
 * int32_t/uint32_t/int64_t/uint64_t/float, with explicit padding so every
 * 64-bit field is 8-byte aligned and the size is a multiple of 8.
 * Fields that are not relevant to `kind` are zero. Set struct_size to
 * sizeof(xefy_event_t) before every poll.
 */
typedef struct xefy_event {
  uint32_t struct_size;   /* in: sizeof(xefy_event_t); engine writes at most this many bytes */
  int32_t kind;           /* XEFY_EVENT_* */
  uint32_t window_id;     /* target window (xefy_window_get_id); 0 for app-level events */
  uint32_t reserved0;     /* padding, 0 */
  int64_t timestamp_ns;   /* monotonic clock; same base for every event kind */

  /* XEFY_EVENT_POINTER */
  int32_t pointer_kind;     /* XEFY_POINTER_* */
  int32_t pointer_device;   /* XEFY_DEVICE_* */
  int32_t pointer_id;       /* stable per contact/device while it is active */
  int32_t pointer_button;   /* XEFY_BUTTON_* that changed on DOWN/UP, else 0 */
  uint32_t pointer_buttons; /* bitmask of buttons held after this event */
  uint32_t modifiers;       /* XEFY_MOD_*; POINTER and KEY events */
  float x, y;               /* logical pixels, window content origin top-left */
  float scroll_dx, scroll_dy; /* logical pixels, positive = content moves right/down; SCROLL only */
  float pressure;           /* 0..1; 1 for mouse while pressed */

  /* XEFY_EVENT_KEY */
  int32_t key_action;       /* XEFY_KEY_* */
  uint32_t physical_key;    /* USB HID usage, page 0x07 (layout independent) */
  uint32_t logical_key;     /* Unicode scalar of the produced character, else 0x40000000 + HID usage */
  int32_t is_repeat;        /* 1 for auto-repeat (key_action is DOWN) */

  /* XEFY_EVENT_WINDOW_RESIZED / XEFY_EVENT_WINDOW_SCALE_CHANGED / XEFY_EVENT_WINDOW_FOCUS */
  float width, height;      /* logical pixels */
  float scale;              /* device pixel ratio */
  int32_t focused;          /* 0/1 */
  uint32_t reserved1;       /* padding, 0 */
} xefy_event_t;

/* App thread. Fetches the next event, in order (the engine may coalesce
 * redundant events such as repeated WINDOW_RESIZED, never reorders).
 *   timeout_ms  0 = do not block; -1 = wait until an event arrives; > 0 = wait
 *               at most that many milliseconds; other negatives are invalid.
 * Returns XEFY_OK with *out_event written, or XEFY_NO_EVENT (not an error) when
 * the timeout expired; then out_event->kind is XEFY_EVENT_NONE. out_event->struct_size
 * must be set by the caller and must be at least the size of the ABI 0.1
 * layout, else XEFY_ERR_INVALID_ARGUMENT. Events already queued for a window
 * that has since been destroyed are dropped. */
XEFY_API int32_t xefy_app_poll_event(xefy_app_t* app, xefy_event_t* out_event, int32_t timeout_ms);

/* App thread. Copies the committed UTF-8 text of the most recently polled
 * TEXT_INPUT event into buffer; returns the needed byte length (see rule 5), 0 if
 * no TEXT_INPUT was polled yet. The text stays available until the next
 * TEXT_INPUT event is polled, so it may be fetched more than once.
 * Committed text only. TODO(minor): preedit/composition event for IME. */
XEFY_API int32_t xefy_app_get_text_input(xefy_app_t* app, char* buffer, int32_t capacity);

/* ---------------------------------------------------------------- windows */

enum { XEFY_WINDOW_RESIZABLE = 1 << 0, XEFY_WINDOW_BORDERLESS = 1 << 1,
       XEFY_WINDOW_TRANSPARENT = 1 << 2,
       XEFY_WINDOW_HIDDEN = 1 << 3 /* create hidden; see xefy_window_show */ };

typedef struct xefy_window_desc {
  uint32_t struct_size;
  uint32_t flags;       /* XEFY_WINDOW_* */
  float width, height;  /* initial content size, logical pixels */
  int32_t title_length;
  int32_t reserved;     /* padding, 0 */
  const char* title_utf8; /* copied during create */
} xefy_window_desc_t;

XEFY_API int32_t xefy_window_create(xefy_app_t* app, const xefy_window_desc_t* desc, xefy_window_t** out_window);
/* Closes the window. A pending frame request is dropped and an open frame
 * (begin_frame without end_frame) is discarded. */
XEFY_API int32_t xefy_window_destroy(xefy_window_t* window);
/* Stable, non-zero, and never reused for the lifetime of the app. This is the
 * value events carry in window_id. */
XEFY_API int32_t xefy_window_get_id(xefy_window_t* window, uint32_t* out_id);
XEFY_API int32_t xefy_window_get_size(xefy_window_t* window, float* out_width, float* out_height);
XEFY_API int32_t xefy_window_get_scale(xefy_window_t* window, float* out_device_pixel_ratio);
XEFY_API int32_t xefy_window_set_title(xefy_window_t* window, const char* utf8, int32_t length_bytes);
/* Content size, logical pixels. The resulting WINDOW_RESIZED event is queued like any other. */
XEFY_API int32_t xefy_window_set_size(xefy_window_t* window, float width, float height);
/* Top-left of the content area in logical screen coordinates (origin: top-left
 * of the primary display). Platforms that cannot place windows (e.g. Wayland)
 * return XEFY_ERR_UNSUPPORTED. */
XEFY_API int32_t xefy_window_set_position(xefy_window_t* window, float x, float y);
/* Windows created with XEFY_WINDOW_HIDDEN stay invisible until shown. Hidden
 * windows get no FRAME events; a pending frame request is kept until shown. */
XEFY_API int32_t xefy_window_show(xefy_window_t* window);
XEFY_API int32_t xefy_window_hide(xefy_window_t* window);
/* Schedules ONE FRAME event at the next vsync (requests made before it is
 * delivered coalesce). Call again after handling FRAME to animate. */
XEFY_API int32_t xefy_window_request_frame(xefy_window_t* window);
/* Starts a frame and returns the window's canvas. Intended use: on a FRAME
 * event, begin_frame, draw, end_frame. Calls outside a frame, or begin_frame while
 * a frame is already open, return XEFY_ERR_WRONG_STATE. */
XEFY_API int32_t xefy_window_begin_frame(xefy_window_t* window, xefy_canvas_t** out_canvas);
/* Flushes and presents. The canvas handle from begin_frame is invalid afterwards.
 * XEFY_ERR_WRONG_STATE if no frame is open. */
XEFY_API int32_t xefy_window_end_frame(xefy_window_t* window);

/* ----------------------------------------------------------------- canvas */
/* A canvas is valid only between xefy_window_begin_frame and
 * xefy_window_end_frame; any canvas call outside returns XEFY_ERR_WRONG_STATE.
 * Canvas units are logical pixels (the engine applies the device scale). Colors
 * are 0xAARRGGBB, non-premultiplied. Matrices are 9 floats, row-major:
 * [scaleX skewX transX  skewY scaleY transY  persp0 persp1 persp2]. */

XEFY_API int32_t xefy_canvas_save(xefy_canvas_t* canvas);
XEFY_API int32_t xefy_canvas_restore(xefy_canvas_t* canvas);
XEFY_API int32_t xefy_canvas_translate(xefy_canvas_t* canvas, float dx, float dy);
XEFY_API int32_t xefy_canvas_scale(xefy_canvas_t* canvas, float sx, float sy);
XEFY_API int32_t xefy_canvas_rotate(xefy_canvas_t* canvas, float degrees);
XEFY_API int32_t xefy_canvas_concat(xefy_canvas_t* canvas, const float* matrix9);
XEFY_API int32_t xefy_canvas_clip_rect(xefy_canvas_t* canvas, const xefy_rect_t* rect, int32_t anti_alias);
XEFY_API int32_t xefy_canvas_clip_rrect(xefy_canvas_t* canvas, const xefy_rrect_t* rrect, int32_t anti_alias);
XEFY_API int32_t xefy_canvas_clear(xefy_canvas_t* canvas, uint32_t color_argb);
XEFY_API int32_t xefy_canvas_draw_rect(xefy_canvas_t* canvas, const xefy_rect_t* rect, const xefy_paint_t* paint);
XEFY_API int32_t xefy_canvas_draw_rrect(xefy_canvas_t* canvas, const xefy_rrect_t* rrect, const xefy_paint_t* paint);
XEFY_API int32_t xefy_canvas_draw_circle(xefy_canvas_t* canvas, float cx, float cy, float radius, const xefy_paint_t* paint);
XEFY_API int32_t xefy_canvas_draw_line(xefy_canvas_t* canvas, float x0, float y0, float x1, float y1, const xefy_paint_t* paint);
XEFY_API int32_t xefy_canvas_draw_path(xefy_canvas_t* canvas, const xefy_path_t* path, const xefy_paint_t* paint);

enum { XEFY_SAMPLING_NEAREST = 0, XEFY_SAMPLING_LINEAR = 1 };
/* src NULL = whole image; paint NULL = default (opaque, src-over). */
XEFY_API int32_t xefy_canvas_draw_image(xefy_canvas_t* canvas, const xefy_image_t* image,
                                        const xefy_rect_t* src, const xefy_rect_t* dst,
                                        int32_t sampling, const xefy_paint_t* paint);
/* The paragraph must have been laid out. (x, y) is its top-left. */
XEFY_API int32_t xefy_canvas_draw_paragraph(xefy_canvas_t* canvas, const xefy_paragraph_t* paragraph, float x, float y);

/* ------------------------------------------------------------------ paint */

enum { XEFY_PAINT_FILL = 0, XEFY_PAINT_STROKE = 1 };
enum { XEFY_STROKE_CAP_BUTT = 0, XEFY_STROKE_CAP_ROUND = 1, XEFY_STROKE_CAP_SQUARE = 2 };
enum { XEFY_STROKE_JOIN_MITER = 0, XEFY_STROKE_JOIN_ROUND = 1, XEFY_STROKE_JOIN_BEVEL = 2 };
enum { XEFY_TILE_CLAMP = 0, XEFY_TILE_REPEAT = 1, XEFY_TILE_MIRROR = 2 };
/* Values are ABI-stable and independent of Skia's enum order. */
enum {
  XEFY_BLEND_CLEAR = 0, XEFY_BLEND_SRC = 1, XEFY_BLEND_DST = 2, XEFY_BLEND_SRC_OVER = 3,
  XEFY_BLEND_DST_OVER = 4, XEFY_BLEND_SRC_IN = 5, XEFY_BLEND_DST_IN = 6, XEFY_BLEND_SRC_OUT = 7,
  XEFY_BLEND_DST_OUT = 8, XEFY_BLEND_SRC_ATOP = 9, XEFY_BLEND_DST_ATOP = 10, XEFY_BLEND_XOR = 11,
  XEFY_BLEND_PLUS = 12, XEFY_BLEND_MODULATE = 13, XEFY_BLEND_SCREEN = 14, XEFY_BLEND_OVERLAY = 15,
  XEFY_BLEND_DARKEN = 16, XEFY_BLEND_LIGHTEN = 17, XEFY_BLEND_MULTIPLY = 18
};

/* Defaults: color 0xFF000000, fill, stroke width 1, butt caps, miter joins,
 * miter limit 4, anti-alias on, SRC_OVER, no shader. */
XEFY_API int32_t xefy_paint_create(xefy_paint_t** out_paint);
XEFY_API int32_t xefy_paint_destroy(xefy_paint_t* paint);
XEFY_API int32_t xefy_paint_set_color(xefy_paint_t* paint, uint32_t color_argb);
XEFY_API int32_t xefy_paint_set_style(xefy_paint_t* paint, int32_t style);
XEFY_API int32_t xefy_paint_set_stroke_width(xefy_paint_t* paint, float width);
XEFY_API int32_t xefy_paint_set_stroke_cap(xefy_paint_t* paint, int32_t cap);   /* XEFY_STROKE_CAP_* */
XEFY_API int32_t xefy_paint_set_stroke_join(xefy_paint_t* paint, int32_t join); /* XEFY_STROKE_JOIN_* */
/* Miter joins longer than limit * stroke width fall back to bevel. Must be >= 1. */
XEFY_API int32_t xefy_paint_set_stroke_miter_limit(xefy_paint_t* paint, float limit);
XEFY_API int32_t xefy_paint_set_anti_alias(xefy_paint_t* paint, int32_t anti_alias);
XEFY_API int32_t xefy_paint_set_blend_mode(xefy_paint_t* paint, int32_t blend_mode);
/* Replaces any shader. colors/stops are copied. stops NULL = evenly spaced; count >= 2. */
XEFY_API int32_t xefy_paint_set_linear_gradient(xefy_paint_t* paint, const xefy_point_t* start, const xefy_point_t* end,
                                                const uint32_t* colors, const float* stops, int32_t count,
                                                int32_t tile_mode);
XEFY_API int32_t xefy_paint_clear_shader(xefy_paint_t* paint);

/* ------------------------------------------------------------------- path */

enum { XEFY_FILL_RULE_WINDING = 0, XEFY_FILL_RULE_EVEN_ODD = 1 };

XEFY_API int32_t xefy_path_create(xefy_path_t** out_path);
XEFY_API int32_t xefy_path_destroy(xefy_path_t* path);
/* Clears the geometry; the fill rule is kept. Default fill rule: WINDING. */
XEFY_API int32_t xefy_path_reset(xefy_path_t* path);
XEFY_API int32_t xefy_path_set_fill_rule(xefy_path_t* path, int32_t fill_rule); /* XEFY_FILL_RULE_* */
XEFY_API int32_t xefy_path_move_to(xefy_path_t* path, float x, float y);
XEFY_API int32_t xefy_path_line_to(xefy_path_t* path, float x, float y);
XEFY_API int32_t xefy_path_quad_to(xefy_path_t* path, float cx, float cy, float x, float y);
XEFY_API int32_t xefy_path_cubic_to(xefy_path_t* path, float c1x, float c1y, float c2x, float c2y, float x, float y);
XEFY_API int32_t xefy_path_close(xefy_path_t* path);
XEFY_API int32_t xefy_path_add_rect(xefy_path_t* path, const xefy_rect_t* rect);
XEFY_API int32_t xefy_path_add_rrect(xefy_path_t* path, const xefy_rrect_t* rrect);

/* ------------------------------------------------------------------- text */
/* Backed by Skia's paragraph module. Text offsets in this API are UTF-8 BYTE
 * offsets into the concatenation of all add_text() input (the engine converts
 * to/from Skia's UTF-16 units). */

enum { XEFY_TEXT_ALIGN_LEFT = 0, XEFY_TEXT_ALIGN_RIGHT = 1, XEFY_TEXT_ALIGN_CENTER = 2,
       XEFY_TEXT_ALIGN_JUSTIFY = 3, XEFY_TEXT_ALIGN_START = 4, XEFY_TEXT_ALIGN_END = 5 };
enum { XEFY_TEXT_DIRECTION_LTR = 0, XEFY_TEXT_DIRECTION_RTL = 1 };
enum { XEFY_AFFINITY_UPSTREAM = 0, XEFY_AFFINITY_DOWNSTREAM = 1 };

typedef struct xefy_paragraph_style {
  uint32_t struct_size;
  int32_t text_align;      /* XEFY_TEXT_ALIGN_* */
  int32_t text_direction;  /* XEFY_TEXT_DIRECTION_* */
  int32_t max_lines;       /* 0 = unlimited */
  int32_t ellipsis_length; /* 0 = no ellipsis */
  int32_t reserved;
  const char* ellipsis_utf8; /* copied during builder_create */
} xefy_paragraph_style_t;

typedef struct xefy_text_style {
  uint32_t struct_size;
  float font_size;        /* logical pixels */
  int32_t font_weight;    /* 100..900 */
  int32_t italic;         /* 0/1 */
  uint32_t color_argb;
  float letter_spacing;   /* logical pixels */
  float line_height;      /* multiple of font_size; 0 = font default */
  int32_t family_length;
  const char* family_utf8; /* comma-separated fallback list allowed; copied during push */
} xefy_text_style_t;

/* The collection always falls back to platform system fonts after added fonts. */
XEFY_API int32_t xefy_font_collection_create(xefy_font_collection_t** out_fonts);
XEFY_API int32_t xefy_font_collection_destroy(xefy_font_collection_t* fonts);
XEFY_API int32_t xefy_font_collection_add_file(xefy_font_collection_t* fonts, const char* path_utf8, int32_t path_length);
/* Data is copied; the buffer may be freed on return. */
XEFY_API int32_t xefy_font_collection_add_memory(xefy_font_collection_t* fonts, const uint8_t* data, int32_t length_bytes);

XEFY_API int32_t xefy_paragraph_builder_create(xefy_font_collection_t* fonts, const xefy_paragraph_style_t* style,
                                               xefy_paragraph_builder_t** out_builder);
XEFY_API int32_t xefy_paragraph_builder_destroy(xefy_paragraph_builder_t* builder);
XEFY_API int32_t xefy_paragraph_builder_push_style(xefy_paragraph_builder_t* builder, const xefy_text_style_t* style);
XEFY_API int32_t xefy_paragraph_builder_pop_style(xefy_paragraph_builder_t* builder);
XEFY_API int32_t xefy_paragraph_builder_add_text(xefy_paragraph_builder_t* builder, const char* utf8, int32_t length_bytes);
/* Does not consume the builder; destroy it separately. Needs layout() before use. */
XEFY_API int32_t xefy_paragraph_builder_build(xefy_paragraph_builder_t* builder, xefy_paragraph_t** out_paragraph);

XEFY_API int32_t xefy_paragraph_destroy(xefy_paragraph_t* paragraph);
XEFY_API int32_t xefy_paragraph_layout(xefy_paragraph_t* paragraph, float width);
XEFY_API int32_t xefy_paragraph_get_height(const xefy_paragraph_t* paragraph, float* out);
XEFY_API int32_t xefy_paragraph_get_longest_line(const xefy_paragraph_t* paragraph, float* out);
XEFY_API int32_t xefy_paragraph_get_min_intrinsic_width(const xefy_paragraph_t* paragraph, float* out);
XEFY_API int32_t xefy_paragraph_get_max_intrinsic_width(const xefy_paragraph_t* paragraph, float* out);
XEFY_API int32_t xefy_paragraph_get_line_count(const xefy_paragraph_t* paragraph, int32_t* out);
/* Hit test: paragraph-local (x, y) -> nearest text offset. out_affinity may be NULL. */
XEFY_API int32_t xefy_paragraph_get_offset_at_position(const xefy_paragraph_t* paragraph, float x, float y,
                                                       int32_t* out_offset, int32_t* out_affinity);
/* Writes up to `capacity` boxes covering [start, end) into out_rects; returns the
 * NEEDED count (>= 0) or -(status). out_rects may be NULL when capacity is 0. */
XEFY_API int32_t xefy_paragraph_get_rects_for_range(const xefy_paragraph_t* paragraph, int32_t start_offset,
                                                    int32_t end_offset, xefy_rect_t* out_rects, int32_t capacity);
/* Zero-width caret box at `offset` (valid for empty text / end of text too). */
XEFY_API int32_t xefy_paragraph_get_caret_rect(const xefy_paragraph_t* paragraph, int32_t offset, xefy_rect_t* out_rect);

/* ----------------------------------------------------------------- images */

/* Decodes PNG/JPEG/WebP synchronously on the calling thread; data is not retained. */
XEFY_API int32_t xefy_image_decode(const uint8_t* data, int32_t length_bytes, xefy_image_t** out_image);
XEFY_API int32_t xefy_image_destroy(xefy_image_t* image);
XEFY_API int32_t xefy_image_get_width(const xefy_image_t* image, int32_t* out_pixels);
XEFY_API int32_t xefy_image_get_height(const xefy_image_t* image, int32_t* out_pixels);

/* ------------------------------------------------------- platform services */

enum {
  XEFY_CURSOR_DEFAULT = 0, XEFY_CURSOR_TEXT = 1, XEFY_CURSOR_POINTER = 2, XEFY_CURSOR_CROSSHAIR = 3,
  XEFY_CURSOR_GRAB = 4, XEFY_CURSOR_GRABBING = 5, XEFY_CURSOR_RESIZE_EW = 6, XEFY_CURSOR_RESIZE_NS = 7,
  XEFY_CURSOR_RESIZE_NESW = 8, XEFY_CURSOR_RESIZE_NWSE = 9, XEFY_CURSOR_NOT_ALLOWED = 10,
  XEFY_CURSOR_WAIT = 11, XEFY_CURSOR_HIDDEN = 12
};

/* Returns needed byte length (or -(status)); see rule 5. Empty clipboard returns 0. */
XEFY_API int32_t xefy_clipboard_get_text(xefy_app_t* app, char* buffer, int32_t capacity);
XEFY_API int32_t xefy_clipboard_set_text(xefy_app_t* app, const char* utf8, int32_t length_bytes);
XEFY_API int32_t xefy_window_set_cursor(xefy_window_t* window, int32_t cursor);
/* IME: caret_rect is in window logical pixels and lets the OS place its candidate window. */
XEFY_API int32_t xefy_window_text_input_activate(xefy_window_t* window, const xefy_rect_t* caret_rect);
XEFY_API int32_t xefy_window_text_input_set_caret_rect(xefy_window_t* window, const xefy_rect_t* caret_rect);
XEFY_API int32_t xefy_window_text_input_deactivate(xefy_window_t* window);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XEFY_ENGINE_H_ */
