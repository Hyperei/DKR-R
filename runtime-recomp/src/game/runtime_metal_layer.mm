#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SDL.h>
#include <cstdio>

static Uint32 fullscreen_event_type() {
    static const Uint32 type = SDL_RegisterEvents(1);
    return type;
}

extern "C" void dkr_set_window_fullscreen(Uint32 window_id, bool fullscreen) {
    // Window operations belong to the main thread. Use SDL's own thread-safe
    // queue, which the game's event pump consumes, rather than Cocoa dispatch.
    SDL_Event event{};
    event.type = fullscreen_event_type();
    event.user.windowID = window_id;
    event.user.code = fullscreen ? 1 : 0;
    if (event.type == Uint32(-1) || SDL_PushEvent(&event) != 1)
        std::fprintf(stderr, "[window] cannot queue fullscreen change: %s\n", SDL_GetError());
}

extern "C" bool dkr_handle_fullscreen_event(const SDL_Event* event) {
    if (event->type != fullscreen_event_type()) return false;
    // An ID, not a raw pointer, makes requests harmless after window shutdown.
    SDL_Window* window = SDL_GetWindowFromID(event->user.windowID);
    if (window && SDL_SetWindowFullscreen(window,
            event->user.code ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0)
        std::fprintf(stderr, "[window] fullscreen change failed: %s\n", SDL_GetError());
    return true;
}

extern "C" bool dkr_show_metal_layer(void* ns_window_ptr, void* layer_ptr) {
    if (ns_window_ptr == nullptr || layer_ptr == nullptr) return false;
    NSWindow* window = (__bridge NSWindow*)ns_window_ptr;
    CALayer* layer = (__bridge CALayer*)layer_ptr;
    NSView* contentView = window.contentView;
    for (NSView* view in contentView.subviews) {
        if (view.layer == layer) {
            // SDL2-compat can leave the launcher's separate Metal view above
            // RT64's view after SDL_DestroyRenderer. Keep ownership with SDL;
            // only change stacking order at the renderer handoff.
            [contentView addSubview:view positioned:NSWindowAbove relativeTo:nil];
            return true;
        }
    }
    return false;
}

extern "C" void* dkr_create_metal_layer(void* ns_window_ptr) {
    if (ns_window_ptr == nullptr) {
        return nullptr;
    }
    NSWindow* window = (__bridge NSWindow*)ns_window_ptr;
    NSView* contentView = [window contentView];
    [contentView setWantsLayer:YES];

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.frame = contentView.bounds;
    metalLayer.contentsScale = window.backingScaleFactor;
    metalLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;

    [contentView.layer addSublayer:metalLayer];
    return (__bridge_retained void*)metalLayer;
}

extern "C" void dkr_destroy_metal_layer(void* layer_ptr) {
    if (layer_ptr == nullptr) {
        return;
    }
    CAMetalLayer* layer = (__bridge_transfer CAMetalLayer*)layer_ptr;
    [layer removeFromSuperlayer];
}
#endif
