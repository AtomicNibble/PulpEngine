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

    bool winSockInit(void)
    {
        platform::WSADATA winsockInfo;

        if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
            return false;
        }

        return true;
    }

    void winSockShutDown(void)
    {
        if (platform::WSACleanup() != 0) {
            // rip
            return;
        }
    }


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

    struct Server
    {
        platform::SOCKET socket;
    };

    void sendDataToServer(Server& srv, const void* pData, int32_t len)
    {
#if X_DEBUG
        if (len > MAX_PACKET_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        // send some data...
        // TODO: none blocking?
        int res = platform::send(srv.socket, reinterpret_cast<const char*>(pData), len, 0);
        if (res == SOCKET_ERROR) {
            X_ERROR("Telem", "Socket: send failed with error: %d", platform::WSAGetLastError());
            return;
        }
    }

    bool handleConnectionResponse(Server& srv, tt_uint8* pData, tt_size len)
    {
        X_UNUSED(srv, len);

        auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);
        switch (pPacketHdr->type)
        {
            case PacketType::ConnectionRequestAccepted:
                // don't care about response currently.
                return true;
            case PacketType::ConnectionRequestRejected: {
                auto* pConRej = reinterpret_cast<const ConnectionRequestRejectedHdr*>(pData);
                auto* pStrData = reinterpret_cast<const char*>(pConRej + 1);
                X_ERROR("Telem", "Connection rejected: %.*s", pConRej->reasonLen, pStrData);
            }
            default:
                return false;
        }
    }

    bool readPacket(Server& srv, char* pBuffer, int& bufLengthInOut)
    {
        // this should return complete packets or error.
        int bytesRead = 0;
        int bufLength = sizeof(PacketBase);

        while (1) {
            int maxReadSize = bufLength - bytesRead;
            int res = platform::recv(srv.socket, &pBuffer[bytesRead], maxReadSize, 0);

            if (res == 0) {
                X_ERROR("Telem", "Connection closing...");
                return false;
            }
            else if (res < 0) {
                X_ERROR("Telem", "recv failed with error: %d", platform::WSAGetLastError());
                return false;
            }

            bytesRead += res;

            X_LOG0("Telem", "got: %d bytes\n", res);

            if (bytesRead == sizeof(PacketBase))
            {
                auto* pHdr = reinterpret_cast<const PacketBase*>(pBuffer);
                if (pHdr->dataSize == 0) {
                    X_ERROR("Telem", "Client sent packet with length zero...");
                    return false;
                }

                if (pHdr->dataSize > bufLengthInOut) {
                    X_ERROR("Telem", "Client sent oversied packet of size %i...", static_cast<tt_int32>(pHdr->dataSize));
                    return false;
                }

                bufLength = pHdr->dataSize;
            }

            if (bytesRead == bufLength) {
                bufLengthInOut = bytesRead;
                return true;
            }
            else if (bytesRead > bufLength) {
                X_ERROR("Telem", "Overread packet bytesRead: %d recvbuflen: %d", bytesRead, bufLength);
                return false;
            }
        }
    }

    bool connectToServer()
    {
        const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

        struct platform::addrinfo hints, *servinfo = nullptr;
        core::zero_object(hints);
        hints.ai_family = AF_UNSPEC; // ipv4/6
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = platform::IPPROTO_TCP;

        // Resolve the server address and port
        auto res = platform::getaddrinfo("127.0.0.1", "8001", &hints, &servinfo);
        if (res != 0) {
            X_ERROR("Telem", "Failed to get addre info. Error: %d", platform::WSAGetLastError());
            return false;
        }

        platform::SOCKET connectSocket = INV_SOCKET;

        for (auto pPtr = servinfo; pPtr != nullptr; pPtr = pPtr->ai_next) {
            // Create a SOCKET for connecting to server
            connectSocket = platform::socket(pPtr->ai_family, pPtr->ai_socktype, pPtr->ai_protocol);
            if (connectSocket == INV_SOCKET) {
                return false;
            }

            // Connect to server.
            res = connect(connectSocket, pPtr->ai_addr, static_cast<int>(pPtr->ai_addrlen));
            if (res == SOCKET_ERROR) {
                platform::closesocket(connectSocket);
                connectSocket = INV_SOCKET;
                continue;
            }

            break;
        }

        platform::freeaddrinfo(servinfo);

        if (connectSocket == INV_SOCKET) {
            return false;
        }

        // how big?
        tt_int32 sock_opt = 1024 * 16;
        res = platform::setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
        if (res != 0) {
            X_ERROR("Telem", "Failed to set sndbuf on socket. Error: %d", platform::WSAGetLastError());
            return false;
        }

        ConnectionRequestViewerHdr cr;
        core::zero_object(cr);
        cr.type = PacketType::ConnectionRequestViewer;
        cr.viewerVer.major = TELEM_VERSION_MAJOR;
        cr.viewerVer.minor = TELEM_VERSION_MINOR;
        cr.viewerVer.patch = TELEM_VERSION_PATCH;
        cr.viewerVer.build = TELEM_VERSION_BUILD;

        Server srv;
        srv.socket = connectSocket;

        sendDataToServer(srv, &cr, sizeof(cr));

        // wait for a response O.O
        char recvbuf[MAX_PACKET_SIZE];
        int recvbuflen = sizeof(recvbuf);

        // TODO: support timeout.
        if (!readPacket(srv, recvbuf, recvbuflen)) {
            return false;
        }

        if (!handleConnectionResponse(srv, reinterpret_cast<tt_uint8*>(recvbuf), static_cast<tt_size>(recvbuflen))) {
            return false;
        }

        return true;
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

        if (!winSockInit()) {
            return 1;
        }

        // TEMP: connect to server on start up.
        connectToServer();

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

        winSockShutDown();
    }

    return 0;
}


