#include "ImGuiRaylib.h"

#ifdef GAME_DEV_TOOLS

#include <raylib.h>
#include <rlgl.h>

#include <imgui.h>

#include <cstdint>

// Keys the panels actually need: the mouse does the work, and these are what
// makes a slider's Ctrl+click text entry usable.
static const struct
{
    int raylibKey;
    ImGuiKey imguiKey;
} KEY_MAP[] = {
    { KEY_TAB, ImGuiKey_Tab },
    { KEY_LEFT, ImGuiKey_LeftArrow },
    { KEY_RIGHT, ImGuiKey_RightArrow },
    { KEY_UP, ImGuiKey_UpArrow },
    { KEY_DOWN, ImGuiKey_DownArrow },
    { KEY_HOME, ImGuiKey_Home },
    { KEY_END, ImGuiKey_End },
    { KEY_DELETE, ImGuiKey_Delete },
    { KEY_BACKSPACE, ImGuiKey_Backspace },
    { KEY_ENTER, ImGuiKey_Enter },
    { KEY_KP_ENTER, ImGuiKey_KeypadEnter },
    { KEY_ESCAPE, ImGuiKey_Escape },
    { KEY_A, ImGuiKey_A }, // the Ctrl+A / C / V / X / Z shortcuts of a text field
    { KEY_C, ImGuiKey_C },
    { KEY_V, ImGuiKey_V },
    { KEY_X, ImGuiKey_X },
    { KEY_Y, ImGuiKey_Y },
    { KEY_Z, ImGuiKey_Z },
};

// ImGui 1.92 asks the backend to create and destroy its textures rather than
// handing over one atlas up front, so this runs every frame over the list.
static void UpdateTextures()
{
    for (ImTextureData *texture : ImGui::GetPlatformIO().Textures)
    {
        if (texture->Status == ImTextureStatus_WantCreate)
        {
            // raylib copies the pixels to the GPU here; ImGui keeps owning them.
            Image image = {};
            image.data = texture->GetPixels();
            image.width = texture->Width;
            image.height = texture->Height;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

            Texture2D uploaded = LoadTextureFromImage(image);
            texture->SetTexID((ImTextureID)(intptr_t)uploaded.id);
            texture->SetStatus(ImTextureStatus_OK);
        }
        else if (texture->Status == ImTextureStatus_WantUpdates)
        {
            // ImGui asks for sub-rectangles, but its Pixels array is the whole
            // texture and a rectangle out of it is not contiguous. Font atlas
            // updates are rare, so re-uploading all of it is the simpler trade.
            rlUpdateTexture((unsigned int)(intptr_t)texture->GetTexID(), 0, 0, texture->Width, texture->Height,
                            PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, texture->GetPixels());
            texture->SetStatus(ImTextureStatus_OK);
        }
        else if (texture->Status == ImTextureStatus_WantDestroy && texture->UnusedFrames > 0)
        {
            rlUnloadTexture((unsigned int)(intptr_t)texture->GetTexID());
            texture->SetTexID(ImTextureID_Invalid);
            texture->SetStatus(ImTextureStatus_Destroyed);
        }
    }
}

// The draw lists go through rlgl's immediate-mode batch rather than a shader and
// buffers of our own: ImGui draws a few thousand vertices of 2D triangles, which
// is exactly what that batch is for, and it keeps this file free of GL calls.
static void RenderDrawData(ImDrawData *drawData)
{
    if (drawData == nullptr || drawData->CmdListsCount == 0)
        return;

    // Flush whatever the game queued, so none of it inherits the state below.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlEnableScissorTest();

    const float screenHeight = (float)GetScreenHeight();
    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex)
    {
        const ImDrawList *list = drawData->CmdLists[listIndex];
        for (const ImDrawCmd &command : list->CmdBuffer)
        {
            if (command.UserCallback != nullptr)
            {
                command.UserCallback(list, &command);
                continue;
            }

            float width = command.ClipRect.z - command.ClipRect.x;
            float height = command.ClipRect.w - command.ClipRect.y;
            if (width <= 0.0f || height <= 0.0f)
                continue;

            // The batch has to be flushed before every scissor change. rlgl holds
            // vertices until something forces a draw, but rlScissor is immediate
            // GL state, so anything still queued would be clipped by the *next*
            // command's rectangle: the symptom was ImGui window title bars
            // disappearing, cut away by a scissor set further down the window.
            rlDrawRenderBatchActive();
            // rlScissor takes the OpenGL bottom-left origin, ImGui gives top-left.
            rlScissor((int)command.ClipRect.x, (int)(screenHeight - command.ClipRect.w),
                      (int)width, (int)height);

            rlSetTexture((unsigned int)(intptr_t)command.GetTexID());
            rlBegin(RL_TRIANGLES);
            for (unsigned int element = 0; element < command.ElemCount; ++element)
            {
                unsigned int index = list->IdxBuffer[command.IdxOffset + element] + command.VtxOffset;
                const ImDrawVert &vertex = list->VtxBuffer[index];
                // rlgl splits the batch at triangle boundaries by itself, so a
                // list of any length is safe between one rlBegin and rlEnd.
                const unsigned char *color = (const unsigned char *)&vertex.col;
                rlColor4ub(color[0], color[1], color[2], color[3]);
                rlTexCoord2f(vertex.uv.x, vertex.uv.y);
                rlVertex2f(vertex.pos.x, vertex.pos.y);
            }
            rlEnd();
        }
    }

    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlDisableScissorTest();
    rlEnableBackfaceCulling();
}

void imgui::Initialize()
{
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.BackendPlatformName = "raylib";
    io.BackendRendererName = "rlgl";
    // Textures are created on demand in UpdateTextures, which is the 1.92 way.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    // No imgui.ini: the panel has its own config file, and this one would land in
    // whatever directory the game happened to be launched from.
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
}

void imgui::Shutdown()
{
    // Ask for every texture back before the context goes away, otherwise the
    // atlas leaks its GPU texture.
    for (ImTextureData *texture : ImGui::GetPlatformIO().Textures)
    {
        if (texture->GetTexID() != ImTextureID_Invalid)
        {
            rlUnloadTexture((unsigned int)(intptr_t)texture->GetTexID());
            texture->SetTexID(ImTextureID_Invalid);
        }
    }

    ImGui::DestroyContext();
}

void imgui::BeginFrame(float deltaTime)
{
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)GetScreenWidth(), (float)GetScreenHeight());
    io.DeltaTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;

    Vector2 mouse = GetMousePosition();
    io.AddMousePosEvent(mouse.x, mouse.y);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, IsMouseButtonDown(MOUSE_BUTTON_RIGHT));
    io.AddMouseButtonEvent(ImGuiMouseButton_Middle, IsMouseButtonDown(MOUSE_BUTTON_MIDDLE));
    io.AddMouseWheelEvent(0.0f, GetMouseWheelMove());

    io.AddKeyEvent(ImGuiMod_Ctrl, IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));
    io.AddKeyEvent(ImGuiMod_Shift, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
    for (const auto &key : KEY_MAP)
        io.AddKeyEvent(key.imguiKey, IsKeyDown(key.raylibKey));

    for (int character = GetCharPressed(); character != 0; character = GetCharPressed())
        io.AddInputCharacter(character);

    ImGui::NewFrame();
}

void imgui::EndFrame()
{
    ImGui::Render();
    UpdateTextures();
    RenderDrawData(ImGui::GetDrawData());
}

bool imgui::WantsMouse()
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool imgui::WantsKeyboard()
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

#endif
