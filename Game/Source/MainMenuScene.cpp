#include "MainMenuScene.h"

#if defined(GAME_DEBUG)
    #define BUILD_NAME "Debug"
#elif defined(GAME_DEVELOPMENT)
    #define BUILD_NAME "Development"
#else
    #define BUILD_NAME "Release"
#endif

// This scene has no physics and no GameObjects.
void MainMenuScene::Initialize()
{
}

void MainMenuScene::Update(float deltaTime)
{
    (void)deltaTime;

    if (settingsOpen && IsKeyPressed(KEY_ESCAPE))
        settingsOpen = false;
}

void MainMenuScene::Draw()
{
    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;

    ClearBackground(uistyle::Background);

    // Placeholder title treatment until the real art lands (Milestone 13).
    uistyle::DrawTitle("ARCADE CAR SOCCER", centerX, 90.0f * scale, 52.0f);
    uistyle::DrawTextCentered("ROCKET POWERED SOCCER", centerX, 172.0f * scale, 18.0f, uistyle::TextDim);

    if (settingsOpen)
    {
        Rectangle area = { centerX - SettingsMenu::PreferredWidth() * 0.5f, 200.0f * scale,
                           SettingsMenu::PreferredWidth(), SettingsMenu::PreferredHeight() };
        settingsOpen = settingsMenu.Draw(*settings, area);
    }
    else
    {
        const float rowWidth = 320.0f * scale;
        const float rowHeight = 46.0f * scale;
        const float rowGap = 12.0f * scale;
        Rectangle row = { centerX - rowWidth * 0.5f, 260.0f * scale, rowWidth, rowHeight };

        menu.itemCount = 3;
        menu.Update();

        if (menu.Item(row, "Play", 0))
            pendingAction = MenuAction::StartMatch;
        row.y += rowHeight + rowGap;

        if (menu.Item(row, "Settings", 1))
            settingsOpen = true;
        row.y += rowHeight + rowGap;

        if (menu.Item(row, "Exit", 2))
            pendingAction = MenuAction::ExitGame;
    }

    uistyle::DrawTextAt("Made with raylib, Jolt Physics and Dear ImGui",
                        20.0f * scale, GetScreenHeight() - 58.0f * scale, 16.0f, uistyle::TextDim);
    uistyle::DrawTextAt(TextFormat("Arcade Car Soccer - %s build - %s", BUILD_NAME, __DATE__),
                        20.0f * scale, GetScreenHeight() - 34.0f * scale, 16.0f, uistyle::TextDim);
}

void MainMenuScene::Shutdown()
{
}
