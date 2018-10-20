#include "stdafx.h"

#include "Console.h"
#include "ConsoleVariable.h"

#include "String\StringTokenizer.h"
#include "Platform\ClipBoard.h"
#include "Containers\FixedArray.h"

#include "Memory\VirtualMem.h"

#include <ITimer.h>
#include <ICore.h>
#include <IFont.h>
#include <IRender.h>
#include <ILog.h>
#include <ICore.h>
#include <IFileSys.h>
#include <IFrameData.h>
#include <IPrimativeContext.h>
#include <IFont.h>
#include <I3DEngine.h>
#include <IConfig.h>

#include "Platform\Window.h"

#include <algorithm>

X_NAMESPACE_BEGIN(core)

namespace
{
    static const size_t VAR_ALLOCATION_SIZE = core::Max(
        core::Max(
            core::Max(
                core::Max(
                    core::Max(
                        core::Max(
                            core::Max(
                                core::Max(
                                    sizeof(CVarString<CVarBaseConst>),
                                    sizeof(CVarInt<CVarBaseConst>)),
                                sizeof(CVarFloat<CVarBaseConst>)),
                            sizeof(CVarString<CVarBaseHeap>)),
                        sizeof(CVarInt<CVarBaseHeap>)),
                    sizeof(CVarFloat<CVarBaseHeap>)),
                sizeof(CVarFloatRef)),
            sizeof(CVarIntRef)),
        sizeof(CVarColRef));

    static const size_t VAR_ALLOCATION_ALIGNMENT = core::Max(
        core::Max(
            core::Max(
                core::Max(
                    core::Max(
                        core::Max(
                            core::Max(
                                core::Max(
                                    X_ALIGN_OF(CVarString<CVarBaseConst>),
                                    X_ALIGN_OF(CVarInt<CVarBaseConst>)),
                                X_ALIGN_OF(CVarFloat<CVarBaseConst>)),
                            X_ALIGN_OF(CVarString<CVarBaseHeap>)),
                        X_ALIGN_OF(CVarInt<CVarBaseHeap>)),
                    X_ALIGN_OF(CVarFloat<CVarBaseHeap>)),
                X_ALIGN_OF(CVarFloatRef)),
            X_ALIGN_OF(CVarIntRef)),
        X_ALIGN_OF(CVarColRef));

    static void sortVarsByName(core::Array<core::ICVar*>& vars)
    {
        using namespace std;

        std::sort(vars.begin(), vars.end(),
            [](core::ICVar* a, core::ICVar* b) {
                return strcmp(a->GetName(), b->GetName()) < 0;
            });
    }

    static void sortCmdsByName(core::Array<core::ConsoleCommand*>& vars)
    {
        using namespace std;

        std::sort(vars.begin(), vars.end(),
            [](core::ConsoleCommand* a, core::ConsoleCommand* b) {
                return strcmp(a->Name, b->Name) < 0;
            });
    }

    class CommandParser
    {
    public:
        explicit CommandParser(const char* cmd) :
            begin_(cmd),
            end_(cmd + core::strUtil::strlen(cmd))
        {
        }

        CommandParser(const char* pBegin, const char* pEnd) :
            begin_(pBegin),
            end_(pEnd)
        {
        }

        bool extractCommand(core::StringRange<char>& cmd)
        {
            const char* cur = begin_;
            const char* end = end_;

            while (cur < end) {
                char ch = *cur++;
                switch (ch) {
                    case '\'':
                    case '\"':
                        while ((*cur++ != ch) && cur < end)
                            ;
                        break;
                    case '\n':
                    case '\r':
                    case ';':
                    case '\0': {
                        // we class this as end of a command.
                        cmd = StringRange<char>(begin_, cur);
                        begin_ = cur;
                        return true;
                    } break;
                }
            }

            // got anything?
            if (begin_ < cur) {
                cmd = StringRange<char>(begin_, cur);
                begin_ = cur;
                return true;
            }
            return false;
        }

    private:
        const char* begin_;
        const char* end_;
    };

    struct AutoResult
    {
        AutoResult() :
            AutoResult(nullptr, nullptr, nullptr)
        {
        }
        AutoResult(const char* pName, ICVar* var, ConsoleCommand* pCmd) :
            pName(pName),
            var(var),
            pCmd(pCmd)
        {
        }

        X_INLINE bool operator<(const AutoResult& oth)
        {
            return strcmp(pName, oth.pName) < 0;
        }

    public:
        const char* pName;
        ICVar* var;
        ConsoleCommand* pCmd;
    };

    const int32_t CONSOLE_INPUT_FONT_SIZE = 18;
    const int32_t CONSOLE_DEFAULT_LOG_FONT_SIZE = 14;

    const float CONSOLE_INPUT_LINE_HIEGHT = 1.1f;
    const float CONSOLE_DEFAULT_LOG_LINE_HIEGHT = 1.1f;


    bool cvarModifyBegin(ICVar* pCVar, ExecSource::Enum source)
    {
        X_ASSERT_NOT_NULL(pCVar);

        ICVar::FlagType flags = pCVar->GetFlags();

        if (flags.IsSet(VarFlag::READONLY)) {
            X_ERROR("Console", "can't set value of read only cvar: %s", pCVar->GetName());
            return false;
        }

        if (source == ExecSource::CONFIG) {
            flags.Set(VarFlag::CONFIG);
            pCVar->SetFlags(flags);
        }

        return true;
    }
} // namespace

// ==================================================

ConsoleCommand::ConsoleCommand() // flags default con is (0)
{
}

// ==================================================

ConsoleCommandArgs::ConsoleCommandArgs(
    core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& line)
{
    TokenizeString(line.begin(), line.end());
}

ConsoleCommandArgs::~ConsoleCommandArgs()
{
}

size_t ConsoleCommandArgs::GetArgCount(void) const
{
    return argNum_;
}

const char* ConsoleCommandArgs::GetArg(size_t Idx) const
{
    X_ASSERT(Idx < argNum_, "Argument index out of range")(argNum_, Idx);
    return argv_[Idx];
}

// we want to get arguemtns.
// how will a command be formated.
// command val, val1, #var_name, string var3,
void ConsoleCommandArgs::TokenizeString(const char* begin, const char* end)
{
    argNum_ = 0;
    core::zero_object(tokenized_);

    size_t len, totalLen = 0;

    len = static_cast<size_t>(end - begin);
    if (len < 1) {
        return;
    }

    // need to be made use of.
    X_UNUSED(end);

    const char* start = begin;
    const char* commandLine = begin;
    while (char ch = *commandLine++) {
        if (argNum_ == MAX_COMMAND_ARGS) {
            return;
        }

        switch (ch) {
            case '\'':
            case '\"': {
                while ((*commandLine++ != ch) && *commandLine)
                    ; // find end

                argv_[argNum_] = tokenized_ + totalLen;
                argNum_++;

                size_t Len = (commandLine - start) - 2;

                ::memcpy(tokenized_ + totalLen, start + 1, Len);
                totalLen += Len + 1;

                start = commandLine;
                break;
            }
            case ' ':
                start = commandLine;
                break;
            default: {
                if ((*commandLine == ' ') || !*commandLine) {
                    argv_[argNum_] = tokenized_ + totalLen;
                    argNum_++;

                    if (*start == '#') {
                        ++start;

                        core::StackString<256> name(start, commandLine);

                        // it's a var name.
                        ICVar* var = gEnv->pConsole->getCVar(name.c_str());

                        if (var) {
                            core::ICVar::StrBuf strBuf;

                            name.clear();
                            name.append(var->GetString(strBuf));

                            ::memcpy(tokenized_ + totalLen, name.begin(), name.length());
                            totalLen += (name.length() + 1);
                            start = commandLine + 1;
                            continue;
                        }
                    }

                    size_t Len = (commandLine - start);

                    ::memcpy(tokenized_ + totalLen, start, Len);
                    totalLen += (Len + 1);

                    start = commandLine + 1;
                }
            } break;
        } // switch
    }     // while
}

//////////////////////////////////////////////////////////////////////////

const char* XConsole::CMD_HISTORY_FILE_NAME = "cmdHistory.txt";
const char* XConsole::USER_CFG_FILE_NAME = "user_config.cfg";

//////////////////////////////////////////////////////////////////////////

XConsole::ExecCommand::ExecCommand(const string& cmd, ExecSource::Enum src, bool sm) :
    command(cmd),
    source(src),
    silentMode(sm)
{
}

XConsole::Cursor::Cursor() :
    curTime(0.f),
    displayTime(0.5f),
    draw(false)
{
}

//////////////////////////////////////////////////////////////////////////

XConsole::XConsole() :
    varHeap_(
        bitUtil::RoundUpToMultiple<size_t>(
            VarPool::getMemoryRequirement(VAR_ALLOCATION_SIZE) * VAR_MAX,
            VirtualMem::GetPageSize())),
    varAllocator_(varHeap_.start(), varHeap_.end(),
        VarPool::getMemoryRequirement(VAR_ALLOCATION_SIZE),
        VarPool::getMemoryAlignmentRequirement(VAR_ALLOCATION_ALIGNMENT),
        VarPool::getMemoryOffsetRequirement()),
    varArena_(&varAllocator_, "VarArena"),
    cmdHistory_(g_coreArena),
    consoleLog_(g_coreArena),
    varMap_(g_coreArena),
    cmdMap_(g_coreArena),
    binds_(g_coreArena),
    configCmds_(g_coreArena),
    varArchive_(g_coreArena),
    cmds_(g_coreArena),
    coreEventListernRegd_(false),
    historyLoadPending_(false),
    historyFileSize_(0),
    historyFileBuf_(g_coreArena)
{
    historyPos_ = 0;
    cursorPos_ = 0;
    scrollPos_ = 0;

    g_coreArena->addChildArena(&varArena_);

    consoleState_ = consoleState::CLOSED;

    pCore_ = nullptr;
    pFont_ = nullptr;
    pRender_ = nullptr;
    pPrimContext_ = nullptr;

    // Auto goat a boat.
    autoCompleteNum_ = 0;
    autoCompleteIdx_ = -1;
    autoCompleteSelect_ = false;

    // reserve a pickle. (vars are registered before 'Startup' might change that)
    varMap_.reserve(4096);
    cmdMap_.reserve(1024);
    binds_.reserve(128);
    configCmds_.reserve(64);
    varArchive_.reserve(64);

    repeatEventTimer_ = TimeVal(0ll);
    repeatEventInterval_ = TimeVal(0.025f);
    repeatEventInitialDelay_ = TimeVal(0.5f);
}

XConsole::~XConsole()
{
}

void XConsole::registerVars(void)
{
    ADD_CVAR_REF_NO_NAME(console_debug, 0, 0, 1, VarFlag::SYSTEM | VarFlag::CHEAT,
        "Debugging for console operations. 0=off 1=on");
    ADD_CVAR_REF_NO_NAME(console_case_sensitive, 0, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Console input auto complete is case-sensitive");
    ADD_CVAR_REF_NO_NAME(console_save_history, 1, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Saves command history to file");
    ADD_CVAR_REF_NO_NAME(console_buffer_size, 1000, 1, 10000, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Size of the log buffer");
    ADD_CVAR_REF_NO_NAME(console_output_draw_channel, 1, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Draw the channel in a diffrent color. 0=disabled 1=enabled");
    ADD_CVAR_REF_NO_NAME(console_cursor_skip_color_codes, 1, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Skips over the color codes when moving cursor.");
    ADD_CVAR_REF_NO_NAME(console_disable_mouse, 2, 0, 2, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Disable mouse input when console open. 1=expanded only 2=always");

    ADD_CVAR_REF_NO_NAME(console_output_font_size, CONSOLE_DEFAULT_LOG_FONT_SIZE, 1, 256, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Font size of log messages");
    ADD_CVAR_REF_NO_NAME(console_output_font_line_height, CONSOLE_DEFAULT_LOG_LINE_HIEGHT, 0.1f, 10.f, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
        "Line height of log messages");

    ADD_CVAR_REF_COL_NO_NAME(console_cmd_color, Color(0.0f, 0.5f, 0.5f, 1.f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console cmd color");
    ADD_CVAR_REF_COL_NO_NAME(console_input_box_color, Color(0.3f, 0.3f, 0.3f, 0.75f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console input box color");
    ADD_CVAR_REF_COL_NO_NAME(console_input_box_color_border, Color(0.1f, 0.1f, 0.1f, 1.0f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console input box color");
    ADD_CVAR_REF_COL_NO_NAME(console_output_box_color, Color(0.2f, 0.2f, 0.2f, 0.9f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output box color");
    ADD_CVAR_REF_COL_NO_NAME(console_output_box_color_border, Color(0.1f, 0.1f, 0.1f, 1.0f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output box color");
    ADD_CVAR_REF_COL_NO_NAME(console_output_box_channel_color, Color(0.15f, 0.15f, 0.15f, 0.5f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output box channel color");
    ADD_CVAR_REF_COL_NO_NAME(console_output_scroll_bar_color, Color(0.5f, 0.5f, 0.5f, 1.0f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output scroll bar color");
    ADD_CVAR_REF_COL_NO_NAME(console_output_scroll_bar_slider_color, Color(0.0f, 0.0f, 0.0f, 0.9f),
        VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output scroll bar slider color");
}

void XConsole::registerCmds(void)
{
    ADD_COMMAND_MEMBER("exec", this, XConsole, &XConsole::Command_Exec, VarFlag::SYSTEM, "executes a file(.cfg)");
    ADD_COMMAND_MEMBER("history", this, XConsole, &XConsole::Command_History, VarFlag::SYSTEM, "displays command history");
    ADD_COMMAND_MEMBER("help", this, XConsole, &XConsole::Command_Help, VarFlag::SYSTEM, "displays help info");
    ADD_COMMAND_MEMBER("listCmds", this, XConsole, &XConsole::Command_ListCmd, VarFlag::SYSTEM, "lists avaliable commands");
    ADD_COMMAND_MEMBER("listDvars", this, XConsole, &XConsole::Command_ListDvars, VarFlag::SYSTEM, "lists dvars");
    ADD_COMMAND_MEMBER("listDvarValues", this, XConsole, &XConsole::Command_ListDvarsValues, VarFlag::SYSTEM, "Same as 'listDvars' but showing values");
    ADD_COMMAND_MEMBER("exit", this, XConsole, &XConsole::Command_Exit, VarFlag::SYSTEM, "closes the game");
    ADD_COMMAND_MEMBER("quit", this, XConsole, &XConsole::Command_Exit, VarFlag::SYSTEM, "closes the game");
    ADD_COMMAND_MEMBER("echo", this, XConsole, &XConsole::Command_Echo, VarFlag::SYSTEM, "prints text in argument, prefix dvar's with # to print value");
    ADD_COMMAND_MEMBER("vreset", this, XConsole, &XConsole::Command_VarReset, VarFlag::SYSTEM, "resets a variable to it's default value");
    ADD_COMMAND_MEMBER("vdesc", this, XConsole, &XConsole::Command_VarDescribe, VarFlag::SYSTEM, "describes a variable");
    ADD_COMMAND_MEMBER("seta", this, XConsole, &XConsole::Command_SetVarArchive, VarFlag::SYSTEM, "set a var and flagging it to be archived");

    ADD_COMMAND_MEMBER("bind", this, XConsole, &XConsole::Command_Bind, VarFlag::SYSTEM, "binds a key to a action Eg: bind shift a 'echo hello';");
    ADD_COMMAND_MEMBER("clearBinds", this, XConsole, &XConsole::Command_BindsClear, VarFlag::SYSTEM, "clears all binds");
    ADD_COMMAND_MEMBER("listBinds", this, XConsole, &XConsole::Command_BindsList, VarFlag::SYSTEM, "lists all the binds");

    ADD_COMMAND_MEMBER("saveModifiedVars", this, XConsole, &XConsole::Command_SaveModifiedVars, VarFlag::SYSTEM, "Saves modifed vars");

    ADD_COMMAND_MEMBER("consoleShow", this, XConsole, &XConsole::Command_ConsoleShow, VarFlag::SYSTEM, "opens the console");
    ADD_COMMAND_MEMBER("consoleHide", this, XConsole, &XConsole::Command_ConsoleHide, VarFlag::SYSTEM, "hides the console");
    ADD_COMMAND_MEMBER("consoleToggle", this, XConsole, &XConsole::Command_ConsoleToggle, VarFlag::SYSTEM, "toggle the console");
}

// ------------------------------------

bool XConsole::init(ICore* pCore, bool basic)
{
    X_LOG0("Console", "Starting console");
    pCore_ = pCore;

    // dispatch command history loading.
    if (!basic) {
        pCore_->GetILog()->AddLogger(&logger_);

        coreEventListernRegd_ = pCore_->GetCoreEventDispatcher()->RegisterListener(this);

        if (console_save_history) {
            loadCmdHistory(true);
        }
    }
    else {
        // when in basic mode, don't save out.
        console_save_history = 0;
    }

    return true;
}

bool XConsole::asyncInitFinalize(void)
{
    // the history file should of loaded now, so lets parse it.
    {
        core::CriticalSection::ScopedLock lock(historyFileLock_);

        while (historyLoadPending_) {
            historyFileCond_.Wait(historyFileLock_);
        }

        if (historyFileSize_) {
            const char* pBegin = historyFileBuf_.get();
            const char* pEnd = pBegin + historyFileSize_;

            parseCmdHistory(pBegin, pEnd);

            historyFileBuf_.reset();
            historyFileSize_ = 0;
        }
    }

    return true;
}

bool XConsole::loadRenderResources(void)
{
    X_ASSERT_NOT_NULL(pCore_);
    X_ASSERT_NOT_NULL(pCore_->GetIFontSys());
    X_ASSERT_NOT_NULL(pCore_->GetIRender());
    X_ASSERT_NOT_NULL(pCore_->Get3DEngine());

    pFont_ = pCore_->GetIFontSys()->getDefault();

    X_ASSERT_NOT_NULL(pFont_);

    pRender_ = pCore_->GetIRender();
    pPrimContext_ = pCore_->Get3DEngine()->getPrimContext(engine::PrimContext::CONSOLE);
    return true;
}

// --------------------

void XConsole::shutDown(void)
{
    X_LOG0("Console", "Shutting Down");

    // check if core failed to init.
    if (pCore_) {
        if (coreEventListernRegd_) {
            pCore_->GetCoreEventDispatcher()->RemoveListener(this);
        }
    //    pCore_->GetHotReloadMan()->addfileType(nullptr, CONFIG_FILE_EXTENSION);
        pCore_->GetILog()->RemoveLogger(&logger_);
    }

    // clear up vars.
    if (!varMap_.empty()) {

        // we use the value of the last item to move iterator
        for (auto it = varMap_.begin(); it != varMap_.end();) 
        {
            auto* pVar = it->second;
            ++it;

            X_DELETE(pVar, &varArena_);
        }

        varMap_.clear();
    }

    inputBuffer_.clear();
    refString_.clear();
    cmdHistory_.clear();
}

void XConsole::freeRenderResources(void)
{
    pRender_ = nullptr;
}

void XConsole::saveChangedVars(void)
{
    if (varMap_.empty()) {
        X_WARNING("Console", "Skipping saving of modified vars. no registerd vars.");
        return;
    }

    X_LOG0("Console", "Saving moified vars");

    core::XFileScoped file;

    core::Path<char> userConfigPath("config/");
    userConfigPath.append(USER_CFG_FILE_NAME);

    // we need to handle the case where a modified var is in the config
    // but we are shutting down before that var has been registerd again.
    // so it won't get saved out into the new one.
    // so i'm going to parse the exsisting config and keep any var sets that are for vars that don't currently exsist.

    core::Array<char> buf(g_coreArena);
    core::ArrayGrowMultiply<core::StringRange<char>> keep(g_coreArena);

    if (gEnv->pFileSys->fileExists(userConfigPath)) {
        if (file.openFile(userConfigPath, FileFlag::READ | FileFlag::SHARE)) {
            const auto size = safe_static_cast<size_t>(file.remainingBytes());

            if (size > 0) {
                buf.resize(size);
                if (file.read(buf.data(), size) != size) {
                    X_ERROR("Console", "Failed to read exsisiting config file data");
                }
                else {
                    core::StringTokenizer<char> tokenizer(buf.begin(), buf.end(), '\n');
                    core::StringRange<char> line(nullptr, nullptr);

                    // we save this file so it should only have 'seta' in but lets not error if something else.
                    while (tokenizer.extractToken(line)) {
                        core::StringTokenizer<char> lineTokenizer(line.getStart(), line.getEnd(), ' ');
                        core::StringRange<char> token(nullptr, nullptr);

                        if (lineTokenizer.extractToken(token) && core::strUtil::IsEqual(token.getStart(), token.getEnd(), "seta")) {
                            // get the name.
                            if (lineTokenizer.extractToken(token)) {
                                // work out if we have this var.
                                core::StackString256 name(token.getStart(), token.getEnd());

                                ICVar* pVar = getCVar(name.c_str());
                                if (!pVar) {
                                    keep.push_back(line);
                                }
                            }
                        }
                    }
                }
            }

            file.close();
        }
    }

    core::ByteStream stream(g_coreArena);
    stream.reserve(4096);
    stream.write("// auto generated\n", sizeof("// auto generated\n") - 1);

    for (auto& k : keep) {
        stream.write(k.getStart(), k.getLength());
        stream.write('\n');
    }

    core::ICVar::StrBuf strBuf;

    for (auto& it : varMap_) {
        ICVar* pVar = it.second;
        ICVar::FlagType flags = pVar->GetFlags();

        // we always save 'ARCHIVE' and only save 'SAVE_IF_CHANGED' if 'MODIFIED'
        bool save = (flags.IsSet(VarFlag::SAVE_IF_CHANGED) && flags.IsSet(VarFlag::MODIFIED)) || flags.IsSet(VarFlag::ARCHIVE);

        if (save) {
            // save out name + value.
            const char* pName = pVar->GetName();
            const char* pValue = pVar->GetString(strBuf);
            
            stream.write("seta ", 5);
            stream.write(pName, core::strUtil::strlen(pName));
            stream.write(' ');
            stream.write(pValue, core::strUtil::strlen(pValue));
            stream.write('\n');
        }
    }

    if (!file.openFile(userConfigPath, FileFlag::WRITE | FileFlag::RECREATE)) {
        X_ERROR("Console", "Failed to open file for saving modifed vars");
        return;
    }

    if (file.write(stream.data(), stream.size()) != stream.size()) {
        X_ERROR("Console", "Failed to write modifed vars data");
    }

    file.close();
}


void XConsole::dispatchRepeateInputEvents(core::FrameTimeData& time)
{
    // we must be open to accept input.
    // cancel any repeat events when we close.
    if (!isVisable()) {
        repeatEvent_.keyId = input::KeyId::UNKNOWN;
        return;
    }

    if (repeatEvent_.keyId != input::KeyId::UNKNOWN) {
        // we want to be able to de increment the time.
        repeatEventTimer_ -= time.unscaledDeltas[ITimer::Timer::UI];

        if (repeatEventTimer_.GetValue() < 0) {
            processInput(repeatEvent_);

            repeatEventTimer_ = repeatEventInterval_;
        }
    }
}


void XConsole::runCmds(void)
{
    while (cmds_.isNotEmpty()) {
        executeStringInternal(cmds_.peek());
        cmds_.pop();
    }
}

void XConsole::draw(core::FrameTimeData& time)
{
    cursor_.curTime += time.unscaledDeltas[ITimer::Timer::UI];

    if (cursor_.curTime > cursor_.displayTime) {
        cursor_.draw = !cursor_.draw;   // toggle it
        cursor_.curTime = TimeVal(0ll); // reset
    }

    drawBuffer();
}

bool XConsole::onInputEvent(const input::InputEvent& event)
{
    if (event.action == input::InputState::CHAR) {
        return handleInputChar(event);
    }

    return handleInput(event);
}


ICVar* XConsole::registerString(const char* pName, const char* Value,
    VarFlags Flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);

    ICVar* pCVar = getCVar(pName);
    if (pCVar) {
        return pCVar;
    }

    if (Flags.IsSet(VarFlag::CPY_NAME)) {
        pCVar = X_NEW(CVarString<CVarBaseHeap>, &varArena_,
            "CVarString<H>")(this, pName, Value, Flags, pDesc);
    }
    else {
        pCVar = X_NEW(CVarString<CVarBaseConst>, &varArena_,
            "CVarString")(this, pName, Value, Flags, pDesc);
    }

    registerVar(pCVar);
    return pCVar;
}

ICVar* XConsole::registerInt(const char* pName, int Value, int Min,
    int Max, VarFlags Flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);

    ICVar* pCVar = getCVarForRegistration(pName);
    if (pCVar) {
        return pCVar;
    }

    pCVar = X_NEW(CVarInt<CVarBaseConst>, &varArena_, "CVarInt")(this, pName, Value, Min, Max, Flags, pDesc);
    registerVar(pCVar);
    return pCVar;
}

ICVar* XConsole::registerFloat(const char* pName, float Value, float Min,
    float Max, VarFlags Flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);

    ICVar* pCVar = getCVarForRegistration(pName);
    if (pCVar) {
        return pCVar;
    }

    pCVar = X_NEW(CVarFloat<CVarBaseConst>, &varArena_, "CVarFloat")(this, pName, Value, Min, Max, Flags, pDesc);
    registerVar(pCVar);
    return pCVar;
}

ICVar* XConsole::registerRef(const char* pName, float* src, float defaultvalue,
    float Min, float Max, VarFlags flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);
    X_ASSERT_NOT_NULL(src);

    ICVar* pCVar = getCVarForRegistration(pName);
    if (pCVar) {
        return pCVar;
    }

    *src = defaultvalue;

    pCVar = X_NEW(CVarFloatRef, &varArena_, "CVarRefFloat")(this, pName, src, Min, Max, flags, pDesc);
    registerVar(pCVar);
    return pCVar;
}

ICVar* XConsole::registerRef(const char* pName, int* src, int defaultvalue,
    int Min, int Max, VarFlags flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);
    X_ASSERT_NOT_NULL(src);

    ICVar* pCVar = getCVarForRegistration(pName);
    if (pCVar) {
        return pCVar;
    }

    *src = defaultvalue;

    pCVar = X_NEW(CVarIntRef, &varArena_, "CVarRefInt")(this, pName, src, Min, Max, flags, pDesc);
    registerVar(pCVar);
    return pCVar;
}

ICVar* XConsole::registerRef(const char* pName, Color* src, Color defaultvalue,
    VarFlags flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);
    X_ASSERT_NOT_NULL(src);

    ICVar* pCVar = getCVarForRegistration(pName);
    if (pCVar) {
        return pCVar;
    }

    *src = defaultvalue;

    pCVar = X_NEW(CVarColRef, &varArena_, "CVarRefCol")(this, pName, src, flags, pDesc);
    registerVar(pCVar);
    return pCVar;
}

ICVar* XConsole::registerRef(const char* pName, Vec3f* src, Vec3f defaultvalue,
    VarFlags flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);
    X_ASSERT_NOT_NULL(src);

    ICVar* pCVar = getCVarForRegistration(pName);
    if (pCVar) {
        return pCVar;
    }

    *src = defaultvalue;

    pCVar = X_NEW(CVarVec3Ref, &varArena_, "CVarRefVec3")(this, pName, src, flags, pDesc);
    registerVar(pCVar);
    return pCVar;
}

ICVar* XConsole::getCVar(const char* pName)
{
    auto it = varMap_.find(pName);
    if (it != varMap_.end()) {
        return it->second;
    }
    return nullptr;
}

void XConsole::unregisterVariable(const char* pVarName)
{
    auto it = varMap_.find(pVarName);
    if (it == varMap_.end()) {
        X_WARNING("Console", "Failed to find var \"%s\" for removal", pVarName);
        return;
    }

    auto* pVar = it->second;

    varMap_.erase(pVarName);
    X_DELETE(pVar, &varArena_);
}

void XConsole::unregisterVariable(ICVar* pCVar)
{
    auto pName = pCVar->GetName();

    varMap_.erase(pName);
    X_DELETE(pCVar, &varArena_);
}

// Commands :)

void XConsole::registerCommand(const char* pName, ConsoleCmdFunc func, VarFlags Flags, const char* pDesc)
{
    X_ASSERT_NOT_NULL(pName);

    ConsoleCommand cmd;

    cmd.Name = pName;
    cmd.Flags = Flags;
    cmd.func = func;
    if (pDesc) {
        cmd.Desc = pDesc;
    }

    // pass cmd.Name instead of Name, saves creating a second core::string
    if (cmdMap_.find(cmd.Name) != cmdMap_.end()) {
        X_WARNING("Console", "command already exsists: \"%s", pName);
    }

    cmdMap_.insert(std::make_pair(cmd.Name, cmd));
}

void XConsole::unRegisterCommand(const char* pName)
{
    auto it = cmdMap_.find(X_CONST_STRING(pName));
    if (it != cmdMap_.end()) {
        cmdMap_.erase(it);
    }
}

void XConsole::exec(const char* pCommand)
{
    X_ASSERT_NOT_NULL(pCommand);

    addCmd(pCommand, ExecSource::SYSTEM, false);
}


bool XConsole::loadAndExecConfigFile(const char* pFileName)
{
    core::Path<char> path;

    path = "config";
    path /= pFileName;
    path.setExtension(CONFIG_FILE_EXTENSION);

    X_LOG0("Config", "Loading config: \"%s\"", pFileName);

    core::XFileScoped file;
    if (!file.openFile(path, FileFlag::READ)) {
        X_ERROR("Config", "failed to load: \"%s\"", path.c_str());
        return false;
    }

    auto bytes = safe_static_cast<size_t>(file.remainingBytes());
    if (bytes == 0) {
        return true;
    }

    core::Array<char, core::ArrayAlignedAllocator<char>> data(g_coreArena);
    data.getAllocator().setBaseAlignment(16);
    data.resize(bytes + 2);

    if (file.read(data.data(), bytes) != bytes) {
        X_ERROR("Config", "failed to read: \"%s\"", path.c_str());
        return false;
    }

    // 2 bytes at end so the multiline search can be more simple.
    // and not have to worrie about reading out of bounds.
    data[bytes] = '\0';
    data[bytes + 1] = '\0';

    // execute all the data in the file.
    // it's parsed in memory.
    // remove comments here.
    char* begin = data.begin();
    char* end = begin + bytes;
    const char* pComment;

    // we support // and /* */ so loook for a '/'
    while ((pComment = core::strUtil::Find(begin, end, '/')) != nullptr) {
        // wee need atleast 1 more char.
        if (pComment >= (end - 1)) {
            break;
        }

        begin = const_cast<char*>(++pComment);

        if (*begin == '*') {
            begin[-1] = ' ';
            *begin++ = ' ';

            while (*begin != '*' && begin[1] != '/' && begin < end) {
                *begin++ = ' ';
            }
        }
        else if (*begin == '/') {
            // signle line.
            begin[-1] = ' ';
            *begin++ = ' ';

            while (*begin != '\n' && begin < end) {
                *begin++ = ' ';
            }
        }
        else {
            ++begin;
        }
    }

    configExec(data.begin(), data.begin() + bytes);
    return true;
}


void XConsole::addLineToLog(const char* pStr, uint32_t length)
{
    X_UNUSED(length);

    decltype(logLock_)::ScopedLock lock(logLock_);

    consoleLog_.emplace(pStr, length);

    const int32_t bufferSize = console_buffer_size;

    if (safe_static_cast<int32_t, size_t>(consoleLog_.size()) > bufferSize) {
        consoleLog_.pop();

        // too handle the case we log after render has been shutdown or not init.
        // really the MaxVisibleLogLines should be updated to not use render interface
        // and just listen for core events to get size.
        if (pRender_) {
            const auto noneScroll = maxVisibleLogLines();

            // move scroll wheel with the moving items?
            if (scrollPos_ > 0 && scrollPos_ < (safe_static_cast<std::remove_const<decltype(noneScroll)>::type>(consoleLog_.size()) - noneScroll)) {
                scrollPos_++;
            }
        }
    }
    else {
        if (scrollPos_ > 0) {
            scrollPos_++;
        }
    }
}

int32_t XConsole::getLineCount(void) const
{
    return safe_static_cast<int32_t, size_t>(consoleLog_.size());
}

// -------------------------------------------------

ICVar* XConsole::getCVarForRegistration(const char* pName)
{
    auto it = varMap_.find(X_CONST_STRING(pName));
    if (it != varMap_.end()) {
        // if you get this warning you need to fix it.
        X_ERROR("Console", "var(%s) is already registerd", pName);
        return it->second;
    }

    return nullptr;
}

void XConsole::registerVar(ICVar* pCVar)
{
    auto it = configCmds_.find(X_CONST_STRING(pCVar->GetName()));
    if (it != configCmds_.end()) {
        if (cvarModifyBegin(pCVar, ExecSource::CONFIG)) {
            pCVar->Set(it->second);
        }

        X_LOG2("Console", "Var \"%s\" was set by config on registeration", pCVar->GetName());
    }

    it = varArchive_.find(X_CONST_STRING(pCVar->GetName()));
    if (it != varArchive_.end()) {
        if (cvarModifyBegin(pCVar, ExecSource::CONFIG)) { // is this always gonna be config?
            pCVar->Set(it->second);
        }

        // mark as archive.
        pCVar->SetFlags(pCVar->GetFlags() | VarFlag::ARCHIVE);

        X_LOG2("Console", "Var \"%s\" was set by seta on registeration", pCVar->GetName());
    }

    varMap_.insert(ConsoleVarMap::value_type(pCVar->GetName(), pCVar));
}



void XConsole::displayVarValue(const ICVar* pVar)
{
    if (!pVar) {
        return;
    }

    core::ICVar::StrBuf strBuf;
    X_LOG0("Dvar", "^2\"%s\"^7 = ^6%s", pVar->GetName(), pVar->GetString(strBuf));
}

void XConsole::displayVarInfo(const ICVar* pVar, bool fullInfo)
{
    if (!pVar) {
        return;
    }

    ICVar::FlagType::Description dsc;
    core::ICVar::StrBuf strBuf;

    if (fullInfo) {
        auto min = pVar->GetMin();
        auto max = pVar->GetMax();
        X_LOG0("Dvar", "^2\"%s\"^7 = '%s' min: %f max: %f [^1%s^7] Desc: \"%s\"", pVar->GetName(), pVar->GetString(strBuf), min, max,
            pVar->GetFlags().ToString(dsc), pVar->GetDesc());
    }
    else {
        X_LOG0("Dvar", "^2\"%s\"^7 = %s [^1%s^7]", pVar->GetName(), pVar->GetString(strBuf),
            pVar->GetFlags().ToString(dsc));
    }
}

// -------------------------

X_INLINE void XConsole::addCmd(const char* pCommand, ExecSource::Enum src, bool silent)
{
    cmds_.emplace(string(pCommand), src, silent);
}

void XConsole::addCmd(const string& command, ExecSource::Enum src, bool silent)
{
    cmds_.emplace(command, src, silent);
}


void XConsole::executeStringInternal(const ExecCommand& cmd)
{
    core::StackString512 name;
    core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS> value;
    core::StringRange<char> range(nullptr, nullptr);
    CommandParser parser(cmd.command.begin());
    const char* pPos;

    while (parser.extractCommand(range)) {
        // work out name / value
        pPos = range.find('=');

        if (pPos) {
            name.set(range.getStart(), pPos);
        }
        else if ((pPos = range.find(' ')) != nullptr) {
            name.set(range.getStart(), pPos);
        }
        else {
            name.set(range);
        }

        name.trim();
        name.stripColorCodes();

        if (name.isEmpty()) {
            continue;
        }

        // === Check if is a command ===
        auto itrCmd = cmdMap_.find(X_CONST_STRING(name.c_str()));
        if (itrCmd != cmdMap_.end()) {
            value.set(range);
            value.trim();
            executeCommand((itrCmd->second), value);
            continue;
        }

        // === check for var ===
        auto itrVar = varMap_.find(X_CONST_STRING(name.c_str()));
        if (itrVar != varMap_.end()) {
            ICVar* pCVar = itrVar->second;

            if (pPos) // is there a space || = symbol (meaning there is a possible value)
            {
                value.set(pPos + 1, range.getEnd());
                value.trim();

                if (value.isEmpty()) {
                    // no value was given so assume they wanted
                    // to print the value.
                    displayVarInfo(pCVar);
                }
                else if (cvarModifyBegin(pCVar, cmd.source)) {
                    pCVar->Set(value.c_str());
                    displayVarValue(pCVar);
                }
            }
            else {
                displayVarInfo(pCVar);
            }

            continue;
        }

        // if this was from config, add it to list.
        // so vars not yet registerd can get the value
        if (cmd.source == ExecSource::CONFIG && pPos) {
            value.set(pPos + 1, range.getEnd());
            value.trim();

            auto it = configCmds_.find(X_CONST_STRING(name.c_str()));
            if (it == configCmds_.end()) {
                configCmds_.insert(ConfigCmdsMap::iterator::value_type(name.c_str(), string(value.begin(), value.end())));
            }
            else {
                it->second = string(value.begin(), value.end());
            }
        }

        if (!cmd.silentMode) {
            if (cmd.source == ExecSource::CONFIG) {
                X_WARNING("Config", "Unknown command/var: %s", name.c_str());
            }
            else {
                X_WARNING("Console", "Unknown command: %s", name.c_str());
            }
        }
    }
}


void XConsole::executeCommand(const ConsoleCommand& cmd,
    core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& str) const
{
    str.replace('"', '\'');

    // dunno what I added this for but it's annoying.
    // X_LOG_BULLET;

    if (cmd.Flags.IsSet(VarFlag::CHEAT)) {
        X_WARNING("Console", "Cmd(%s) is cheat protected", cmd.Name.c_str());
        return;
    }

    if (cmd.func) {
        // This is function command, execute it with a list of parameters.
        ConsoleCommandArgs cmdArgs(str);

        if (console_debug) {
            X_LOG0("Console", "Running command \"%s\"", cmdArgs.GetArg(0));
        }

        cmd.func.Invoke(&cmdArgs);
        return;
    }
}

// ------------------------------------------


void XConsole::configExec(const char* pCommand, const char* pEnd)
{
    // if it's from config, should i limit what commands can be used?
    // for now i'll let any be used

    if (gEnv->isRunning()) {
        addCmd(pCommand, ExecSource::CONFIG, false);
    }
    else {
        // we run the command now.
        ExecCommand cmd;
        cmd.command = core::string(pCommand, pEnd);
        cmd.source = ExecSource::CONFIG;
        cmd.silentMode = false;

        executeStringInternal(cmd);
    }
}

// ------------------------------------------


void XConsole::addInputChar(const char c)
{
    const char tidle = '¬';

    if (c == '`' || c == tidle) { // sent twice.
        return;
    }

    if (cursorPos_ < safe_static_cast<int32_t, size_t>(inputBuffer_.length())) {
        inputBuffer_.insert(cursorPos_, c);
    }
    else {
        inputBuffer_ = inputBuffer_ + c;
    }

    cursorPos_++;

    //	X_LOG0("Console Buf", "%s (%i)", inputBuffer_.c_str(), cursorPos_);
}

void XConsole::removeInputChar(bool bBackSpace)
{
    if (inputBuffer_.isEmpty()) {
        return;
    }

    if (bBackSpace) {
        if (cursorPos_ > 0) {
            inputBuffer_.erase(cursorPos_ - 1, 1);
            cursorPos_--;
        }
    }
    else {
        // ho ho h.
        X_ASSERT_NOT_IMPLEMENTED();
    }

    //	X_LOG0("Console Buf", "%s (%i)", inputBuffer_.c_str(), cursorPos_);
}

void XConsole::clearInputBuffer(void)
{
    inputBuffer_ = "";
    cursorPos_ = 0;
}

void XConsole::executeInputBuffer(void)
{
    if (inputBuffer_.isEmpty()) {
        return;
    }

    core::string Temp = inputBuffer_;
    clearInputBuffer();

    addCmdToHistory(Temp);

    addCmd(Temp, ExecSource::CONSOLE, false);
}


bool XConsole::handleInput(const input::InputEvent& event)
{
    // open / close
    const auto keyReleased = event.action == input::InputState::RELEASED;

    if (keyReleased)
    {
        if (event.keyId == input::KeyId::OEM_8)
        {
            bool expand = event.modifiers.IsSet(input::ModifiersMasks::Shift);
            bool visable = isVisable();

            // clear states.
            gEnv->pInput->clearKeyState();

            if (expand) { // shift + ` dose not close anymore just expands.
                showConsole(consoleState::EXPANDED);
            }
            else {
                toggleConsole(false); // toggle it.
            }

            // don't clear if already visable, as we are just expanding.
            if (!visable) {
                clearInputBuffer();
            }

            return true;
        }
        else if (event.keyId == input::KeyId::ESCAPE && isVisable())
        {
            clearInputBuffer();
            showConsole(consoleState::CLOSED);
            return true;
        }
    }

#if 0
    // process key binds when console is hidden
    if (consoleState_ == consoleState::CLOSED)
    {
        const char* pCmdStr = 0;

        if (!event.modifiers.IsAnySet()) {
            pCmdStr = FindBind(event.name);
        }
        else {
            // build the key.
            core::StackString<60> bind_name;

            if (event.modifiers.IsSet(input::ModifiersMasks::Ctrl)) {
                bind_name.append("ctrl ");
            }
            if (event.modifiers.IsSet(input::ModifiersMasks::Shift)) {
                bind_name.append("shift ");
            }
            if (event.modifiers.IsSet(input::ModifiersMasks::Alt)) {
                bind_name.append("alt ");
            }
            if (event.modifiers.IsSet(input::ModifiersMasks::Win)) {
                bind_name.append("win ");
            }

            bind_name.append(event.name);

            pCmdStr = FindBind(bind_name.c_str());
        }

        if (pCmdStr) {
            AddCmd(pCmdStr, ExecSource::CONSOLE, false);
            return true;
        }
    }
#endif

    if (!isVisable()) {
        return false;
    }

    // -- OPEN --
    if (keyReleased) {
        repeatEvent_.keyId = input::KeyId::UNKNOWN;
    }

    if (event.action != input::InputState::PRESSED) 
    {
        if (event.deviceType == input::InputDeviceType::KEYBOARD) {
            return isVisable();
        }

        // eat mouse move?
        // Stops the camera moving around when we have console open.
        if (event.deviceType == input::InputDeviceType::MOUSE) {
            if (event.keyId != input::KeyId::MOUSE_Z) {
                if (console_disable_mouse == 1) // only if expanded
                {
                    return isExpanded();
                }
                if (console_disable_mouse == 2) {
                    return isVisable();
                }

                return false;
            }
        }
        else {
            return false;
        }
    }

    if (event.action == input::InputState::PRESSED) {
        repeatEvent_ = event;
        repeatEventTimer_ = repeatEventInitialDelay_;
    }

    if (isExpanded()) // you can only scroll a expanded console.
    {
        if (event.keyId == input::KeyId::MOUSE_Z) {
            int32_t scaled = static_cast<int32_t>(event.value);
            bool positive = (scaled >= 0);

            scaled /= 20;

            // enuse scaled didnt remove all scrolling
            if (positive && scaled < 1) {
                scaled = 1;
            }
            else if (!positive && scaled > -1) {
                scaled = -1;
            }

            scrollPos_ += scaled;

            validateScrollPos();
            return true;
        }
        else if (event.keyId == input::KeyId::PAGE_UP) {
            pageUp();
        }
        else if (event.keyId == input::KeyId::PAGE_DOWN) {
            pageDown();
        }
    }

  
    if (event.keyId != input::KeyId::TAB) {
        //	ResetAutoCompletion();
    }

    if (event.keyId == input::KeyId::V && event.modifiers.IsSet(input::ModifiersMasks::Ctrl)) {
        paste();
        return false;
    }

    if (event.keyId == input::KeyId::C && event.modifiers.IsSet(input::ModifiersMasks::Ctrl)) {
        copy();
        return false;
    }

    return processInput(event);
}

bool XConsole::handleInputChar(const input::InputEvent& event)
{
    if (!isVisable()) {
        return false;
    }

    repeatEvent_ = event;

    processInput(event);
    return true;
}


bool XConsole::processInput(const input::InputEvent& event)
{
    using namespace input;

    X_ASSERT(isVisable(), "ProcessInput called when not visible")(isVisable());

    // consume char input.
    if (event.action == InputState::CHAR) {
        addInputChar(event.inputchar);
        return true;
    }

    if (event.keyId == KeyId::ENTER || event.keyId == KeyId::NUMPAD_ENTER) {
        if (autoCompleteIdx_ >= 0) {
            autoCompleteSelect_ = true;
        }
        else {
            executeInputBuffer();
        }
        return true;
    }
    else if (event.keyId == KeyId::BACKSPACE || event.keyId == KeyId::DELETE) {
        // shift + DEL / BACK fully clears
        if (event.modifiers.IsSet(ModifiersMasks::Shift)) {
            clearInputBuffer();
        }
        else {
            removeInputChar(true);
        }
        return true;
    }
    else if (event.keyId == KeyId::LEFT_ARROW) {
        if (cursorPos_) { // can we go left?
            cursorPos_--;

            // support moving whole words
            if (event.modifiers.IsSet(ModifiersMasks::Ctrl)) {
                while (cursorPos_ && inputBuffer_[cursorPos_] != ' ') {
                    cursorPos_--;
                }
            }

            // disable blinking while moving.
            cursor_.curTime = TimeVal(0ll);
            cursor_.draw = true;

            if (console_cursor_skip_color_codes) {
                // if we are at a number and ^ is before us go back two more.
                if (cursorPos_ >= 1) {
                    const char curChar = inputBuffer_[cursorPos_];

                    if (core::strUtil::IsDigit(curChar)) {
                        const char PreChar = inputBuffer_[cursorPos_ - 1];

                        if (PreChar == '^') {
                            cursorPos_--;
                            if (cursorPos_ > 0) {
                                cursorPos_--;
                            }
                        }
                    }
                }
            }
        }
        return true;
    }
    else if (event.keyId == KeyId::RIGHT_ARROW) {
        // are we pre end ?
        if (cursorPos_ < safe_static_cast<int32_t>(inputBuffer_.length())) {
            cursorPos_++;

            // support moving whole words
            if (event.modifiers.IsSet(ModifiersMasks::Ctrl)) {
                while (cursorPos_ < safe_static_cast<int32_t>(inputBuffer_.length())
                       && inputBuffer_[cursorPos_] != ' ') {
                    cursorPos_++;
                }
            }

            // disable blinking while moving.
            cursor_.curTime = TimeVal(0ll);
            cursor_.draw = true;

            if (console_cursor_skip_color_codes) {
                uint32_t charsLeft = (safe_static_cast<int32_t>(inputBuffer_.length()) - cursorPos_);
                if (charsLeft >= 2) {
                    const char curChar = inputBuffer_[cursorPos_];
                    const char nextChar = inputBuffer_[cursorPos_ + 1];
                    if (curChar == '^' && core::strUtil::IsDigit(nextChar)) {
                        cursorPos_ += 2;
                    }
                }
            }
        }
        else if (autoCompleteIdx_ >= 0) {
            autoCompleteSelect_ = true;
        }
        return true;
    }
    else if (event.keyId == KeyId::HOME) {
        cursorPos_ = 0;
    }
    else if (event.keyId == KeyId::END) {
        cursorPos_ = safe_static_cast<int32_t>(inputBuffer_.length());
    }
    else if (event.keyId == KeyId::UP_ARROW) {
        if (isAutocompleteVis() && autoCompleteIdx_ >= 0) {
            autoCompleteIdx_ = core::Max(-1, --autoCompleteIdx_);
        }
        else {
            const char* pHistoryLine = getHistory(CmdHistory::UP);

            if (pHistoryLine) {
                if (console_debug) {
                    X_LOG0("Cmd history", "%s", pHistoryLine);
                }

                inputBuffer_ = pHistoryLine;
                cursorPos_ = safe_static_cast<int32_t>(inputBuffer_.size());
            }
        }
        return true;
    }
    else if (event.keyId == KeyId::DOWN_ARROW) {
        bool inHistory = (historyPos_ < static_cast<int32_t>(cmdHistory_.size()));
        bool multiAutoComplete = autoCompleteNum_ > 1;

        if (isAutocompleteVis() && (!inHistory || multiAutoComplete)) {
            autoCompleteIdx_ = core::Min(autoCompleteNum_ - 1, ++autoCompleteIdx_);

            // reset history if we move into autocomplete?
            // i think so..
            resetHistoryPos();
        }
        else {
            const char* pHistoryLine = getHistory(CmdHistory::DOWN);

            if (pHistoryLine) {
                if (console_debug) {
                    X_LOG0("Cmd history", "%s", pHistoryLine);
                }

                inputBuffer_ = pHistoryLine;
                cursorPos_ = safe_static_cast<int32_t>(inputBuffer_.size());
            }
        }
        return true;
    }

    return true;
}


// ----------------------------


void XConsole::saveCmdHistory(void) const
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pFileSys);

    core::ByteStream stream(g_coreArena);
    stream.reserve(0x100);

    for (auto& str : cmdHistory_) {
        stream.write(str.c_str(), str.length());
        stream.write("\n", 1);
    }

    FileFlags mode;
    mode.Set(FileFlag::WRITE);
    mode.Set(FileFlag::RECREATE);

    XFileScoped file;
    if (file.openFile(core::Path<>(CMD_HISTORY_FILE_NAME), mode)) {
        file.write(stream.data(), safe_static_cast<uint32_t>(stream.size()));
    }
}

void XConsole::loadCmdHistory(bool async)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pFileSys);

    core::CriticalSection::ScopedLock lock(historyFileLock_);

    if (historyLoadPending_) {
        return;
    }

    FileFlags mode;
    mode.Set(FileFlag::READ);
    mode.Set(FileFlag::SHARE);

    if (async) {
        // load the file async
        historyLoadPending_ = true;

        core::IoRequestOpenRead open;
        open.callback.Bind<XConsole, &XConsole::historyIoRequestCallback>(this);
        open.mode = mode;
        open.path = CMD_HISTORY_FILE_NAME;
        open.arena = g_coreArena;

        gEnv->pFileSys->AddIoRequestToQue(open);
    }
    else {
        XFileMemScoped file;
        if (file.openFile(core::Path<>(CMD_HISTORY_FILE_NAME), mode)) {
            const char* pBegin = file->getBufferStart();
            const char* pEnd = file->getBufferEnd();

            parseCmdHistory(pBegin, pEnd);
        }
    }
}


void XConsole::historyIoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    X_UNUSED(fileSys);
    X_UNUSED(bytesTransferred);

    // history file loaded.
    X_ASSERT(pRequest->getType() == core::IoRequest::OPEN_READ_ALL, "Recived unexpected request type")(pRequest->getType());
    const core::IoRequestOpenRead* pOpenRead = static_cast<const IoRequestOpenRead*>(pRequest);

    {
        core::CriticalSection::ScopedLock lock(historyFileLock_);

        historyLoadPending_ = false;

        // if it faild do we care?
        if (!pFile) {
            X_LOG2("Console", "Failed to load history file");
            return;
        }

        // we can't process it on this thread.
        // so store it.
        historyFileBuf_ = core::UniquePointer<const char[]>(pOpenRead->arena, reinterpret_cast<const char*>(pOpenRead->pBuf));
        historyFileSize_ = pOpenRead->dataSize;
    }

    historyFileCond_.NotifyAll();
}

void XConsole::parseCmdHistory(const char* pBegin, const char* pEnd)
{
    core::StringTokenizer<char> tokenizer(pBegin, pEnd, '\n');
    StringRange<char> range(nullptr, nullptr);

    // lets not clear, we can append and just pop off end if too many in total.
    // CmdHistory_.clear();

    while (tokenizer.extractToken(range)) {
        if (range.getLength() > 0) {
            cmdHistory_.emplace(range.getStart(), range.getEnd());

            if (cmdHistory_.size() > MAX_HISTORY_ENTRIES) {
                cmdHistory_.pop();
            }
        }
    }

    resetHistoryPos();
}


X_INLINE void XConsole::addCmdToHistory(const char* pCommand)
{
    addCmdToHistory(string(pCommand));
}

void XConsole::addCmdToHistory(const string& command)
{
    // make sure it's not same as last command
    if (cmdHistory_.isEmpty() || cmdHistory_.front() != command) {
        cmdHistory_.emplace(command);
    }

    // limit hte history.
    while (cmdHistory_.size() > MAX_HISTORY_ENTRIES) {
        cmdHistory_.pop();
    }

    resetHistoryPos();

    if (console_save_history) {
        saveCmdHistory();
    }
}

void XConsole::resetHistoryPos(void)
{
    historyPos_ = safe_static_cast<int32_t>(cmdHistory_.size());
}


const char* XConsole::getHistory(CmdHistory::Enum direction)
{
    if (cmdHistory_.isEmpty()) {
        return nullptr;
    }

    if (direction == CmdHistory::UP) {

        if (historyPos_ <= 0) {
            return nullptr;
        }

        historyPos_--;

        refString_ = cmdHistory_[historyPos_];
        return refString_.c_str();
    }
    else // down
    {
        // are we above base cmd?
        if (historyPos_ < safe_static_cast<int32_t>(cmdHistory_.size()) - 1) {
            historyPos_++;

            // adds a refrence to the string.
            refString_ = cmdHistory_[historyPos_];
            return refString_.c_str();
        }
    }

    return nullptr;
}

// --------------------------------

// Binds a cmd to a key
void XConsole::addBind(const char* pKey, const char* pCmd)
{
    // check for override ?
    const char* Old = findBind(pKey);

    if (Old) {
        if (core::strUtil::IsEqual(Old, pCmd)) {
            // bind is same.
            return;
        }
        if (console_debug) {
            X_LOG1("Console", "Overriding bind \"%s\" -> %s with -> %s", pKey, Old, pCmd);
        }

        auto it = binds_.find(X_CONST_STRING(pKey));
        it->second = pCmd;
    }

    binds_.insert(ConsoleBindMap::value_type(core::string(pKey), core::string(pCmd)));
}

// returns the command for a given key
// returns null if no bind found
const char* XConsole::findBind(const char* key)
{
    auto it = binds_.find(X_CONST_STRING(key));
    if (it != binds_.end()) {
        return it->second.c_str();
    }
    return nullptr;
}

// removes all the binds.
void XConsole::clearAllBinds(void)
{
    binds_.clear();
}

void XConsole::listbinds(IKeyBindDumpSink* CallBack)
{
    for (auto bind : binds_) {
        CallBack->OnKeyBindFound(bind.first.c_str(), bind.second.c_str());
    }
}


// --------------------------------------

void XConsole::pageUp(void)
{
    const int32_t visibleNum = maxVisibleLogLines();
    scrollPos_ += visibleNum;

    validateScrollPos();
}

void XConsole::pageDown(void)
{
    const int32_t visibleNum = maxVisibleLogLines();
    scrollPos_ -= visibleNum;

    validateScrollPos();
}

void XConsole::validateScrollPos(void)
{
    if (scrollPos_ < 0) {
        scrollPos_ = 0;
    }
    else {
        int32_t logSize = safe_static_cast<int32_t>(consoleLog_.size());
        const int32_t visibleNum = maxVisibleLogLines();

        logSize -= visibleNum;
        logSize += 2;

        if (scrollPos_ > logSize) {
            scrollPos_ = logSize;
        }
    }
}

// --------------------------------------


void XConsole::resetAutoCompletion(void)
{
    autoCompleteIdx_ = -1;
    autoCompleteNum_ = 0;
    autoCompleteSelect_ = false;
}

int32_t XConsole::maxVisibleLogLines(void) const
{
    const int32_t height = renderRes_.y - 40;

    const float lineSize = (console_output_font_size * console_output_font_line_height);
    return static_cast<int32_t>(height / lineSize);
}

/// ------------------------------------------------------


void XConsole::drawBuffer(void)
{
    if (consoleState_ == consoleState::CLOSED) {
        return;
    }

    if (!pPrimContext_) {
        return;
    }

    const Vec2f outputFontSize(static_cast<float>(console_output_font_size), static_cast<float>(console_output_font_size));
    const Vec2f inputFontSize(static_cast<float>(CONSOLE_INPUT_FONT_SIZE), static_cast<float>(CONSOLE_INPUT_FONT_SIZE));

    const Vec2i& res = renderRes_;

    const float xStart = 5.f;
    const float yStart = 35.f;
    const float width = res.x - 10.f;
    const float height = res.y - 40.f;

    font::TextDrawContext ctx;
    ctx.pFont = pFont_;
    ctx.effectId = 0;
    ctx.SetColor(Col_Khaki);
    ctx.SetSize(inputFontSize);

    {
        pPrimContext_->drawQuad(xStart, 5, width, 24, console_input_box_color, console_input_box_color_border);

        if (isExpanded()) {
            pPrimContext_->drawQuad(xStart, yStart, width, height, console_output_box_color, console_output_box_color_border);

            if (console_output_draw_channel) {
                ctx.SetSize(outputFontSize);

                const float32_t channelWidth = pFont_->GetCharWidth(L' ', 15, ctx);

                pPrimContext_->drawQuad(xStart, yStart, channelWidth, height, console_output_box_channel_color);

                ctx.SetSize(inputFontSize);
            }
        }
    }

    {
        const wchar_t* pTxt = X_WIDEN(X_ENGINE_NAME " Engine " X_BUILD_STRING ">");
        const wchar_t* pTxtEnd = pTxt + (sizeof(X_ENGINE_NAME " Engine " X_BUILD_STRING ">") - 1);

        Vec2f pos(10, 5);
        Vec2f txtwidth = pFont_->GetTextSize(pTxt, pTxtEnd, ctx);

        ctx.SetEffectId(pFont_->GetEffectId("drop"));

        pPrimContext_->drawText(Vec3f(pos.x, pos.y, 1), ctx, pTxt, pTxtEnd);

        ctx.SetDefaultEffect();
        ctx.SetColor(Col_White);

        pos.x += txtwidth.x;
        pos.x += 2;

        if (cursor_.draw) {
            float Lwidth = pFont_->GetTextSize(inputBuffer_.c_str(), inputBuffer_.c_str() + cursorPos_, ctx).x + 3; // 3px offset from engine txt.

            pPrimContext_->drawText(pos.x + Lwidth, pos.y, ctx, "_");
        }

        // the log.
        if (isExpanded()) {
            ctx.SetSize(outputFontSize);
            ctx.clip.set(0, 30, width, std::numeric_limits<float>::max());
            ctx.flags.Set(font::DrawTextFlag::CLIP);

            float fCharHeight = ctx.GetCharHeight() * console_output_font_line_height;

            float xPos = 8;
            float yPos = (height + yStart) - (fCharHeight + 10); // 15 uints up
            int32_t scroll = 0;

            decltype(logLock_)::ScopedLock lock(logLock_);

            const int32_t num = safe_static_cast<int32_t>(consoleLog_.size());

            for (int32_t i = (num - 1); i >= 0 && yPos >= 30; --i, ++scroll) {
                if (scroll >= scrollPos_) {
                    const char* pBuf = consoleLog_[i].c_str();

                    pPrimContext_->drawText(xPos, yPos, ctx, pBuf);
                    yPos -= fCharHeight;
                }
            }

            drawScrollBar();
        }

        // draw the auto complete
        drawInputTxt(pos);
    }
}

void XConsole::drawScrollBar(void)
{
    if (!isExpanded()) {
        return;
    }

    if (pFont_ && pPrimContext_ && pRender_) {
        // oooo shit nuggger wuggger.
        const Vec2i& res = renderRes_;

        const float width = res.x - 10.f;
        const float height = res.y - 40.f;

        const float barWidth = 6.f;
        const float marging = 5.f;
        const float sliderHeight = 20.f;

        float start_x = (width + 5) - (barWidth + marging);
        float start_y = 35.f + marging;
        float barHeight = height - (marging * 2);

        float slider_x = start_x;
        float slider_y = start_y;
        float slider_width = barWidth;
        float slider_height = sliderHeight;

        // work out the possition of slider.
        // we take the height of the bar - slider and divide.
        const int32_t visibleNum = maxVisibleLogLines();
        const int32_t scrollableLines = safe_static_cast<int32_t>(consoleLog_.size()) - (visibleNum + 2);

        float positionPercent = PercentageOf(scrollPos_, scrollableLines) * 0.01f;
        float offset = (barHeight - slider_height) * positionPercent;

        slider_y += (barHeight - slider_height - offset);
        if (slider_y < start_y) {
            slider_y = start_y;
        }

        pPrimContext_->drawQuad(start_x, start_y, barWidth, barHeight, console_output_scroll_bar_color);
        pPrimContext_->drawQuad(slider_x, slider_y, slider_width, slider_height, console_output_scroll_bar_slider_color);
    }
}

void XConsole::drawInputTxt(const Vec2f& start)
{
    const size_t max_auto_complete_results = 32;
    typedef core::FixedArray<AutoResult, max_auto_complete_results> Results;
    Results results;

    Color txtCol = Col_White;
    Vec2f txtPos = start;

    if (pFont_ && pPrimContext_) {
        const char* inputBegin = inputBuffer_.begin();
        const char* inputEnd = inputBuffer_.end();
        const char* pName;
        size_t NameLen, inputLen;

        if (inputBuffer_.isEmpty()) {
            resetAutoCompletion();
            return;
        }

        // check for vreset
        if (const char* vreset = core::strUtil::Find(inputBegin, inputEnd, "vreset ")) {
            // check if we have atleast 1 char.
            if (inputBuffer_.length() > 7) {
                inputBegin = core::Min(vreset + 7, inputEnd); // cap search.
                txtCol = console_cmd_color;
            }
        }

        // check for spaces.
        if (const char* space = core::strUtil::Find(inputBegin, inputEnd, ' ')) {
            //	if (space == (inputEnd -1))
            inputEnd = space; // cap search. (-1 will be safe since must be 1 char in string)
        }

        inputLen = inputEnd - inputBegin;
        if (inputLen == 0) {
            return;
        }

        typedef core::traits::Function<bool(const char*, const char*,
            const char*, const char*)>
            MyComparisionFunc;

        MyComparisionFunc::Pointer pComparison = core::strUtil::IsEqual;
        if (!console_case_sensitive) {
            pComparison = core::strUtil::IsEqualCaseInsen;
        }

        // try find and cmd's / dvars that match the current input.
        auto it = varMap_.begin();

        for (; it != varMap_.end(); ++it) {
            pName = it->second->GetName();
            NameLen = core::strUtil::strlen(pName);

            // if var name shorter than search leave it !
            if (NameLen < inputLen) {
                continue;
            }

            // we search same length.
            if (pComparison(pName, pName + inputLen, inputBegin, inputEnd)) {
                results.emplace_back(pName, it->second, nullptr);
            }

            if (results.size() == results.capacity()) {
                break;
            }
        }

        if (results.size() < results.capacity()) {
            // do the commands baby!
            auto cmdIt = cmdMap_.begin();
            for (; cmdIt != cmdMap_.end(); ++cmdIt) {
                pName = cmdIt->second.Name.c_str();
                NameLen = cmdIt->second.Name.length();

                // if cmd name shorter than search leave it !
                if (NameLen < inputLen) {
                    continue;
                }

                // we search same length.
                if (pComparison(pName, pName + inputLen, inputBegin, inputEnd)) {
                    results.emplace_back(pName, nullptr, &cmdIt->second);
                }

                if (results.size() == results.capacity()) {
                    break;
                }
            }
        }

        // sort them?
        std::sort(results.begin(), results.end());

        // Font contex
        font::TextDrawContext ctx;
        ctx.pFont = pFont_;
        ctx.effectId = 0;
        ctx.SetSize(Vec2f(14, 14));

        // Autocomplete
        if (autoCompleteNum_ != safe_static_cast<int, size_t>(results.size())) {
            autoCompleteIdx_ = -1;
        }

        autoCompleteNum_ = safe_static_cast<int, size_t>(results.size());
        autoCompleteIdx_ = core::Min(autoCompleteIdx_, autoCompleteNum_ - 1);
        autoCompleteSelect_ = autoCompleteIdx_ >= 0 ? autoCompleteSelect_ : false;

        if (autoCompleteSelect_) {
            if (inputBuffer_.find("vreset ")) {
                inputBuffer_ = "vreset ";
                inputBuffer_ += results[autoCompleteIdx_].pName;
            }
            else {
                inputBuffer_ = results[autoCompleteIdx_].pName;
            }
            //	if (results[autoCompleteIdx_].var) // for var only?
            inputBuffer_ += ' '; // add a space
            cursorPos_ = safe_static_cast<int32, size_t>(inputBuffer_.length());
            autoCompleteIdx_ = -1;
            autoCompleteSelect_ = false;
        }
        // ~AutoComplete

        if (!results.isEmpty()) {
            // calculate a size.
            float fCharHeight = 1.1f * ctx.GetCharHeight();
            float xpos = start.x;
            float ypos = start.y + 30;
            float width = 200; // min width
            float height = 5;
        resultsChanged:
            bool isSingleVar = results.size() == 1 && results[0].var;
            bool isSingleCmd = results.size() == 1 && !results[0].var;

            Color col = console_input_box_color;
            col.a = 1.f;

            if (isSingleCmd) {
                // if what they have entered fully matches a cmd,
                // change the txt color to darkblue.
                // we need to check if it's only a substring match currently.
                core::StackString<128> temp(inputBegin, inputEnd);
                const char* fullName = results[0].pName;

                temp.trim();

                if (temp.isEqual(fullName)) {
                    txtCol = console_cmd_color;
                }
            }
            else if (results.size() > 1) {
                // if there is a space after the cmd's name.
                // we check if the input has a complete match
                // to any of the results.
                // if so only show that.
                string::const_str pos = inputBuffer_.find(' ');
                if (pos) // != string::npos) //== (inputBuffer_.length() - 1))
                {
                    Results::const_iterator resIt;
                    core::StackString<128> temp(inputBuffer_.begin(), pos);

                    resIt = results.begin();
                    for (; resIt != results.end(); ++resIt) {
                        if (core::strUtil::IsEqual(temp.c_str(), resIt->pName)) {
                            // ok remove / add.
                            AutoResult res = *resIt;

                            results.clear();
                            results.append(res);
                            goto resultsChanged;
                        }
                    }
                }
            }

            if (isSingleVar) {
                core::StackString<128> nameStr;
                core::StackString<32> defaultStr;
                core::StackString<128> value; // split the value and name for easy alignment.
                core::StackString<128> defaultValue;
                core::StackString<512> description;
                core::StackString<256> domain;

                ICVar* pCvar = results[0].var;
                ICVar::FlagType flags = pCvar->GetFlags();

                const float nameValueSpacing = 15.f;
                const float colorBoxWidth = 40.f;
                const float colorBoxPadding = 3.f;
                float xposValueOffset = 0.f;
                bool isColorVar = pCvar->GetType() == VarFlag::COLOR;
                // display more info for a single var.
                // varname: value
                // default: default_value
                // ------------------
                // Description.
                // Possible Values;

                nameStr.appendFmt("%s", pCvar->GetName());
                defaultStr.append("	default");

                {
                    ICVar::StrBuf strBuf;

                    value.appendFmt("%s", pCvar->GetString(strBuf));
                    defaultValue.appendFmt("%s", pCvar->GetDefaultStr(strBuf));
                }
                description.append(pCvar->GetDesc()); // dose string length for us / caps.

                if (pCvar->GetType() == VarFlag::INT) {
                    if (flags.IsSet(VarFlag::BITFIELD)) {
                        // need to work out all bits that are set in max.

                        auto intToalphaBits = [](int32_t val, core::StackString<48>& strOut) {
                            strOut.clear();
                            for (uint32_t b = 1; b < sizeof(val) * 8; b++) {
                                if (core::bitUtil::IsBitSet(val, b)) {
                                    strOut.append(core::bitUtil::BitToAlphaChar(b), 1);
                                }
                            }
                        };

                        core::StackString<48> allBitsStr;
                        core::StackString<48> valueBitsStr;
                        core::StackString<48> defaultBitsStr;

                        intToalphaBits(pCvar->GetMaxInt(), allBitsStr);
                        intToalphaBits(pCvar->GetInteger(), valueBitsStr);
                        intToalphaBits(pCvar->GetDefaultInt(), defaultBitsStr);

                        value.appendFmt(" (%s)", valueBitsStr.c_str());
                        defaultValue.appendFmt(" (%s)", defaultBitsStr.c_str());
                        domain.appendFmt("Domain is bitfield of the following: '%s' commands: b, b+, b-, b^", allBitsStr.c_str());
                    }
                    else {
                        domain.appendFmt("Domain is any interger between: %d and %d",
                            pCvar->GetMinInt(),
                            pCvar->GetMaxInt());
                    }
                }
                else if (pCvar->GetType() == VarFlag::FLOAT) {
                    domain.appendFmt("Domain is any real number between: %g and %g",
                        pCvar->GetMin(),
                        pCvar->GetMax());
                }
                else if (pCvar->GetType() == VarFlag::COLOR) {
                    domain.appendFmt("Domain is 1 or 4 real numbers between: 0.0 and 1.0");
                }

                height = fCharHeight * 2.5f;
                width = core::Max(
                    width,
                    pFont_->GetTextSize(nameStr.begin(), nameStr.end(), ctx).x,
                    pFont_->GetTextSize(defaultStr.begin(), defaultStr.end(), ctx).x);

                width += nameValueSpacing; // name - value spacing.
                xposValueOffset = width;
                width += core::Max(
                    pFont_->GetTextSize(value.begin(), value.end(), ctx).x,
                    pFont_->GetTextSize(defaultValue.begin(), defaultValue.end(), ctx).x);

                float box2Offset = 5;
                float height2 = fCharHeight * (domain.isEmpty() ? 1.5f : 2.5f);
                float width2 = core::Max(
                    width,
                    pFont_->GetTextSize(description.begin(), description.end(), ctx).x,
                    pFont_->GetTextSize(domain.begin(), domain.end(), ctx).x);

                width += 15;  // add a few pixels.
                width2 += 15; // add a few pixels.

                if (isColorVar) {
                    width += colorBoxWidth + colorBoxPadding;
                }

                // Draw the boxes
                pPrimContext_->drawQuad(xpos, ypos, width, height, col, console_input_box_color_border);
                pPrimContext_->drawQuad(xpos, ypos + height + box2Offset, width2, height2, col, console_input_box_color_border);

                if (isColorVar) {
                    float colxpos = xpos + width - (colorBoxPadding + colorBoxWidth);
                    float colypos = ypos + (colorBoxPadding * 2.f);
                    float colHeight = fCharHeight - colorBoxPadding * 2;

                    // draw the colors :D !
                    CVarColRef* PColVar = static_cast<CVarColRef*>(pCvar);

                    pPrimContext_->drawQuad(colxpos, colypos, colorBoxWidth, colHeight, PColVar->GetColor(), Col_Black);

                    // 2nd box.
                    colypos += fCharHeight;

                    pPrimContext_->drawQuad(colxpos, colypos, colorBoxWidth, colHeight, PColVar->GetDefaultColor(), Col_Black);

                    // How about a preview of the new color?
                    string::const_str pos = inputBuffer_.find(nameStr.c_str());
                    if (pos) {
                        //	static core::StackString<64> lastValue;
                        core::StackString<64> colorStr(&pos[nameStr.length()],
                            inputBuffer_.end());

                        colorStr.trim();

                        // save a lex if string is the same.
                        // slap myself, then it's not drawn lol.
                        // add some checks if still needs drawing if the lex time is a issue.
                        if (!colorStr.isEmpty()) // && colorStr != lastValue)
                        {
                            //	lastValue = colorStr;

                            // parse it.
                            Color previewCol;
                            if (CVarColRef::ColorFromString(colorStr.c_str(), previewCol)) {
                                // draw a box on the end cus i'm a goat.
                                pPrimContext_->drawQuad(xpos + width + 5, ypos, height, height, previewCol, Col_Black);
                            }
                        }
                    }
                }

                // draw da text baby!
                xpos += 5;

                ctx.SetColor(Col_Whitesmoke);
                pPrimContext_->drawText(xpos, ypos, ctx, nameStr.begin(), nameStr.end());
                pPrimContext_->drawText(xpos + xposValueOffset, ypos, ctx, value.begin(), value.end());

                ctx.SetColor(Col_Darkgray);
                ypos += fCharHeight;
                pPrimContext_->drawText(xpos, ypos, ctx, defaultStr.begin(), defaultStr.end());
                pPrimContext_->drawText(xpos + xposValueOffset, ypos, ctx, defaultValue.begin(), defaultValue.end());

                ypos += fCharHeight * 1.5f;
                ypos += box2Offset;

                ctx.SetColor(Col_Darkgray);
                pPrimContext_->drawText(xpos, ypos, ctx, description.begin(), description.end());

                if (!domain.isEmpty()) {
                    ypos += fCharHeight;
                    pPrimContext_->drawText(xpos, ypos, ctx, domain.begin(), domain.end());
                }
            }
            else if (isSingleCmd) {
                AutoResult& result = *results.begin();
                const core::string& descStr = result.pCmd->Desc;

                const float box2Offset = 5.f;
                const float descWidth = core::Max(width, pFont_->GetTextSize(descStr.begin(), descStr.end(), ctx).x) + 10.f;

                width = core::Max(width, pFont_->GetTextSize(result.pName, result.pName + core::strUtil::strlen(result.pName), ctx).x);
                width += 10; // add a few pixels.
                height += fCharHeight;

                pPrimContext_->drawQuad(xpos, ypos, width, height, col, console_output_box_color_border);
                pPrimContext_->drawQuad(xpos, ypos + height + box2Offset, descWidth, height, col, console_input_box_color_border);

                xpos += 5.f;

                // cmd color
                ctx.SetColor(console_cmd_color);
                pPrimContext_->drawText(xpos, ypos, ctx, result.pName);

                ypos += fCharHeight;
                ypos += 5.f;
                ypos += box2Offset;

                ctx.SetColor(Col_Whitesmoke);
                pPrimContext_->drawText(xpos, ypos, ctx, descStr.begin(), descStr.end());
            }
            else {
                Results::const_iterator resIt;

                resIt = results.begin();
                for (; resIt != results.end(); ++resIt) {
                    width = core::Max(width, pFont_->GetTextSize(resIt->pName, resIt->pName + core::strUtil::strlen(resIt->pName), ctx).x);
                    height += fCharHeight;
                }

                width += 10; // add a few pixels.
                             //		height += 5; // add a few pixels.

                pPrimContext_->drawQuad(xpos, ypos, width, height, col, console_output_box_color_border);

                xpos += 5;

                resIt = results.begin();

                for (int idx = 0; resIt != results.end(); ++resIt, idx++) {
                    if (autoCompleteIdx_ >= 0 && autoCompleteIdx_ == idx) {
                        ctx.SetColor(Col_Darkturquoise);
                    }
                    else if (resIt->var) {
                        ctx.SetColor(Col_Whitesmoke);
                    }
                    else {
                        ctx.SetColor(console_cmd_color);
                    }

                    pPrimContext_->drawText(xpos, ypos, ctx, resIt->pName);

                    ypos += fCharHeight;
                }
            }
        }

        // the input
        if (!inputBuffer_.isEmpty()) {
            const Vec2i& res = renderRes_;

            const float width = res.x - 10.f;
            const Vec2f inputFontSize(static_cast<float>(CONSOLE_INPUT_FONT_SIZE), static_cast<float>(CONSOLE_INPUT_FONT_SIZE));

            ctx.SetSize(inputFontSize);
            ctx.SetColor(txtCol);
            ctx.flags.Set(font::DrawTextFlag::CLIP);
            ctx.clip.set(0, 0, width, std::numeric_limits<float>::max());

            core::string::const_str space = inputBuffer_.find(' ');
            if (space) {
                core::StackString<128> temp(inputBuffer_.begin(), space);

                // preserve any colors.
                core::StackString<128> temp2;

                if (const char* pCol = temp.find('^')) {
                    if (core::strUtil::IsDigit(pCol[1])) {
                        temp2.appendFmt("^%c", pCol[1]);
                    }
                }

                //	temp2.append(&inputBuffer_[space]);
                temp2.append(space);

                pPrimContext_->drawText(txtPos.x, txtPos.y, ctx, temp.begin(), temp.end());
                ctx.SetColor(Col_White);
                txtPos.x += pFont_->GetTextSize(temp.begin(), temp.end(), ctx).x;
                pPrimContext_->drawText(txtPos.x, txtPos.y, ctx, temp2.begin(), temp2.end());
            }
            else {
                pPrimContext_->drawText(txtPos.x, txtPos.y, ctx, inputBuffer_.begin(), inputBuffer_.end());
            }
        }
    }
}

// ----------------------

void XConsole::copy(void)
{
    core::clipboard::setText(inputBuffer_.begin(), inputBuffer_.end());
}

void XConsole::paste(void)
{
    core::clipboard::ClipBoardBuffer buffer;
    const char* pTxt = core::clipboard::getText(buffer);

    if (pTxt) {
        // insert it at current pos.
        inputBuffer_.insert(cursorPos_, pTxt);
        // add to length
        cursorPos_ += safe_static_cast<int32_t, size_t>(core::strUtil::strlen(pTxt));
    }
    else {
        X_LOG1("Console", "Failed to paste text to console");
    }
}

// ----------------------

void XConsole::OnCoreEvent(const CoreEventData& ed)
{
    if (ed.event == CoreEvent::RENDER_RES_CHANGED) {
        renderRes_.x = static_cast<int32_t>(ed.renderRes.width);
        renderRes_.y = static_cast<int32_t>(ed.renderRes.height);
    }
}

// ----------------------

void XConsole::listCommands(const char* pSearchPatten)
{
    core::Array<ConsoleCommand*> sorted_cmds(g_coreArena);
    sorted_cmds.setGranularity(cmdMap_.size());

    for (auto itrCmd = cmdMap_.begin(); itrCmd != cmdMap_.end(); ++itrCmd) {
        ConsoleCommand& cmd = itrCmd->second;

        if (!pSearchPatten || strUtil::WildCompare(pSearchPatten, cmd.Name)) {
            sorted_cmds.append(&cmd);
        }
    }

    sortCmdsByName(sorted_cmds);

    X_LOG0("Console", "------------ ^8Commands(%" PRIuS ")^7 ------------", sorted_cmds.size());

    ConsoleCommand::FlagType::Description dsc;
    for (const auto* pCmd : sorted_cmds) {
        X_LOG0("Command", "^2\"%s\"^7 [^1%s^7] Desc: \"%s\"", pCmd->Name.c_str(),
            pCmd->Flags.ToString(dsc), pCmd->Desc.c_str());
    }

    X_LOG0("Console", "------------ ^8Commands End^7 ------------");
}

void XConsole::listVariables(const char* pSearchPatten)
{
    // i'm not storing the vars in a ordered map since it's slow to get them.
    // and i only need order when priting them.
    // so it's not biggy doing the sorting here.
    core::Array<ICVar*> sorted_vars(g_coreArena);
    sorted_vars.setGranularity(varMap_.size());

    for (const auto& it : varMap_) {
        ICVar* var = it.second;

        if (!pSearchPatten || strUtil::WildCompare(pSearchPatten, var->GetName())) {
            sorted_vars.emplace_back(var);
        }
    }

    sortVarsByName(sorted_vars);

    X_LOG0("Console", "-------------- ^8Vars(%" PRIuS ")^7 --------------", sorted_vars.size());

    ICVar::FlagType::Description dsc;
    for (const auto& var : sorted_vars) {
        X_LOG0("Dvar", "^2\"%s\"^7 [^1%s^7] Desc: \"%s\"", var->GetName(), var->GetFlags().ToString(dsc), var->GetDesc());
    }

    X_LOG0("Console", "-------------- ^8Vars End^7 --------------");
}

void XConsole::listVariablesValues(const char* pSearchPatten)
{
    // i'm not storing the vars in a ordered map since it's slow to get them.
    // and i only need order when priting them.
    // so it's not biggy doing the sorting here.
    core::Array<ICVar*> sorted_vars(g_coreArena);
    sorted_vars.setGranularity(varMap_.size());

    for (const auto& it : varMap_) {
        ICVar* var = it.second;

        if (!pSearchPatten || strUtil::WildCompare(pSearchPatten, var->GetName())) {
            sorted_vars.emplace_back(var);
        }
    }

    sortVarsByName(sorted_vars);

    X_LOG0("Console", "-------------- ^8Vars(%" PRIuS ")^7 --------------", sorted_vars.size());

    for (const auto* pVar : sorted_vars) {
        displayVarValue(pVar);
    }

    X_LOG0("Console", "-------------- ^8Vars End^7 --------------");
}


// ==================================================================

void XConsole::Command_Exec(IConsoleCmdArgs* pCmd)
{
    if (pCmd->GetArgCount() != 2) {
        X_WARNING("Console", "exec <filename>");
        return;
    }

    const char* pFilename = pCmd->GetArg(1);

    loadAndExecConfigFile(pFilename);
}

void XConsole::Command_History(IConsoleCmdArgs* pCmd)
{
    const char* pSearch = nullptr;
    if (pCmd->GetArgCount() == 2) {
        pSearch = pCmd->GetArg(1);
    }

    X_LOG0("Console", "-------------- ^8History^7 ---------------");
    X_LOG_BULLET;

    int32_t idx = 0;
    for (auto& history : cmdHistory_) {
        if (!pSearch || history.find(pSearch)) {
            X_LOG0("Console", "> %" PRIi32 " %s", idx, history.c_str());
        }

        ++idx;
    }

    X_LOG0("Console", "------------ ^8History End^7 -------------");
}

void XConsole::Command_Help(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    X_LOG0("Console", "------- ^8Help^7 -------");
    X_LOG_BULLET;
    X_LOG0("Console", "listcmds: lists avaliable commands");
    X_LOG0("Console", "listdvars: lists dvars");
    X_LOG0("Console", "listbinds: lists all the bind");
}

void XConsole::Command_ListCmd(IConsoleCmdArgs* pCmd)
{
    // optional search criteria
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listCommands(pSearchPatten);
}

void XConsole::Command_ListDvars(IConsoleCmdArgs* pCmd)
{
    // optional search criteria
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listVariables(pSearchPatten);
}

void XConsole::Command_ListDvarsValues(IConsoleCmdArgs* pCmd)
{
    // optional search criteria
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listVariablesValues(pSearchPatten);
}

void XConsole::Command_Exit(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    // we want to exit I guess.
    // dose this check even make sense?
    // it might for dedicated server.
    if (gEnv && gEnv->pCore && gEnv->pCore->GetGameWindow()) {
        gEnv->pCore->GetGameWindow()->Close();
    }
    else {
        X_ERROR("Cmd", "Failed to exit game");
    }
}

void XConsole::Command_Echo(IConsoleCmdArgs* pCmd)
{
    // we only print the 1st arg ?
    StackString<1024> txt;

    size_t TotalLen = 0;
    size_t Len = 0;

    for (size_t i = 1; i < pCmd->GetArgCount(); i++) {
        const char* str = pCmd->GetArg(i);

        Len = strlen(str);

        if ((TotalLen + Len) >= 896) // we need to make sure other info like channle name fit into 1024
        {
            X_WARNING("Echo", "input too long truncating");

            // trim it to be sorter then total length.
            // but not shorter than it's length.
            size_t new_len = core::Min(Len, 896 - TotalLen);
            //									  o o
            // watch this become a bug one day.  ~~~~~
            const_cast<char*>(str)[new_len] = '\0';

            txt.appendFmt("%s", str);
            break; // stop adding.
        }

        txt.appendFmt("%s ", str);

        TotalLen = txt.length();
    }

    X_LOG0("echo", txt.c_str());
}

void XConsole::Command_VarReset(IConsoleCmdArgs* pCmd)
{
    if (pCmd->GetArgCount() != 2) {
        X_WARNING("Console", "vreset <var_name>");
        return;
    }

    // find the var
    ICVar* pCvar = getCVar(pCmd->GetArg(1));
    if (!pCvar) {
        X_ERROR("Console", "var with name \"%s\" not found", pCmd->GetArg(1));
        return;
    }

    pCvar->Reset();

    displayVarValue(pCvar);
}

void XConsole::Command_VarDescribe(IConsoleCmdArgs* pCmd)
{
    if (pCmd->GetArgCount() != 2) {
        X_WARNING("Console", "vdesc <var_name>");
        return;
    }

    // find the var
    ICVar* pCvar = getCVar(pCmd->GetArg(1));
    if (!pCvar) {
        X_ERROR("Console", "var with name \"%s\" not found", pCmd->GetArg(1));
        return;
    }

    displayVarInfo(pCvar, true);
}

void XConsole::Command_Bind(IConsoleCmdArgs* pCmd)
{
    size_t Num = pCmd->GetArgCount();

    if (Num < 3) {
        X_WARNING("Console", "bind <key_combo> '<cmd>'");
        return;
    }

    core::StackString<1024> cmd;

    for (size_t i = 2; i < Num; i++) {
        cmd.append(pCmd->GetArg(i));
        if (i + 1 == Num)
            cmd.append(';', 1);
        else
            cmd.append(' ', 1);
    }

    addBind(pCmd->GetArg(1), cmd.c_str());
}

void XConsole::Command_BindsClear(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    clearAllBinds();
}

void XConsole::Command_BindsList(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    struct PrintBinds : public IKeyBindDumpSink
    {
        virtual void OnKeyBindFound(const char* Bind, const char* Command)
        {
            X_LOG0("Console", "Key: %s Cmd: \"%s\"", Bind, Command);
        }
    };

    PrintBinds print;

    X_LOG0("Console", "--------------- ^8Binds^7 ----------------");

    listbinds(&print);

    X_LOG0("Console", "------------- ^8Binds End^7 --------------");
}

void XConsole::Command_SetVarArchive(IConsoleCmdArgs* Cmd)
{
    size_t Num = Cmd->GetArgCount();

    if (Num != 3 && Num != 5 && Num != 6) {
        X_WARNING("Console", "seta <var> <val>");
        return;
    }

    const char* pVarName = Cmd->GetArg(1);

    if (ICVar* pCBar = getCVar(pVarName)) {
        VarFlag::Enum type = pCBar->GetType();
        if (type == VarFlag::COLOR || type == VarFlag::VECTOR) {
            // just concat themm all into a string
            core::StackString512 merged;

            for (size_t i = 2; i < Num; i++) {
                merged.append(Cmd->GetArg(i));
                merged.append(" ");
            }

            pCBar->Set(merged.c_str());
        }
        else {
            if (Num != 3) {
                X_WARNING("Console", "seta <var> <val>");
                return;
            }

            pCBar->Set(Cmd->GetArg(2));
        }

        pCBar->SetFlags(pCBar->GetFlags() | VarFlag::ARCHIVE);
    }
    else {
        core::string merged;

        for (size_t i = 2; i < Num; i++) {
            merged.append(Cmd->GetArg(i));
            merged.append(" ");
        }

        merged.trimRight();

        // we just add it to config cmd map
        auto it = varArchive_.find(X_CONST_STRING(pVarName));
        if (it == varArchive_.end()) {
            varArchive_.insert(ConfigCmdsMap::iterator::value_type(pVarName, merged));
        }
        else {
            it->second = merged;
        }
    }
}

void XConsole::Command_ConsoleShow(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    showConsole(XConsole::consoleState::OPEN);
}

void XConsole::Command_ConsoleHide(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    showConsole(XConsole::consoleState::CLOSED);
}

void XConsole::Command_ConsoleToggle(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    toggleConsole(false);
}

void XConsole::Command_SaveModifiedVars(IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    saveChangedVars();
}

X_NAMESPACE_END