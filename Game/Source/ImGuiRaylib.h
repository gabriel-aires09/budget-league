#ifndef IMGUIRAYLIB_H
#define IMGUIRAYLIB_H

// Dear ImGui on raylib: the whole backend, both halves. It exists only in Debug
// and Development, so every call site is already inside GAME_DEV_TOOLS.
//
// BeginFrame must run before anything calls ImGui, and EndFrame between the
// scene's own drawing and EndDrawing, so the panels land on top of the game.
namespace imgui
{
    void Initialize();
    void Shutdown();

    void BeginFrame(float deltaTime);
    void EndFrame();

    // True when a panel is under the mouse or has keyboard focus, so the game can
    // leave that input alone.
    bool WantsMouse();
    bool WantsKeyboard();
}

#endif
