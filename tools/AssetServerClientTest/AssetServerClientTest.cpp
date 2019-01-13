#include "stdafx.h"
#include "EngineApp.h"

#include <Platform\Pipe.h>
#include <Platform\Console.h>

#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")

// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")
X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");

#endif // !X_LIB

#include "..\protobuf\src\assetdb.pb.h"

#if X_DEBUG
X_LINK_LIB("libprotobufd")
#else
X_LINK_LIB("libprotobuf")
#endif // !X_DEBUG

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
    AssetServerTestArena;

core::MemoryArenaBase* g_arena = nullptr;

X_DISABLE_WARNING(4244)
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
X_ENABLE_WARNING(4244)

bool ReadDelimitedFrom(google::protobuf::io::ZeroCopyInputStream* rawInput,
    google::protobuf::MessageLite* message, bool* cleanEof)
{
    google::protobuf::io::CodedInputStream input(rawInput);
    const int start = input.CurrentPosition();
    if (cleanEof) {
        *cleanEof = false;
    }

    // Read the size.
    uint32_t size;
    if (!input.ReadVarint32(&size)) {
        if (cleanEof) {
            *cleanEof = input.CurrentPosition() == start;
        }
        return false;
    }
    // Tell the stream not to read beyond that size.
    google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);

    // Parse the message.
    if (!message->MergeFromCodedStream(&input)) {
        return false;
    }
    if (!input.ConsumedEntireMessage()) {
        return false;
    }

    // Release the limit.
    input.PopLimit(limit);
    return true;
}

bool WriteDelimitedTo(const google::protobuf::MessageLite& message,
    google::protobuf::io::ZeroCopyOutputStream* rawOutput)
{
    // We create a new coded stream for each message.
    google::protobuf::io::CodedOutputStream output(rawOutput);

    // Write the size.
    const int size = message.ByteSize();
    output.WriteVarint32(size);

    uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
    if (buffer != nullptr) {
        // Optimization:  The message fits in one buffer, so use the faster
        // direct-to-array serialization path.
        message.SerializeWithCachedSizesToArray(buffer);
    }

    else {
        // Slightly-slower path when the message is multiple buffers.
        message.SerializeWithCachedSizes(&output);
        if (output.HadError()) {
            X_ERROR("Proto", "Failed to write msg to output stream");
            return false;
        }
    }

    return true;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    {
        core::MallocFreeAllocator allocator;
        AssetServerTestArena arena(&allocator, "AssetServerTestArena");
        g_arena = &arena;

        bool res = false;

        EngineApp engine;

        if (engine.Init(hInstance, lpCmdLine))
        {
            gEnv->pConsoleWnd->redirectSTD();

            core::IPC::Pipe pipe;

            if (pipe.open("\\\\.\\pipe\\" X_ENGINE_NAME "_AssetServer",
                    core::IPC::Pipe::OpenMode::READ | core::IPC::Pipe::OpenMode::WRITE | core::IPC::Pipe::OpenMode::SHARE)) {
                GOOGLE_PROTOBUF_VERIFY_VERSION;

                // connect to me baby.
                ULONG serverId;
                ULONG serverSessionId;

                pipe.getServerProcessId(&serverId);
                pipe.getServerSessionId(&serverSessionId);

                X_LOG0("AssetClientTest", "ServerId: %i", serverId);
                X_LOG0("AssetClientTest", "ServerSessionId: %i", serverSessionId);

                size_t bytesRead;
                const size_t bufLength = 0x200;
                uint8_t buffer[bufLength];

                bool cleanEof;

                // write the delimited msg to the buffer.
                {
                    ProtoBuf::AssetDB::AddAsset* pAdd = new ProtoBuf::AssetDB::AddAsset();
                    pAdd->set_name("test_asset");
                    pAdd->set_type(ProtoBuf::AssetDB::AssetType::MODEL);

                    ProtoBuf::AssetDB::Request request;
                    request.set_allocated_add(pAdd);

                    google::protobuf::io::ArrayOutputStream arrayOutput(buffer, bufLength);
                    WriteDelimitedTo(request, &arrayOutput);

                    if (!pipe.write(buffer, safe_static_cast<size_t, int64_t>(arrayOutput.ByteCount()))) {
                        X_ERROR("AssetClientTest", "failed to write buffer");
                    }
                    if (!pipe.flush()) {
                        X_ERROR("AssetClientTest", "failed to flush pipe");
                    }
                }

                // wait for the response.
                {
                    if (!pipe.read(buffer, sizeof(buffer), &bytesRead)) {
                        X_ERROR("AssetClientTest", "failed to read response");
                    }

                    google::protobuf::io::ArrayInputStream arrayInput(buffer,
                        safe_static_cast<int32_t, size_t>(bytesRead));

                    ProtoBuf::AssetDB::Reponse response;

                    if (!ReadDelimitedFrom(&arrayInput, &response, &cleanEof)) {
                        X_ERROR("AssetClientTest", "Failed to read response msg");
                    }

                    if (response.result() != ProtoBuf::AssetDB::Result::OK) {
                        const std::string err = response.error();
                        X_ERROR("AssetClientTest", "Request failed: %s", err.c_str());
                    }
                    else {
                        X_LOG0("AssetClientTest", "Reponse returned OK");
                    }
                }

                // write the delimited msg to the buffer.
                {
                    ProtoBuf::AssetDB::AddAsset* pAdd = new ProtoBuf::AssetDB::AddAsset();
                    pAdd->set_name("test_asset");
                    pAdd->set_type(ProtoBuf::AssetDB::AssetType::ANIM);

                    ProtoBuf::AssetDB::Request request;
                    request.set_allocated_add(pAdd);

                    google::protobuf::io::ArrayOutputStream arrayOutput(buffer, bufLength);
                    WriteDelimitedTo(request, &arrayOutput);

                    if (!pipe.write(buffer, safe_static_cast<size_t, int64_t>(arrayOutput.ByteCount()))) {
                        X_ERROR("AssetClientTest", "failed to write buffer");
                    }
                    if (!pipe.flush()) {
                        X_ERROR("AssetClientTest", "failed to flush pipe");
                    }
                }

                // wait for the response.
                {
                    if (!pipe.read(buffer, sizeof(buffer), &bytesRead)) {
                        X_ERROR("AssetClientTest", "failed to read response");
                    }

                    google::protobuf::io::ArrayInputStream arrayInput(buffer,
                        safe_static_cast<int32_t, size_t>(bytesRead));

                    ProtoBuf::AssetDB::Reponse response;

                    if (!ReadDelimitedFrom(&arrayInput, &response, &cleanEof)) {
                        X_ERROR("AssetClientTest", "Failed to read response msg");
                    }

                    if (response.result() != ProtoBuf::AssetDB::Result::OK) {
                        const std::string err = response.error();
                        X_ERROR("AssetClientTest", "Request failed: %s", err.c_str());
                    }
                    else {
                        X_LOG0("AssetClientTest", "Reponse returned OK");
                    }
                }

                // write the delimited msg to the buffer.
                {
                    ProtoBuf::AssetDB::AddAsset* pAdd = new ProtoBuf::AssetDB::AddAsset();
                    pAdd->set_name("dance_sexy");
                    pAdd->set_type(ProtoBuf::AssetDB::AssetType::ANIM);

                    ProtoBuf::AssetDB::Request request;
                    request.set_allocated_add(pAdd);

                    google::protobuf::io::ArrayOutputStream arrayOutput(buffer, bufLength);
                    WriteDelimitedTo(request, &arrayOutput);

                    if (!pipe.write(buffer, safe_static_cast<size_t, int64_t>(arrayOutput.ByteCount()))) {
                        X_ERROR("AssetClientTest", "failed to write buffer");
                    }
                    if (!pipe.flush()) {
                        X_ERROR("AssetClientTest", "failed to flush pipe");
                    }
                }

                // wait for the response.
                {
                    if (!pipe.read(buffer, sizeof(buffer), &bytesRead)) {
                        X_ERROR("AssetClientTest", "failed to read response");
                    }

                    google::protobuf::io::ArrayInputStream arrayInput(buffer,
                        safe_static_cast<int32_t, size_t>(bytesRead));

                    ProtoBuf::AssetDB::Reponse response;

                    if (!ReadDelimitedFrom(&arrayInput, &response, &cleanEof)) {
                        X_ERROR("AssetClientTest", "Failed to read response msg");
                    }

                    if (response.result() != ProtoBuf::AssetDB::Result::OK) {
                        const std::string err = response.error();
                        X_ERROR("AssetClientTest", "Request failed: %s", err.c_str());
                    }
                    else {
                        X_LOG0("AssetClientTest", "Reponse returned OK");
                    }
                }

                // write the delimited msg to the buffer.
                {
                    ProtoBuf::AssetDB::RenameAsset* pRename = new ProtoBuf::AssetDB::RenameAsset();
                    pRename->set_name("dance_sexy");
                    pRename->set_newname("dance_sexy_new");
                    pRename->set_type(ProtoBuf::AssetDB::AssetType::ANIM);

                    ProtoBuf::AssetDB::Request request;
                    request.set_allocated_rename(pRename);

                    google::protobuf::io::ArrayOutputStream arrayOutput(buffer, bufLength);
                    WriteDelimitedTo(request, &arrayOutput);

                    if (!pipe.write(buffer, safe_static_cast<size_t, int64_t>(arrayOutput.ByteCount()))) {
                        X_ERROR("AssetClientTest", "failed to write buffer");
                    }
                    if (!pipe.flush()) {
                        X_ERROR("AssetClientTest", "failed to flush pipe");
                    }
                }

                // wait for the response.
                {
                    if (!pipe.read(buffer, sizeof(buffer), &bytesRead)) {
                        X_ERROR("AssetClientTest", "failed to read response");
                    }

                    google::protobuf::io::ArrayInputStream arrayInput(buffer,
                        safe_static_cast<int32_t, size_t>(bytesRead));

                    ProtoBuf::AssetDB::Reponse response;

                    if (!ReadDelimitedFrom(&arrayInput, &response, &cleanEof)) {
                        X_ERROR("AssetClientTest", "Failed to read response msg");
                    }

                    if (response.result() != ProtoBuf::AssetDB::Result::OK) {
                        const std::string err = response.error();
                        X_ERROR("AssetClientTest", "Request failed: %s", err.c_str());
                    }
                    else {
                        X_LOG0("AssetClientTest", "Reponse returned OK");
                    }
                }
            }

            // shut down the slut.
            google::protobuf::ShutdownProtobufLibrary();

            // muh consolas!
            gEnv->pConsoleWnd->pressToContinue();
        }

        engine.ShutDown();
    }

    return 0;
}
