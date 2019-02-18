#include "stdafx.h"
#include "EngineApp.h"

#include <../TelemetryServerLib/TelemetryServerLib.h>

#define _LAUNCHER
#include <ModuleExports.h>

// SDL
#include <SDL.h>

// imgui
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>
#include <examples/libs/gl3w/GL/gl3w.h>


#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_FORCE_LINK_FACTORY("XTelemSrvLib");

#endif // !X_LIB

X_LINK_ENGINE_LIB("TelemetryServerLib");

X_LINK_LIB("opengl32.lib");
X_LINK_LIB("SDL2.lib");
// X_LINK_LIB("SDL2main.lib");

namespace
{
    typedef core::MemoryArena<
        core::MallocFreeAllocator,
        core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
    >
        TelemetryViewerArena;

    bool Splitter(bool split_vertically, float thickness, float* size1, float* size2,
        float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    static void DrawFrame(float ww, float wh)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ww, wh));
    //    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
        
        static bool show_app_metrics = false;
        static bool show_app_style_editor = false;
        static bool show_app_about = false;


        if (show_app_metrics) { ImGui::ShowMetricsWindow(&show_app_metrics); }
        if (show_app_style_editor) { ImGui::Begin("Style Editor", &show_app_style_editor); ImGui::ShowStyleEditor(); ImGui::End(); }
        if (show_app_about) { ImGui::ShowAboutWindow(&show_app_about); }

        ImGui::Begin("##TelemetryViewerMain", nullptr,
            ImGuiWindowFlags_NoBringToFrontOnFocus | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_MenuBar);

        // Menu
        ImVec2 menuBarSize;

        if (ImGui::BeginMainMenuBar())
        {
            menuBarSize = ImGui::GetWindowSize();
            if (ImGui::BeginMenu("Menu"))
            {
                if (ImGui::MenuItem("Exit", "Alt+F4")) {

                }
                ImGui::EndMenu();
            }
           
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::MenuItem("Metrics", NULL, &show_app_metrics);
                ImGui::MenuItem("Style Editor", NULL, &show_app_style_editor);
                ImGui::MenuItem("About Dear ImGui", NULL, &show_app_about);
                ImGui::MenuItem("About", NULL, &show_app_about);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

    //    float h = wh - menuBarSize.y;
        const float defauktWorkspaceWidth = 300.f;
        const float minWorkspaceWidth = 128.f;
        const float minTracesWidth = 256.f;

        static float sz1 = defauktWorkspaceWidth;
        static float sz2 = ww - sz1;

        static float lastWidth = ww;

        if (ww != lastWidth) {

            // lets just build a new ratio
            float leftR = sz1 / lastWidth;
            float rightR = sz2 / lastWidth;

            sz1 = ww * leftR;
            sz2 = ww * rightR;

            lastWidth = ww;
        }

        Splitter(true, 4.0f, &sz1, &sz2, minWorkspaceWidth, minTracesWidth);
        {
            ImGui::BeginChild("1", ImVec2(sz1, -1), false);

            if (ImGui::BeginTabBar("Main Tabs"))
            {
                if (ImGui::BeginTabItem("Traces", nullptr, 0))
                {
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings", nullptr, 0))
                {

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::EndChild();
        }
        ImGui::SameLine();
        {
            ImGui::BeginChild("2", ImVec2(sz2, -1), false);

            if (ImGui::BeginTabBar("View Tabs"))
            {
                if (ImGui::BeginTabItem("Traces", nullptr, 0))
                {
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    }

} // namespace



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hPrevInstance);
    X_UNUSED(nCmdShow);

    TelemetryViewerArena::AllocationPolicy allocator;
    TelemetryViewerArena arena(&allocator, "TelemetryViewerArena");

    {
        EngineApp app;

        if (!app.Init(hInstance, &arena, lpCmdLine)) {
            return 1;
        }

        // should the viewer always just talk to server via tcp? or do we have a in memory mode?
        // I dunno support both be nice but fiddly?

        // Setup SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        {
            X_ERROR("", "Error: %s", SDL_GetError());
            return -1;
        }

        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        SDL_DisplayMode current;
        SDL_GetCurrentDisplayMode(0, &current);
        auto window = SDL_CreateWindow("TelemetryViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        SDL_GL_SetSwapInterval(1); // Enable vsync

                // Initialize OpenGL loader
        bool err = gl3wInit() != 0;
        if (err) {
            X_ERROR("", "Failed to initialize OpenGL loader!");
            return 1;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); 
        (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL3_Init(glsl_version);

        const ImVec4 clearColor = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);

        bool done = false;
        while (!done)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);

                if (event.type == SDL_QUIT) {
                    done = true;
                }
                else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                    done = true;
                }

            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();

            DrawFrame(io.DisplaySize.x, io.DisplaySize.y);

            ImGui::Render();
            SDL_GL_MakeCurrent(window, gl_context);
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }

        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    return 0;
}


