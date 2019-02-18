#include "stdafx.h"
#include "EngineApp.h"

#include <../TelemetryServerLib/TelemetryServerLib.h>

#define _LAUNCHER
#include <ModuleExports.h>

// SDL
#include <SDL.h>

// imgui
#include <imgui.h>
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
        //const char* glsl_version = "#version 130";
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

         //   ImGui::ShowDemoWindow(nullptr);

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


