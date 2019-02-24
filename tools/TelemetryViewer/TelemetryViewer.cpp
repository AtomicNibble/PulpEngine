#include "stdafx.h"
#include "TelemetryViewer.h"

X_NAMESPACE_BEGIN(telemetry)


namespace
{
    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    static const char* IntTable100 =
        "00010203040506070809"
        "10111213141516171819"
        "20212223242526272829"
        "30313233343536373839"
        "40414243444546474849"
        "50515253545556575859"
        "60616263646566676869"
        "70717273747576777879"
        "80818283848586878889"
        "90919293949596979899";

    X_DISABLE_WARNING(4244)

    inline void PrintTinyInt(char*& buf, uint64_t v)
    {
        if (v >= 10)
        {
            *buf++ = '0' + v / 10;
        }
        *buf++ = '0' + v % 10;
    }

    inline void PrintTinyInt0(char*& buf, uint64_t v)
    {
        if (v >= 10)
        {
            *buf++ = '0' + v / 10;
        }
        else
        {
            *buf++ = '0';
        }
        *buf++ = '0' + v % 10;
    }

    inline void PrintSmallInt(char*& buf, uint64_t v)
    {
        if (v >= 100)
        {
            memcpy(buf, IntTable100 + v / 10 * 2, 2);
            buf += 2;
        }
        else if (v >= 10)
        {
            *buf++ = '0' + v / 10;
        }
        *buf++ = '0' + v % 10;
    }

    inline void PrintFrac00(char*& buf, uint64_t v)
    {
        *buf++ = '.';
        v += 5;
        if (v / 10 % 10 == 0)
        {
            *buf++ = '0' + v / 100;
        }
        else
        {
            memcpy(buf, IntTable100 + v / 10 * 2, 2);
            buf += 2;
        }
    }

    inline void PrintFrac0(char*& buf, uint64_t v)
    {
        *buf++ = '.';
        *buf++ = '0' + (v + 50) / 100;
    }

    inline void PrintSmallIntFrac(char*& buf, uint64_t v)
    {
        uint64_t in = v / 1000;
        uint64_t fr = v % 1000;
        if (fr >= 995)
        {
            PrintSmallInt(buf, in + 1);
        }
        else
        {
            PrintSmallInt(buf, in);
            if (fr > 5)
            {
                PrintFrac00(buf, fr);
            }
        }
    }

    inline void PrintSecondsFrac(char*& buf, uint64_t v)
    {
        uint64_t in = v / 1000;
        uint64_t fr = v % 1000;
        if (fr >= 950)
        {
            PrintTinyInt0(buf, in + 1);
        }
        else
        {
            PrintTinyInt0(buf, in);
            if (fr > 50)
            {
                PrintFrac0(buf, fr);
            }
        }
    }

    const char* TimeToString(int64_t _ns)
    {
        enum { Pool = 8 };
        static char bufpool[Pool][64];
        static int bufsel = 0;
        char* buf = bufpool[bufsel];
        char* bufstart = buf;
        bufsel = (bufsel + 1) % Pool;

        uint64_t ns;
        if (_ns < 0)
        {
            *buf = '-';
            buf++;
            ns = -_ns;
        }
        else
        {
            ns = _ns;
        }

        if (ns < 1000)
        {
            PrintSmallInt(buf, ns);
            memcpy(buf, " ns", 4);
        }
        else if (ns < 1000ll * 1000)
        {
            PrintSmallIntFrac(buf, ns);
#ifdef TRACY_EXTENDED_FONT
            memcpy(buf, " \xce\xbcs", 5);
#else
            memcpy(buf, " us", 4);
#endif
        }
        else if (ns < 1000ll * 1000 * 1000)
        {
            PrintSmallIntFrac(buf, ns / 1000);
            memcpy(buf, " ms", 4);
        }
        else if (ns < 1000ll * 1000 * 1000 * 60)
        {
            PrintSmallIntFrac(buf, ns / (1000ll * 1000));
            memcpy(buf, " s", 3);
        }
        else if (ns < 1000ll * 1000 * 1000 * 60 * 60)
        {
            const auto m = int64_t(ns / (1000ll * 1000 * 1000 * 60));
            const auto s = int64_t(ns - m * (1000ll * 1000 * 1000 * 60)) / (1000ll * 1000);
            PrintTinyInt(buf, m);
            *buf++ = ':';
            PrintSecondsFrac(buf, s);
            *buf++ = '\0';
        }
        else if (ns < 1000ll * 1000 * 1000 * 60 * 60 * 24)
        {
            const auto h = int64_t(ns / (1000ll * 1000 * 1000 * 60 * 60));
            const auto m = int64_t(ns / (1000ll * 1000 * 1000 * 60) - h * 60);
            const auto s = int64_t(ns / (1000ll * 1000 * 1000) - h * (60 * 60) - m * 60);
            PrintTinyInt(buf, h);
            *buf++ = ':';
            PrintTinyInt0(buf, m);
            *buf++ = ':';
            PrintTinyInt0(buf, s);
            *buf++ = '\0';
        }
        else
        {
            const auto d = int64_t(ns / (1000ll * 1000 * 1000 * 60 * 60 * 24));
            const auto h = int64_t(ns / (1000ll * 1000 * 1000 * 60 * 60) - d * 24);
            const auto m = int64_t(ns / (1000ll * 1000 * 1000 * 60) - d * (60 * 24) - h * 60);
            const auto s = int64_t(ns / (1000ll * 1000 * 1000) - d * (60 * 60 * 24) - h * (60 * 60) - m * 60);
            if (d < 1000)
            {
                PrintSmallInt(buf, d);
                *buf++ = 'd';
            }
            else
            {
                buf += sprintf(buf, "%" PRIi64 "d", d);
            }
            PrintTinyInt0(buf, h);
            *buf++ = ':';
            PrintTinyInt0(buf, m);
            *buf++ = ':';
            PrintTinyInt0(buf, s);
            *buf++ = '\0';
        }
        return bufstart;
    }

    const char* RealToString(double val, bool separator)
    {
        enum { Pool = 8 };
        static char bufpool[Pool][64];
        static int bufsel = 0;
        char* buf = bufpool[bufsel];
        bufsel = (bufsel + 1) % Pool;

        sprintf(buf, "%f", val);
        auto ptr = buf;
        if (*ptr == '-') ptr++;

        const auto vbegin = ptr;

        if (separator)
        {
            while (*ptr != '\0' && *ptr != ',' && *ptr != '.') ptr++;
            auto end = ptr;
            while (*end != '\0') end++;
            auto sz = end - ptr;

            while (ptr - vbegin > 3)
            {
                ptr -= 3;
                memmove(ptr + 1, ptr, sz);
                *ptr = ',';
                sz += 4;
            }
        }

        while (*ptr != '\0' && *ptr != ',' && *ptr != '.') ptr++;

        if (*ptr == '\0') return buf;
        while (*ptr != '\0') ptr++;
        ptr--;
        while (*ptr == '0') ptr--;
        if (*ptr != '.' && *ptr != ',') ptr++;
        *ptr = '\0';
        return buf;
    }

    void TextDisabledUnformatted(const char* begin, const char* end = nullptr)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GImGui->Style.Colors[ImGuiCol_TextDisabled]);
        ImGui::TextUnformatted(begin, end);
        ImGui::PopStyleColor();
    }

    void TextFocused(const char* label, const char* value)
    {
        TextDisabledUnformatted(label);
        ImGui::SameLine();
        ImGui::TextUnformatted(value);
    }

    void DrawHelpMarker(const char* desc)
    {
        TextDisabledUnformatted("(?)");
        if (ImGui::IsItemHovered())
        {
            const auto ty = ImGui::GetFontSize();
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(450.0f * ty / 15.f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void DrawTextContrast(ImDrawList* draw, const ImVec2& pos, uint32_t color, const char* text)
    {
        draw->AddText(pos + ImVec2(1, 1), 0x88000000, text);
        draw->AddText(pos, color, text);
    }


    bool Splitter(bool split_vertically, float thickness, float* size1, float* size2,
        float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + ImGui::CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return ImGui::SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    // ---------------------------


    int32_t GetFrameWidth(int32_t frameScale)
    {
        return frameScale == 0 ? 4 : (frameScale < 0 ? 6 : 1);
    }

    int32_t GetFrameGroup(int32_t frameScale)
    {
        return frameScale < 2 ? 1 : (1 << (frameScale - 1));
    }

    ImU32 GetFrameColor(uint64_t frameTime)
    {
        enum { BestTime = 1000 * 1000 * 1000 / 143 };
        enum { GoodTime = 1000 * 1000 * 1000 / 59 };
        enum { BadTime = 1000 * 1000 * 1000 / 29 };

        return frameTime > BadTime ? 0xFF2222DD :
            frameTime > GoodTime ? 0xFF22DDDD :
            frameTime > BestTime ? 0xFF22DD22 : 0xFFDD9900;
    }

    int64_t GetFrameTime(size_t idx)
    {
        X_UNUSED(idx);
        return 1000 * 1000 * (10 + (idx % 10));
    }

    int64_t GetFrameBegin(size_t idx)
    {
        X_UNUSED(idx);
        return 1000 * 1000 * (10 + (idx % 10));
    }

    int64_t GetFrameEnd(size_t idx)
    {
        X_UNUSED(idx);
        return GetFrameBegin(idx) + 1024;
    }

    int64_t GetTimeBegin(void)
    {
        return GetFrameBegin(0);
    }

    uint64_t GetFrameOffset()
    {
        return 0;
    }

    std::pair<int, int> GetFrameRange(int64_t from, int64_t to)
    {
        X_UNUSED(from, to);
        return { 10, 40 };
    }

    const char* GetThreadString(uint64_t id)
    {
        X_UNUSED(id);
        return "meow";
    }



} // namespace

bool connectToServer(Client& client);



void DrawFrame(Client& client, float ww, float wh)
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

    if (show_app_metrics) { 
        ImGui::ShowMetricsWindow(&show_app_metrics);
    }
    if (show_app_style_editor) { 
        ImGui::Begin("Style Editor", &show_app_style_editor); 
        ImGui::ShowStyleEditor(); 
        ImGui::End(); 
    }
    if (show_app_about) { 
        ImGui::ShowAboutWindow(&show_app_about); 
    }

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

        // keep the sidebar simular size.
        if (ww < sz1) {
            float leftR = sz1 / lastWidth;
            float rightR = sz2 / lastWidth;

            sz1 = ww * leftR;
            sz2 = ww * rightR;
        }
        else {
            sz2 = ww * sz1;
        }

        lastWidth = ww;
    }

    Splitter(true, 4.0f, &sz1, &sz2, minWorkspaceWidth, minTracesWidth);
    {
        ImGui::BeginChild("1", ImVec2(sz1, -1), false);

        if (ImGui::BeginTabBar("Main Tabs"))
        {
            if (ImGui::BeginTabItem("Traces", nullptr, 0))
            {
                if (client.isConnected())
                {
                    // draw me a list like a pickle in the wind.
                    for (const auto& app : client.apps)
                    {
                        if (ImGui::CollapsingHeader(app.appName.c_str()))
                        {
                            // ImGui::Text(app.appName.c_str());
                            // ImGui::Text("Num %" PRIuS, app.traces.size());
                            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.2f, 0.2f, 0.2f));

                            static int selected = -1;

                            for (int32_t i = 0; i < static_cast<int32_t>(app.traces.size()); i++)
                            {
                                const auto& trace = app.traces[i];

                                if (ImGui::Selectable(trace.name.c_str(), selected == i))
                                {
                                    selected = i;
                                }
                            }

                            ImGui::PopStyleColor(1);
                        }
                    }
                }
                else
                {
                    // show some connect button.
                    ImGui::Separator();
                    ImGui::TextUnformatted("Connect to server");

                    char addr[256] = { "127.0.0.1" };

                    bool connectClicked = false;
                    connectClicked |= ImGui::InputText("", addr, sizeof(addr), ImGuiInputTextFlags_EnterReturnsTrue);
                    connectClicked |= ImGui::Button("Connect");

                    if (connectClicked && *addr)
                    {
                        if (!connectToServer(client))
                        {

                        }
                    }
                }

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
            //    DrawFrames();
            //    DrawZones();

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


bool handleDataSream(Client& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const DataStreamHdr*>(pData);
    if (pHdr->type != PacketType::DataStream) {
        X_ASSERT_UNREACHABLE();
    }

    // the data is compressed.
    // decompress it..
    int32_t cmpLen = pHdr->dataSize - sizeof(DataStreamHdr);
    int32_t origLen = pHdr->origSize - sizeof(DataStreamHdr);
    X_UNUSED(cmpLen);

    if (cmpLen == origLen) {
        // uncompressed packets.
        X_ASSERT_NOT_IMPLEMENTED();
    }

    auto* pDst = &client.cmpRingBuf[client.cmpBufferOffset];

    int32_t cmpLenOut = static_cast<int32_t>(client.lz4DecodeStream.decompressContinue(pHdr + 1, pDst, origLen));
    if (cmpLenOut != cmpLen) {
        X_ERROR("TelemViewer", "Failed to inflate data stream");
        return false;
    }

    client.cmpBufferOffset += origLen;
    if (client.cmpBufferOffset >= (COMPRESSION_RING_BUFFER_SIZE - COMPRESSION_MAX_INPUT_SIZE)) {
        client.cmpBufferOffset = 0;
    }

    for (int32 i = 0; i < origLen; )
    {
        auto* pPacket = reinterpret_cast<const DataPacketBase*>(&pDst[i]);

        switch (pPacket->type)
        {
            case DataStreamType::StringTableAdd:
            case DataStreamType::Zone:
            case DataStreamType::TickInfo:
                break;

            default:
                X_NO_SWITCH_DEFAULT_ASSERT;
        }
    }

    return true;
}


bool processPacket(Client& client, uint8_t* pData)
{
    auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);

    switch (pPacketHdr->type)
    {
        case PacketType::DataStream:
            return handleDataSream(client, pData);
            break;
        default:
            X_ERROR("TelemViewer", "Unknown packet type %" PRIi32, static_cast<int>(pPacketHdr->type));
            return false;
    }
}


void sendDataToServer(Client& client, const void* pData, int32_t len)
{
#if X_DEBUG
    if (len > MAX_PACKET_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    // send some data...
    // TODO: none blocking?
    int res = platform::send(client.socket, reinterpret_cast<const char*>(pData), len, 0);
    if (res == SOCKET_ERROR) {
        X_ERROR("Telem", "Socket: send failed with error: %d", platform::WSAGetLastError());
        return;
    }
}

bool readPacket(Client& client, char* pBuffer, int& bufLengthInOut)
{
    // this should return complete packets or error.
    int bytesRead = 0;
    int bufLength = sizeof(PacketBase);

    while (1) {
        int maxReadSize = bufLength - bytesRead;
        int res = platform::recv(client.socket, &pBuffer[bytesRead], maxReadSize, 0);

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

bool handleConnectionResponse(Client& client, tt_uint8* pData, tt_size len)
{
    X_UNUSED(client, len);

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

Client::Client(core::MemoryArenaBase* arena) :
    apps(arena)
{
    socket = INV_SOCKET;
}

bool Client::isConnected(void) const 
{
    return socket != INV_SOCKET;
}

bool connectToServer(Client& client)
{
    if (client.isConnected()) {
        // TODO:
        return false;
    }

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
    cr.dataSize = sizeof(cr);
    cr.type = PacketType::ConnectionRequestViewer;
    cr.viewerVer.major = TELEM_VERSION_MAJOR;
    cr.viewerVer.minor = TELEM_VERSION_MINOR;
    cr.viewerVer.patch = TELEM_VERSION_PATCH;
    cr.viewerVer.build = TELEM_VERSION_BUILD;

    client.socket = connectSocket;
    sendDataToServer(client, &cr, sizeof(cr));

    // wait for a response O.O
    char recvbuf[MAX_PACKET_SIZE];
    int recvbuflen = sizeof(recvbuf);

    // TODO: support timeout.
    if (!readPacket(client, recvbuf, recvbuflen)) {
        return false;
    }

    if (!handleConnectionResponse(client, reinterpret_cast<tt_uint8*>(recvbuf), static_cast<tt_size>(recvbuflen))) {
        return false;
    }

    return true;
}

bool run(Client& client)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        X_ERROR("", "Error: %s", SDL_GetError());
        return false;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    auto pWindow = SDL_CreateWindow("TelemetryViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(pWindow);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = gl3wInit() != 0;
    if (err) {
        X_ERROR("Telem", "Failed to initialize OpenGL loader!");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(pWindow, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    const ImVec4 clearColor = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);

    bool done = false;

    // try connect to server
    (void)connectToServer(client);

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                done = true;
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(pWindow)) {
                done = true;
            }

        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(pWindow);
        ImGui::NewFrame();

        DrawFrame(client, io.DisplaySize.x, io.DisplaySize.y);
        ImGui::ShowDemoWindow();

        ImGui::Render();
        SDL_GL_MakeCurrent(pWindow, gl_context);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(pWindow);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();
    return true;
}

X_NAMESPACE_END