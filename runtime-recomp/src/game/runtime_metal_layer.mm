#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

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
