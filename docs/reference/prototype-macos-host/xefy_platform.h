#ifndef XEFY_PLATFORM_H_
#define XEFY_PLATFORM_H_

#include <stdint.h>

typedef void (*XefyRender)(int32_t width, int32_t height);
typedef void (*XefyEvent)(int32_t id);
typedef void (*XefyInput)(int32_t action, const uint8_t* bytes, int32_t length);

#ifdef __cplusplus
extern "C" {
#endif

int32_t xefy_macos_run(const char* title,
                       uintptr_t render_address,
                       uintptr_t event_address,
                       uintptr_t input_address);
void xefy_ui_clear(int32_t color);
void xefy_ui_rounded_rect(int32_t x,
                          int32_t y,
                          int32_t width,
                          int32_t height,
                          int32_t radius,
                          int32_t color);
void xefy_ui_text(const char* value,
                  int32_t x,
                  int32_t baseline,
                  int32_t size,
                  int32_t color,
                  int32_t bold);
void xefy_ui_button(int32_t id,
                    int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height);
void xefy_ui_text_block(const char* value,
                        int32_t cursor,
                        int32_t x,
                        int32_t y,
                        int32_t width,
                        int32_t height,
                        int32_t size,
                        int32_t color);

#ifdef __cplusplus
}
#endif

#endif
