#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "xefy_platform.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTypeface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "include/gpu/ganesh/mtl/GrMtlTypes.h"
#include "include/gpu/ganesh/mtl/SkSurfaceMetal.h"
#include "include/ports/SkFontMgr_mac_ct.h"

namespace {
struct DrawCommand {
    enum class Kind { RoundedRect, Text, TextBlock } kind;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t radius;
    int32_t size;
    int32_t cursor;
    uint32_t color;
    bool bold;
    std::string text;
};

struct HitTarget {
    int32_t id;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

std::vector<DrawCommand> gCommands;
std::vector<HitTarget> gHitTargets;
uint32_t gClearColor = 0xFF090B14;

SkColor opaqueColor(int32_t color) {
    return static_cast<SkColor>(0xFF000000u | (static_cast<uint32_t>(color) & 0x00FFFFFFu));
}

size_t nextUtf8(const std::string& value, size_t index) {
    const unsigned char first = static_cast<unsigned char>(value[index]);
    size_t count = 1;
    if ((first & 0xE0) == 0xC0) count = 2;
    else if ((first & 0xF0) == 0xE0) count = 3;
    else if ((first & 0xF8) == 0xF0) count = 4;
    return std::min(value.size(), index + count);
}

void drawTextBlock(SkCanvas* canvas,
                   const DrawCommand& command,
                   const SkFont& font,
                   const SkPaint& paint) {
    const SkRect clip = SkRect::MakeXYWH(command.x, command.y, command.width, command.height);
    canvas->save();
    canvas->clipRect(clip);

    const size_t length = command.text.size();
    size_t logicalStart = 0;
    int32_t line = 0;
    bool cursorDrawn = false;
    const SkScalar lineHeight = command.size + 7;
    while (logicalStart <= length && command.y + (line + 1) * lineHeight <= command.y + command.height) {
        const size_t foundNewline = command.text.find('\n', logicalStart);
        const size_t logicalEnd = foundNewline == std::string::npos ? length : foundNewline;
        size_t start = logicalStart;

        if (start == logicalEnd) {
            const SkScalar baseline = command.y + command.size + line * lineHeight;
            if (command.cursor == static_cast<int32_t>(start)) {
                SkPaint caretPaint;
                caretPaint.setAntiAlias(true);
                caretPaint.setColor(SK_ColorWHITE);
                canvas->drawRect(SkRect::MakeXYWH(command.x, baseline - command.size + 2, 1.5f, command.size), caretPaint);
                cursorDrawn = true;
            }
            ++line;
        }

        while (start < logicalEnd && command.y + (line + 1) * lineHeight <= command.y + command.height) {
            size_t position = start;
            size_t fit = start;
            size_t lastSpace = std::string::npos;
            while (position < logicalEnd) {
                const size_t next = nextUtf8(command.text, position);
                const std::string candidate = command.text.substr(start, next - start);
                if (font.measureText(candidate.data(), candidate.size(), SkTextEncoding::kUTF8) > command.width &&
                    fit > start) {
                    break;
                }
                fit = next;
                if (command.text[position] == ' ') lastSpace = position;
                position = next;
                if (font.measureText(candidate.data(), candidate.size(), SkTextEncoding::kUTF8) > command.width) {
                    break;
                }
            }

            size_t end = fit;
            size_t nextStart = fit;
            if (fit < logicalEnd && lastSpace != std::string::npos && lastSpace >= start && lastSpace < fit) {
                end = lastSpace;
                nextStart = lastSpace + 1;
                while (nextStart < logicalEnd && command.text[nextStart] == ' ') ++nextStart;
            }
            if (nextStart <= start) nextStart = nextUtf8(command.text, start);

            const std::string lineText = command.text.substr(start, end - start);
            const SkScalar baseline = command.y + command.size + line * lineHeight;
            canvas->drawString(lineText.c_str(), command.x, baseline, font, paint);

            const int32_t cursor = command.cursor;
            if (!cursorDrawn && cursor >= static_cast<int32_t>(start) &&
                (cursor < static_cast<int32_t>(end) ||
                 (cursor == static_cast<int32_t>(end) && end == logicalEnd))) {
                const size_t prefixLength = static_cast<size_t>(cursor) - start;
                const std::string prefix = command.text.substr(start, prefixLength);
                const SkScalar caretX = command.x + font.measureText(
                    prefix.data(), prefix.size(), SkTextEncoding::kUTF8);
                SkPaint caretPaint;
                caretPaint.setAntiAlias(true);
                caretPaint.setColor(SK_ColorWHITE);
                canvas->drawRect(SkRect::MakeXYWH(caretX, baseline - command.size + 2, 1.5f, command.size), caretPaint);
                cursorDrawn = true;
            }
            start = nextStart;
            ++line;
        }

        if (foundNewline == std::string::npos) break;
        logicalStart = logicalEnd + 1;
    }
    canvas->restore();
}
}  // namespace

extern "C" void xefy_ui_clear(int32_t color) {
    gClearColor = opaqueColor(color);
}

extern "C" void xefy_ui_rounded_rect(int32_t x,
                                      int32_t y,
                                      int32_t width,
                                      int32_t height,
                                      int32_t radius,
                                      int32_t color) {
    DrawCommand command{};
    command.kind = DrawCommand::Kind::RoundedRect;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.radius = radius;
    command.color = opaqueColor(color);
    gCommands.push_back(std::move(command));
}

extern "C" void xefy_ui_text(const char* value,
                              int32_t x,
                              int32_t baseline,
                              int32_t size,
                              int32_t color,
                              int32_t bold) {
    DrawCommand command{};
    command.kind = DrawCommand::Kind::Text;
    command.x = x;
    command.y = baseline;
    command.size = std::max<int32_t>(1, size);
    command.color = opaqueColor(color);
    command.bold = bold != 0;
    command.text = value ? value : "";
    gCommands.push_back(std::move(command));
}

extern "C" void xefy_ui_button(int32_t id,
                                int32_t x,
                                int32_t y,
                                int32_t width,
                                int32_t height) {
    gHitTargets.push_back({id, x, y, width, height});
}

extern "C" void xefy_ui_text_block(const char* value,
                                    int32_t cursor,
                                    int32_t x,
                                    int32_t y,
                                    int32_t width,
                                    int32_t height,
                                    int32_t size,
                                    int32_t color) {
    DrawCommand command{};
    command.kind = DrawCommand::Kind::TextBlock;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.cursor = cursor;
    command.size = std::max<int32_t>(1, size);
    command.color = opaqueColor(color);
    command.text = value ? value : "";
    gCommands.push_back(std::move(command));
}

@interface XefySkiaView : MTKView <MTKViewDelegate>
- (instancetype)initWithFrame:(NSRect)frame
                    fontManager:(sk_sp<SkFontMgr>)fontManager
                         render:(XefyRender)render
                          event:(XefyEvent)event
                          input:(XefyInput)input;
@end

@implementation XefySkiaView {
    id<MTLCommandQueue> _commandQueue;
    sk_sp<GrDirectContext> _context;
    sk_sp<SkFontMgr> _fontManager;
    XefyRender _render;
    XefyEvent _event;
    XefyInput _input;
}

- (instancetype)initWithFrame:(NSRect)frame
                    fontManager:(sk_sp<SkFontMgr>)fontManager
                         render:(XefyRender)render
                          event:(XefyEvent)event
                          input:(XefyInput)input {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    self = [super initWithFrame:frame device:device];
    if (!self || !device) return nil;

    _commandQueue = [device newCommandQueue];
    if (!_commandQueue) return nil;
    _fontManager = std::move(fontManager);
    _render = render;
    _event = event;
    _input = input;
    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.framebufferOnly = NO;
    self.delegate = self;

    GrMtlBackendContext backendContext{};
    backendContext.fDevice.retain((__bridge GrMTLHandle)device);
    backendContext.fQueue.retain((__bridge GrMTLHandle)_commandQueue);
    _context = GrDirectContexts::MakeMetal(backendContext);
    if (!_context) return nil;
    return self;
}

- (void)drawInMTKView:(MTKView*)view {
    if (!_context || !view.currentDrawable || !_render) return;

    gCommands.clear();
    gHitTargets.clear();
    const CGFloat scale = self.window ? self.window.backingScaleFactor : 1.0;
    const int32_t width = static_cast<int32_t>(self.bounds.size.width);
    const int32_t height = static_cast<int32_t>(self.bounds.size.height);
    _render(width, height);

    sk_sp<SkSurface> surface = SkSurfaces::WrapMTKView(
        _context.get(),
        (__bridge GrMTLHandle)view,
        kTopLeft_GrSurfaceOrigin,
        1,
        kBGRA_8888_SkColorType,
        nullptr,
        nullptr);
    if (!surface) return;

    SkCanvas* canvas = surface->getCanvas();
    canvas->save();
    canvas->scale(scale, scale);
    canvas->clear(gClearColor);

    sk_sp<SkTypeface> regularFace = _fontManager->matchFamilyStyle(
        "Helvetica Neue", SkFontStyle::Normal());
    sk_sp<SkTypeface> boldFace = _fontManager->matchFamilyStyle(
        "Helvetica Neue", SkFontStyle::Bold());
    if (!regularFace) regularFace = _fontManager->matchFamilyStyle(nullptr, SkFontStyle::Normal());
    if (!boldFace) boldFace = _fontManager->matchFamilyStyle(nullptr, SkFontStyle::Bold());

    for (const DrawCommand& command : gCommands) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(command.color);
        if (command.kind == DrawCommand::Kind::RoundedRect) {
            const SkRect rect = SkRect::MakeXYWH(command.x, command.y, command.width, command.height);
            canvas->drawRRect(SkRRect::MakeRectXY(rect, command.radius, command.radius), paint);
        } else if (command.kind == DrawCommand::Kind::Text) {
            const SkFont font(command.bold ? boldFace : regularFace, command.size);
            canvas->drawString(command.text.c_str(), command.x, command.y, font, paint);
        } else {
            const SkFont font(regularFace, command.size);
            drawTextBlock(canvas, command, font, paint);
        }
    }

    canvas->restore();
    _context->flushAndSubmit(surface.get());
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    (void)view;
    (void)size;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    if (!_input) return;
    const NSEventModifierFlags modifiers = event.modifierFlags;
    if ((modifiers & NSEventModifierFlagCommand) != 0) {
        [super keyDown:event];
        return;
    }

    int32_t action = 0;
    switch (event.keyCode) {
        case 51: action = 2; break;  // Backspace
        case 117: action = 3; break;  // Forward delete
        case 123: action = 4; break;  // Left
        case 124: action = 5; break;  // Right
        case 115: action = 6; break;  // Home
        case 119: action = 7; break;  // End
        case 36:
        case 76: {
            static const uint8_t newline = '\n';
            _input(1, &newline, 1);
            [self setNeedsDisplay:YES];
            return;
        }
        case 48: {
            static const uint8_t tab = '\t';
            _input(1, &tab, 1);
            [self setNeedsDisplay:YES];
            return;
        }
        default: break;
    }

    if (action != 0) {
        _input(action, nullptr, 0);
        [self setNeedsDisplay:YES];
        return;
    }
    if ((modifiers & NSEventModifierFlagControl) != 0) {
        [super keyDown:event];
        return;
    }

    NSString* characters = event.characters;
    if (characters.length == 0) return;
    const char* utf8 = characters.UTF8String;
    const NSUInteger byteLength = [characters lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    if (utf8 && byteLength > 0) {
        _input(1, reinterpret_cast<const uint8_t*>(utf8), static_cast<int32_t>(byteLength));
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseUp:(NSEvent*)event {
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const CGFloat x = point.x;
    const CGFloat y = self.bounds.size.height - point.y;
    for (auto it = gHitTargets.rbegin(); it != gHitTargets.rend(); ++it) {
        if (x >= it->x && x <= it->x + it->width &&
            y >= it->y && y <= it->y + it->height) {
            if (_event) _event(it->id);
            [self setNeedsDisplay:YES];
            return;
        }
    }
}

@end

@interface XefyAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, copy) NSString* titleText;
@property(nonatomic) XefyRender render;
@property(nonatomic) XefyEvent event;
@property(nonatomic) XefyInput input;
- (instancetype)initWithTitle:(NSString*)title
                        render:(XefyRender)render
                         event:(XefyEvent)event
                         input:(XefyInput)input;
@end

@implementation XefyAppDelegate

- (instancetype)initWithTitle:(NSString*)title
                        render:(XefyRender)render
                         event:(XefyEvent)event
                         input:(XefyInput)input {
    self = [super init];
    if (self) {
        _titleText = [title copy];
        _render = render;
        _event = event;
        _input = input;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    const NSRect frame = NSMakeRect(0, 0, 820, 560);
    self.window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled |
                            NSWindowStyleMaskClosable |
                            NSWindowStyleMaskMiniaturizable |
                            NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    [self.window setTitle:[NSString stringWithFormat:@"%@ · Xefy", self.titleText]];
    [self.window center];

    sk_sp<SkFontMgr> fonts = SkFontMgr_New_CoreText(nullptr);
    XefySkiaView* view = [[XefySkiaView alloc]
        initWithFrame:frame
          fontManager:std::move(fonts)
               render:self.render
                event:self.event
                input:self.input];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.window.contentView = view;
    [self.window makeFirstResponder:view];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    (void)sender;
    return YES;
}

@end

extern "C" int32_t xefy_macos_run(const char* title,
                                  uintptr_t render_address,
                                  uintptr_t event_address,
                                  uintptr_t input_address) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        XefyAppDelegate* delegate = [[XefyAppDelegate alloc]
            initWithTitle:[NSString stringWithUTF8String:title ? title : "Xefy"]
                   render:reinterpret_cast<XefyRender>(render_address)
                    event:reinterpret_cast<XefyEvent>(event_address)
                    input:reinterpret_cast<XefyInput>(input_address)];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
