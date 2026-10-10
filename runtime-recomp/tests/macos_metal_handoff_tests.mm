#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_metal.h>
#include <SDL_syswm.h>
#import <Cocoa/Cocoa.h>
#include <cstdio>

extern "C" bool dkr_show_metal_layer(void*, void*);

int main() {
    @autoreleasepool {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
        SDL_Window* window = SDL_CreateWindow("Metal handoff test", 0, 0, 320, 240,
            SDL_WINDOW_HIDDEN | SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window) return 1;
        SDL_MetalView game = SDL_Metal_CreateView(window);
        SDL_Renderer* launcher = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!game || !launcher) return 1;
        SDL_RenderClear(launcher);
        SDL_RenderPresent(launcher);
        SDL_DestroyRenderer(launcher);
        SDL_SysWMinfo info{};
        SDL_VERSION(&info.version);
        if (!SDL_GetWindowWMInfo(window, &info)) return 1;
        NSWindow* native = info.info.cocoa.window;
        CALayer* layer = (__bridge CALayer*)SDL_Metal_GetLayer(game);
        // Simulate the stale launcher surface explicitly, so this also tests
        // genuine SDL2 builds that happen to reuse the game's view.
        NSView* stale = [[NSView alloc] initWithFrame:native.contentView.bounds];
        stale.wantsLayer = YES;
        [native.contentView addSubview:stale];
        const bool hidden = native.contentView.subviews.lastObject.layer != layer;
        const bool shown = dkr_show_metal_layer((__bridge void*)native, (__bridge void*)layer);
        const bool front = native.contentView.subviews.lastObject.layer == layer;
        const bool returned = dkr_show_metal_layer((__bridge void*)native, (__bridge void*)stale.layer)
            && native.contentView.subviews.lastObject == stale;
        const bool relaunched = dkr_show_metal_layer((__bridge void*)native, (__bridge void*)layer)
            && native.contentView.subviews.lastObject.layer == layer;
        const bool invalid = !dkr_show_metal_layer(nullptr, nullptr);
        [stale removeFromSuperview];
        SDL_Metal_DestroyView(game);
        SDL_DestroyWindow(window);
        SDL_Quit();
        if (!hidden || !shown || !front || !returned || !relaunched || !invalid) return 1;
        std::puts("PASS: game Metal view replaces stale launcher surface");
        return 0;
    }
}
