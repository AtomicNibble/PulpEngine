#include "stdafx.h"

#include "Console.h"
#include "ConsoleNULL.h"
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

#include "Platform\Window.h"

#include <algorithm>



X_NAMESPACE_BEGIN(core)

namespace
{
	static const size_t VAR_ALLOCATION_SIZE =
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		sizeof(CVarString<CVarBaseConst>),
		sizeof(CVarInt<CVarBaseConst>)>::value,
		sizeof(CVarFloat<CVarBaseConst>)>::value,
		sizeof(CVarString<CVarBaseHeap>)>::value,
		sizeof(CVarInt<CVarBaseHeap>)>::value,
		sizeof(CVarFloat<CVarBaseHeap>)>::value,
		sizeof(CVarFloatRef)>::value,
		sizeof(CVarIntRef)>::value,
		sizeof(CVarColRef)>::value;

	static const size_t VAR_ALLOCATION_ALIGNMENT = 
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		core::Max_static_size<
		X_ALIGN_OF(CVarString<CVarBaseConst>),
		X_ALIGN_OF(CVarInt<CVarBaseConst>)>::value,
		X_ALIGN_OF(CVarFloat<CVarBaseConst>)>::value,
		X_ALIGN_OF(CVarString<CVarBaseHeap>)>::value,
		X_ALIGN_OF(CVarInt<CVarBaseHeap>)>::value,
		X_ALIGN_OF(CVarFloat<CVarBaseHeap>)>::value,
		X_ALIGN_OF(CVarFloatRef)>::value,
		X_ALIGN_OF(CVarIntRef)>::value,
		X_ALIGN_OF(CVarColRef)>::value;


	static void sortVarsByName(core::Array<core::ICVar*>& vars)
	{
		using namespace std;

		std::sort(vars.begin(), vars.end(),
			[](core::ICVar* a, core::ICVar* b){
			return strcmp(a->GetName(), b->GetName()) < 0;
		}
		);
	}

	static void sortCmdsByName(core::Array<core::ConsoleCommand*>& vars)
	{
		using namespace std;

		std::sort(vars.begin(), vars.end(),
			[](core::ConsoleCommand* a, core::ConsoleCommand* b){
			return strcmp(a->Name, b->Name) < 0;
		}
		);
	}

	class CommandParser
	{
	public:
		CommandParser(const char* cmd) :
			begin_(cmd),
			end_(cmd + core::strUtil::strlen(cmd))
		{}


		bool extractCommand(core::StringRange<char>& cmd)
		{
			const char* cur = begin_;
			const char* end = end_;

			while (cur < end)
			{
				char ch = *cur++;
				switch (ch)
				{
					case '\'':
					case '\"':
						while ((*cur++ != ch) && cur < end);
						break;
					case '\n':
					case '\r':
					case ';':
					case '\0':
					{
						// we class this as end of a command.	
						cmd = StringRange<char>(begin_, cur);
						begin_ = cur;
						return true;
					}
					break;
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

} // namespace


	void Command_Exec(IConsoleCmdArgs* Cmd)
	{
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		if (Cmd->GetArgCount() != 2)
		{
			X_WARNING("Console", "exec <filename>");
			return;
		}

		const char* filename = Cmd->GetArg(1);

		pConsole->LoadConfig(filename);
	}

	void Command_Help(IConsoleCmdArgs* Cmd)
	{
		X_UNUSED(Cmd);

		X_LOG0("Console", "------- ^8Help^7 -------");
		X_LOG_BULLET;
		X_LOG0("Console", "listcmds: lists avaliable commands");
		X_LOG0("Console", "listdvars: lists dvars");
		X_LOG0("Console", "listbinds: lists all the bind");
	}

	void Command_ListCmd(IConsoleCmdArgs* Cmd)
	{
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		// optional search criteria
		const char* searchPatten = nullptr;

		if (Cmd->GetArgCount() >= 2) {
			searchPatten = Cmd->GetArg(1);
		}

		pConsole->ListCommands(searchPatten);
	}

	void Command_ListDvars(IConsoleCmdArgs* Cmd)
	{
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		// optional search criteria
		const char* searchPatten = nullptr;

		if (Cmd->GetArgCount() >= 2) {
			searchPatten = Cmd->GetArg(1);
		}

		pConsole->ListVariables(searchPatten);
	}

	void Command_Exit(IConsoleCmdArgs* Cmd)
	{
		X_UNUSED(Cmd);

		// we want to exit I guess.
		// dose this check even make sense?
		// it might for dedicated server.
		if (gEnv && gEnv->pCore && gEnv->pCore->GetGameWindow())
			gEnv->pCore->GetGameWindow()->Close();
		else
			X_ERROR("Cmd", "Failed to exit game");
	}

	void Command_Echo(IConsoleCmdArgs* Cmd)
	{
		// we only print the 1st arg ?
		StackString<1024> txt;

		size_t TotalLen = 0;
		size_t Len = 0;

		for (size_t i = 1; i < Cmd->GetArgCount(); i++)
		{
			const char* str = Cmd->GetArg(i);

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

	void Command_Wait(IConsoleCmdArgs* Cmd)
	{
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		if (Cmd->GetArgCount() != 2)
		{
			X_WARNING("Console", "wait <seconds>");
			return;
		}


		pConsole->WaitSeconds_.SetSeconds(atof(Cmd->GetArg(1)));
		pConsole->WaitSeconds_ += gEnv->pTimer->GetFrameStartTime();
	}

	void Command_VarReset(IConsoleCmdArgs* Cmd)
	{
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		if (Cmd->GetArgCount() != 2)
		{
			X_WARNING("Console", "vreset <var_name>");
			return;
		}

		// find the var
		ICVar* cvar = pConsole->GetCVar(Cmd->GetArg(1));

		cvar->Reset();

		pConsole->DisplayVarValue(cvar);
	}

	void Command_Bind(IConsoleCmdArgs* Cmd)
	{
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		size_t Num = Cmd->GetArgCount();

		if (Num < 3)
		{
			X_WARNING("Console", "bind <key_combo> '<cmd>'");
			return;
		}

		core::StackString<1024> cmd;

		for (size_t i = 2; i < Num; i++)
		{
			cmd.append(Cmd->GetArg(i));
			if (i + 1 == Num)
				cmd.append(';', 1);
			else
				cmd.append(' ', 1);
		}

		pConsole->AddBind(Cmd->GetArg(1), cmd.c_str());
	}

	void Command_BindsClear(IConsoleCmdArgs* Cmd)
	{
		X_UNUSED(Cmd);
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);
		pConsole->ClearAllBinds();
	}

	void Command_BindsList(IConsoleCmdArgs* Cmd)
	{
		X_UNUSED(Cmd);
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		struct PrintBinds : public IKeyBindDumpSink {
			virtual void OnKeyBindFound(const char* Bind, const char* Command){
				X_LOG0("Console", "Key: %s Cmd: \"%s\"", Bind, Command);
			}
		};

		PrintBinds print;

		X_LOG0("Console", "--------------- ^8Binds^7 ----------------");

		pConsole->Listbinds(&print);

		X_LOG0("Console", "------------- ^8Binds End^7 --------------");
	}

	void Command_SetVarArchive(IConsoleCmdArgs* Cmd)
	{
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		size_t Num = Cmd->GetArgCount();

		if (Num != 3)
		{
			X_WARNING("Console", "seta <var> <val>");
			return;
		}

		if (ICVar* pCBar = pConsole->GetCVar(Cmd->GetArg(1)))
		{
			pCBar->Set(Cmd->GetArg(2));

			pCBar->SetFlags(pCBar->GetFlags() | VarFlag::ARCHIVE);
		}
		else
		{
			// we need to work out what kinda value it is.
			// int, float, string.
			const char* start = Cmd->GetArg(2);
			char* End;
			// long int val = strtol(Cmd->GetArg(2), &End, 0);
			float valf = strtof(start, &End);


			if (valf != 0)
			{
				// using the End var is safe since we the condition above checks parsing was valid.
				if (fmod(valf, 1) == 0.f && !strUtil::Find(start, End, '.'))
				{
					pConsole->ConfigRegisterInt(Cmd->GetArg(1), (int)valf, 1, 0, 0, "");
				}
				else
				{
					pConsole->ConfigRegisterFloat(Cmd->GetArg(1), valf, 1, 0, 0, "");
				}
			}
			else
			{
				pConsole->ConfigRegisterString(Cmd->GetArg(1), Cmd->GetArg(2), 0, "");
			}

		}
	}

	void Command_ConsoleShow(IConsoleCmdArgs* Cmd)
	{
		X_UNUSED(Cmd);
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		pConsole->ShowConsole(XConsole::consoleState::OPEN);
	}

	void Command_ConsoleHide(IConsoleCmdArgs* Cmd)
	{
		X_UNUSED(Cmd);
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		pConsole->ShowConsole(XConsole::consoleState::CLOSED);
	}

	void Command_ConsoleToggle(IConsoleCmdArgs* Cmd)
	{
		X_UNUSED(Cmd);
		XConsole* pConsole = static_cast<XConsole*>(gEnv->pConsole);

		pConsole->ToggleConsole();
	}



// ==================================================

ConsoleCommand::ConsoleCommand() : pFunc(0) // flags default con is (0)
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
void ConsoleCommandArgs::TokenizeString(const char *begin, const char* end)
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
	while (char ch = *commandLine++)
	{
		if (argNum_ == MAX_COMMAND_ARGS) {
			return;
		}

		switch (ch)
		{
		case '\'':
		case '\"':
		{
			while ((*commandLine++ != ch) && *commandLine); // find end

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
		default:
		{
			if ((*commandLine == ' ') || !*commandLine)
			{
				argv_[argNum_] = tokenized_ + totalLen;
				argNum_++;

				if (*start == '#')
				{
					++start;

					core::StackString<256> name(start, commandLine);

					// it's a var name.
					ICVar* var = gEnv->pConsole->GetCVar(name.c_str());

					if (var)
					{
						name.clear();

						name.append(var->GetString());

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
		}
		break;
		} // switch
	} // while
}

//////////////////////////////////////////////////////////////////////////

const char* XConsole::CMD_HISTORY_FILE_NAME = "cmdHistory.txt";
const char* XConsole::CONFIG_FILE_EXTENSION = "cfg";

int XConsole::console_debug = 0;
int XConsole::console_case_sensitive = 0;
int XConsole::console_save_history = 0;
Color XConsole::console_input_box_color;
Color XConsole::console_input_box_color_border;
Color XConsole::console_output_box_color;
Color XConsole::console_output_box_color_border;
Color XConsole::console_output_box_channel_color;
Color XConsole::console_output_scroll_bar_color;
Color XConsole::console_output_scroll_bar_slider_color;
int	  XConsole::console_output_draw_channel;
int	XConsole::console_buffer_size = 0;
int XConsole::console_disable_mouse = 0;


//////////////////////////////////////////////////////////////////////////


XConsole::DeferredCommand::DeferredCommand(const string& command, bool silentMode) :
	command(command), 
	silentMode(silentMode)
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
	VarMap_(g_coreArena),
	CmdMap_(g_coreArena),
	Binds_(g_coreArena),
	varHeap_(
		bitUtil::RoundUpToMultiple<size_t>(
			VarPool::getMemoryRequirement(VAR_ALLOCATION_SIZE) * VAR_MAX,
			VirtualMem::GetPageSize()
		)
	),
	varAllocator_(varHeap_.start(), varHeap_.end(),
		VarPool::getMemoryRequirement(VAR_ALLOCATION_SIZE),
		VarPool::getMemoryAlignmentRequirement(VAR_ALLOCATION_ALIGNMENT),
		VarPool::getMemoryOffsetRequirement()
	),
	varArena_(&varAllocator_,"VarArena")
{
	HistoryPos_ = -1;
	CursorPos_ = 0;
	ScrollPos_ = 0;

	g_coreArena->addChildArena(&varArena_);

	consoleState_ = consoleState::CLOSED;

	pCore_ = nullptr;
	pFont_ = nullptr;
	pRender_ = nullptr;
	pInput_ = nullptr;
	pBackground_ = nullptr;

	// Auto goat a boat.
	autoCompleteNum_ = 0;
	autoCompleteIdx_ = -1;
	autoCompleteSelect_ = false;

	// reserve a pickle. (vars are registered before 'Startup' might change that)
	VarMap_.reserve(4096);
	CmdMap_.reserve(1024);
	Binds_.reserve(128);


	repeatEventTimer_ = 0.f;
	repeatEventInterval_ = 0.025f;
	repeatEventInitialDelay_ = 0.5f;
}

XConsole::~XConsole()
{

}

//////////////////////////////////////////////////////////////////////////
void XConsole::Startup(ICore* pCore)
{
	X_ASSERT_NOT_NULL(pCore);
	X_LOG0("Console", "Starting console");


	pCore_ = pCore;
	pFont_ = pCore->GetIFontSys()->GetFont("default");
	pRender_ = pCore->GetIRender();
	pInput_ = pCore->GetIInput();

	X_ASSERT_NOT_NULL(pFont_);
//	X_ASSERT_NOT_NULL(pRender_); // can be null i think, ask wincat.
	X_ASSERT_NOT_NULL(pInput_);

	// we want input events plooxx.
	pInput_->AddConsoleEventListener(this);
	
	// add this as a logger.
	pCore->GetILog()->AddLogger(&logger_);

	// load a texture baby!
	pBackground_ = pRender_->LoadTexture("Textures/white.dds", 
		texture::TextureFlags::DONT_STREAM);

	ADD_CVAR_REF_NO_NAME(console_debug, 0, 0, 1, VarFlag::SYSTEM | VarFlag::CHEAT, 
		"Debugging for console operations. 0=off 1=on");
	ADD_CVAR_REF_NO_NAME(console_case_sensitive, 0, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
		"Console input auto complete is case-sensitive");
	ADD_CVAR_REF_NO_NAME(console_save_history, 1, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
		"Saves command history to file");
	ADD_CVAR_REF_NO_NAME(console_buffer_size, 1000, 1, 10000, VarFlag::SYSTEM, 
		"Size of the log buffer");
	ADD_CVAR_REF_NO_NAME(console_output_draw_channel, 1, 0, 1, VarFlag::SYSTEM, 
		"Draw the channel in a diffrent color. 0=disabled 1=enabled");

	ADD_CVAR_REF_COL_NO_NAME(console_input_box_color, Color(0.3f, 0.3f, 0.3f, 0.75f), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console input box color");
	ADD_CVAR_REF_COL_NO_NAME(console_input_box_color_border, Color(0.1f, 0.1f, 0.1f, 1.0f), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console input box color");
	ADD_CVAR_REF_COL_NO_NAME(console_output_box_color, Color(0.2f, 0.2f, 0.2f, 0.9f), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output box color");
	ADD_CVAR_REF_COL_NO_NAME(console_output_box_color_border, Color(0.1f, 0.1f, 0.1f, 1.0f), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output box color");
	ADD_CVAR_REF_COL_NO_NAME(console_output_box_channel_color, Color(0.15f, 0.15f, 0.15f, 0.9f), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output box channel color");
	ADD_CVAR_REF_COL_NO_NAME(console_output_scroll_bar_color, Color(0.5f, 0.5f, 0.5f, 0.5f),
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output scroll bar color");
	ADD_CVAR_REF_COL_NO_NAME(console_output_scroll_bar_slider_color, Color(0.0f, 0.0f, 0.0f, 0.9f), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Console output scroll bar slider color");
	ADD_CVAR_REF_NO_NAME(console_disable_mouse, 2, 0, 2,
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Disable mouse input when console open."
		" 1=expanded only 2=always");


	ADD_COMMAND("exec", Command_Exec, VarFlag::SYSTEM, "executes a file(.cfg)");
	ADD_COMMAND("help", Command_Help, VarFlag::SYSTEM, "displays help info");
	ADD_COMMAND("listcmds", Command_ListCmd, VarFlag::SYSTEM, "lists avaliable commands");
	ADD_COMMAND("listdvars", Command_ListDvars, VarFlag::SYSTEM, "lists dvars");
	ADD_COMMAND("exit", Command_Exit, VarFlag::SYSTEM, "closes the game");
	ADD_COMMAND("quit", Command_Exit, VarFlag::SYSTEM, "closes the game");
	ADD_COMMAND("echo", Command_Echo, VarFlag::SYSTEM, "prints text in argument, prefix dvar's with # to print value.");
	ADD_COMMAND("wait", Command_Wait, VarFlag::SYSTEM, "waits a given number of seconds before processing the next commands");
	ADD_COMMAND("vreset", Command_VarReset, VarFlag::SYSTEM, "resets a variable to it's default value");
	ADD_COMMAND("seta", Command_SetVarArchive, VarFlag::SYSTEM, "set a var and flagging it to be archived");

	ADD_COMMAND("bind", Command_Bind, VarFlag::SYSTEM, "binds a key to a action Eg: bind shift a 'echo hello';");
	ADD_COMMAND("clearbinds", Command_BindsClear, VarFlag::SYSTEM, "clears all binds");
	ADD_COMMAND("listbinds", Command_BindsList, VarFlag::SYSTEM, "lists all the binds");


	ADD_COMMAND("console_show", Command_ConsoleShow, VarFlag::SYSTEM, "opens the console");
	ADD_COMMAND("console_hide", Command_ConsoleHide, VarFlag::SYSTEM, "hides the console");
	ADD_COMMAND("console_toggle", Command_ConsoleToggle, VarFlag::SYSTEM, "toggle the console");

	// hot reload
	pCore->GetHotReloadMan()->addfileType(this, CONFIG_FILE_EXTENSION);

	if (console_save_history) {
		LoadCmdHistory();
	}
}

void XConsole::ShutDown(void)
{
	X_LOG0("Console", "Shutting Down");

	// check if core failed to init.
	if (pCore_)
	{
		pCore_->GetHotReloadMan()->addfileType(nullptr, CONFIG_FILE_EXTENSION);
		pCore_->GetILog()->RemoveLogger(&logger_);
	}
	unregisterInputListener();

	// clear up vars.
	if (!VarMap_.empty())
	{
		while (!VarMap_.empty()) {
			VarMap_.begin()->second->Release();
		}

		VarMap_.clear();
	}

	InputBuffer_.clear();
	RefString_.clear();
	CmdHistory_.clear();
}

void XConsole::SaveChangedVars(void)
{
	ConsoleVarMapItor itrVar, itrVarEnd = VarMap_.end();

	if (VarMap_.empty()) {
		X_WARNING("Console", "Skipping saving of modified vars. no registerd vars.");
		return;
	}

	X_LOG0("Console", "Saving moified vars");

	core::XFileScoped file;
	core::fileModeFlags mode;

	mode.Set(fileMode::WRITE);
	mode.Set(fileMode::RECREATE);

	if (file.openFile("config//user_config.cfg", mode))
	{
		file.writeStringNNT("// auto generated\n");

		for (itrVar = VarMap_.begin(); itrVar != itrVarEnd; ++itrVar)
		{
			ICVar* pVar = itrVar->second;
			ICVar::FlagType flags = pVar->GetFlags();

			// we always save 'ARCHIVE' and only save 'SAVE_IF_CHANGED' if 'MODIFIED'
			bool save = (flags.IsSet(VarFlag::SAVE_IF_CHANGED) &&
				flags.IsSet(VarFlag::MODIFIED)) ||
				flags.IsSet(VarFlag::ARCHIVE);

			if (save)
			{
				// save out name + value.
				const char* pName = pVar->GetName();
				const char* pValue = pVar->GetString();

				file.writeStringNNT("seta ");
				file.writeStringNNT(pName);
				file.write(' ');
				file.writeStringNNT(pValue);
				file.write('\n');
			}
		}
	}
	else
	{
		X_ERROR("Console", "Failed to open file for saving modifed vars");
	}
}


void XConsole::unregisterInputListener(void)
{
	pInput_ = gEnv->pInput;
	if (pInput_) {
		pInput_->RemoveConsoleEventListener(this);
	}
}

void XConsole::freeRenderResources(void)
{
	if (pRender_)
	{
		if (pBackground_) {
			pBackground_->release();
		}
	}
}


bool XConsole::OnInputEvent(const input::InputEvent& event)
{
	// NOT OPEN

	if (event.action == input::InputState::RELEASED && isVisable()) {
		repeatEvent_.keyId = input::KeyId::UNKNOWN;
	}

	if (event.action != input::InputState::PRESSED)
	{
		// if open we eat all none mouse
		if (event.deviceId == input::InputDeviceType::KEYBOARD) {
			return isVisable();
		}

		// eat mouse move?
		// Stops the camera moving around when we have console open.
		if (event.deviceId == input::InputDevice::MOUSE)
		{
			if (event.keyId != input::KeyId::MOUSE_Z)
			{
				if (console_disable_mouse == 1) // only if expanded
				{
					return isExpanded();
				}
				if (console_disable_mouse == 2)
				{
					return isVisable();
				}

				return false;
			}
		}
		else
		{
			return false;
		}
	}

	repeatEvent_ = event;
	repeatEventTimer_ = repeatEventInitialDelay_;

	// process key binds when console is hidden
	if (this->consoleState_ == consoleState::CLOSED)
	{
		const char* cmd = 0;

		if (!event.modifiers.IsAnySet())
		{
			cmd = FindBind(event.name);
		}
		else
		{
			// build the key.
			core::StackString<60> bind_name;

			if (event.modifiers.IsSet(input::ModifiersMasks::Ctrl))
			{
				bind_name.append("ctrl ");	
			}
			if (event.modifiers.IsSet(input::ModifiersMasks::Shift))
			{
				bind_name.append("shift ");
			}
			if (event.modifiers.IsSet(input::ModifiersMasks::Alt))
			{
				bind_name.append("alt ");
			}
			if (event.modifiers.IsSet(input::ModifiersMasks::Win))
			{
				bind_name.append("win ");
			}

			bind_name.append(event.name);

			cmd = FindBind(bind_name.c_str());
		}

		if (cmd)
		{
			ExecuteStringInternal(cmd, ExecSource::CONSOLE);
			return true;
		}
	}
	else
	{
		// OPEN
		if (isExpanded()) // you can only scroll a expanded console.
		{
			if (event.keyId == input::KeyId::MOUSE_Z)
			{
				int32_t scaled = static_cast<int32_t>(event.value);

				scaled /= 20;
				if (scaled < 1) {
					scaled = 1;
				}

				ScrollPos_ += scaled;
				if (ScrollPos_ < 0) {
					ScrollPos_ = 0;
				}
				else
				{
					int32_t logSize = static_cast<int32_t>(ConsoleLog_.size());
					int32_t visibleNum = static_cast<int32_t>(MaxVisibleLogLines());

					logSize -= visibleNum;
					logSize += 2;

					if (ScrollPos_ > logSize) {
						ScrollPos_ = logSize;
					}
				}

				return true;
			}
		}

		if (event.keyId != input::KeyId::TAB)
		{
		//	ResetAutoCompletion();
		}

		if (event.keyId == input::KeyId::V && event.modifiers.IsSet(input::ModifiersMasks::Ctrl))
		{
			Paste();
			return false;
		}

		if (event.keyId == input::KeyId::C && event.modifiers.IsSet(input::ModifiersMasks::Ctrl))
		{
			Copy();
			return false;
		}
	}

	// open / Close console (fixed key)
	if (event.keyId == input::KeyId::OEM_8)
	{
		bool expand = event.modifiers.IsSet(input::ModifiersMasks::Shift);
		bool visable = isVisable();

		// clear states.
		pInput_->ClearKeyState();

		if (expand) { // shift + ` dose not close anymore just expands.
			ShowConsole(consoleState::EXPANDED);
		}
		else {
			ToggleConsole(); // toggle it.
		}

		if (!visable) { /// don't clear if already visable, as we are just expanding.
			ClearInputBuffer();
		}
		return true;
	}
	else if (event.keyId == input::KeyId::ESCAPE)
	{
		ClearInputBuffer();
		ShowConsole(consoleState::CLOSED);
		return false;
	}

	return ProcessInput(event);
}

// i want to send input char once.

bool XConsole::OnInputEventChar(const input::InputEvent& event)
{
	if (!isVisable()) {
		return false;
	}

	repeatEvent_ = event;

	AddInputChar(event.inputchar);

	return true;
}

void XConsole::AddInputChar(const char c)
{
	if (c == '`' || c == '¬') { // sent twice.
		return;
	}

	if (CursorPos_ < safe_static_cast<int32_t, size_t>(InputBuffer_.length())) {
		InputBuffer_.insert(CursorPos_, c);
	}
	else {
		InputBuffer_ = InputBuffer_ + c;
	}

	CursorPos_++;

//	X_LOG0("Console Buf", "%s (%i)", InputBuffer_.c_str(), CursorPos_);
}

void XConsole::RemoveInputChar(bool bBackSpace)
{
	if (InputBuffer_.isEmpty()) {
		return;
	}

	if (bBackSpace)
	{
		if (CursorPos_>0)
		{
			InputBuffer_.erase(CursorPos_ - 1, 1);
			CursorPos_--;
		}
	}
	else
	{
		// ho ho h.
		X_ASSERT_NOT_IMPLEMENTED();
	}

//	X_LOG0("Console Buf", "%s (%i)", InputBuffer_.c_str(), CursorPos_);
}


void XConsole::ClearInputBuffer(void)
{
	InputBuffer_ = "";
	CursorPos_ = 0;
}

void XConsole::ExecuteInputBuffer(void)
{
	if (InputBuffer_.isEmpty()) {
		return;
	}

	string Temp = InputBuffer_;

	ClearInputBuffer();

	AddCmdToHistory(Temp.c_str());

	ExecuteStringInternal(Temp.c_str(), ExecSource::CONSOLE);
}

bool XConsole::ProcessInput(const input::InputEvent& event)
{
	using namespace input;

	if (!isVisable()) {
		return false;
	}

	if (event.keyId == KeyId::ENTER || event.keyId == KeyId::NUMPAD_ENTER)
	{
		if (autoCompleteIdx_ >= 0) {
			autoCompleteSelect_ = true;
		}
		else {
			ExecuteInputBuffer();
		}
		return true;
	}
	else if (event.keyId == KeyId::BACKSPACE || event.keyId == KeyId::DELETE)
	{
		// shift + DEL / BACK fully clears
		if (event.modifiers.IsSet(input::ModifiersMasks::Shift)) {
			ClearInputBuffer();
		}
		else {
			RemoveInputChar(true);
		}
		return true;
	}
	else if (event.keyId == KeyId::LEFT_ARROW)
	{
		if (CursorPos_) {  // can we go left?
			CursorPos_--;
		}
		return true;
	}
	else if (event.keyId == KeyId::RIGHT_ARROW)
	{
		// are we pre end ?
		if (CursorPos_ < safe_static_cast<int32_t, size_t>(InputBuffer_.length())) {
			CursorPos_++;
		}
		else if (autoCompleteIdx_ >= 0) {
			autoCompleteSelect_ = true;
		}
		return true;
	}
	else if (event.keyId == KeyId::UP_ARROW)
	{
		if (isAutocompleteVis() && autoCompleteIdx_ >= 0)
		{
			autoCompleteIdx_ = core::Max(-1,--autoCompleteIdx_);

		}
		else
		{
			const char* HistoryLine = GetHistory(CmdHistory::UP);

			if (HistoryLine)
			{
				if (console_debug) {
					X_LOG0("Cmd history", "%s", HistoryLine);
				}

				InputBuffer_ = HistoryLine;
				CursorPos_ = safe_static_cast<int32_t, size_t>(InputBuffer_.size());
			}
		}
		return true;
	}
	else if (event.keyId == KeyId::DOWN_ARROW)
	{
		if (isAutocompleteVis())
		{
			autoCompleteIdx_= core::Min(autoCompleteNum_ - 1, ++autoCompleteIdx_);

		}
		else
		{
			const char* HistoryLine = GetHistory(CmdHistory::DOWN);

			if (HistoryLine)
			{
				if (console_debug) {
					X_LOG0("Cmd history", "%s", HistoryLine);
				}

				InputBuffer_ = HistoryLine;
				CursorPos_ = safe_static_cast<int32_t, size_t>(InputBuffer_.size());
			}
		}
		return true;
	}
	else if (event.keyId == KeyId::ESCAPE)
	{
		// rekt
		ClearInputBuffer();
	}

	return true;
}

const char* XConsole::GetHistory(CmdHistory::Enum direction)
{
	if (direction == CmdHistory::UP)
	{
		if (!CmdHistory_.empty())
		{
			if (HistoryPos_ < safe_static_cast<int32_t, size_t>(CmdHistory_.size() - 1))
			{
				HistoryPos_++;

				RefString_ = CmdHistory_[HistoryPos_];
				return RefString_.c_str();
			}
		}
	}
	else // down
	{
		// are we above base cmd?
		if (HistoryPos_ > 0)
		{
			HistoryPos_--;
			// adds a refrence to the string.
			RefString_ = CmdHistory_[HistoryPos_];
			return RefString_.c_str();
		}
	}

	return nullptr;
}


void XConsole::SaveCmdHistory(void) const
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	fileModeFlags mode;

	mode.Set(fileMode::WRITE);
	mode.Set(fileMode::RECREATE);
	mode.Set(fileMode::SHARE);

	XFileScoped file;
	if (file.openFile(CMD_HISTORY_FILE_NAME, mode))
	{
		ConsoleBuffer::const_reverse_iterator it = CmdHistory_.crbegin();
		for (; it != CmdHistory_.crend(); it++)
		{
			file.writeString(it->c_str(), safe_static_cast<uint32_t,size_t>(it->length()));
			file.write('\n');
		}
	}
}

void XConsole::LoadCmdHistory(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	fileModeFlags mode;

	mode.Set(fileMode::READ);
	mode.Set(fileMode::SHARE);

	XFileMemScoped file;
	if (file.openFile(CMD_HISTORY_FILE_NAME, mode))
	{
		const char* pBegin = file->getBufferStart();
		const char* pEnd = file->getBufferEnd();

		core::StringTokenizer<char> tokenizer(pBegin, pEnd, '\n');
		StringRange<char> range(nullptr, nullptr);

		while (tokenizer.ExtractToken(range))
		{
			if (range.GetLength() > 0) {
				AddCmdToHistory(core::string(range.GetStart(), range.GetEnd()));
			}
		}

		// limit the history.
		while (CmdHistory_.size() > MAX_HISTORY_ENTRIES) {
			CmdHistory_.pop_back();
		}
	}
}


void XConsole::AddCmdToHistory(const char* Command)
{
	// so we can scroll through past commands.

	HistoryPos_ = -1;

	if (!CmdHistory_.empty())
	{
		// make sure it's not same as last command 
		if (CmdHistory_.front() != Command) {
			CmdHistory_.push_front(core::string(Command));
		}
	}
	else
	{
		// 1st commnd :D
		CmdHistory_.push_front(core::string(Command));
	}

	// limit hte history.
	while (CmdHistory_.size() > MAX_HISTORY_ENTRIES) {
		CmdHistory_.pop_back();
	}

	if (console_save_history) {
		SaveCmdHistory();
	}
}

// Binds a cmd to a key
void XConsole::AddBind(const char* key, const char* cmd)
{
	// check for override ?
	const char* Old = FindBind(key);

	if (Old)
	{
		if (core::strUtil::IsEqual(Old, cmd))
		{
			// bind is same.
			return;
		}
		if (console_debug) {
			X_LOG1("Console", "Overriding bind \"%s\" -> %s with -> %s", key, Old, cmd);
		}

		ConsoleBindMapItor it = Binds_.find(X_CONST_STRING(key));
		it->second = cmd;
	}

	Binds_.insert(ConsoleBindMapItor::value_type(core::string(key), core::string(cmd)));
}

// returns the command for a given key
// returns null if no bind found
const char* XConsole::FindBind(const char* key)
{
	ConsoleBindMapItor it = Binds_.find(X_CONST_STRING(key));
	if (it != Binds_.end()) {
		return it->second.c_str();
	}
	return nullptr;
}

// removes all the binds.
void XConsole::ClearAllBinds(void)
{
	Binds_.clear();
}

void XConsole::Listbinds(IKeyBindDumpSink* CallBack)
{
	for (auto bind : Binds_)
	{
		CallBack->OnKeyBindFound(bind.first.c_str(), bind.second.c_str());
	}
}

ICVar* XConsole::RegisterString(const char* Name, const char* Value, 
	int Flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_ASSERT_NOT_NULL(Name);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	if (bitUtil::IsBitFlagSet(Flags, VarFlag::CPY_NAME))
	{
		Flags = bitUtil::ClearBitFlag(Flags, VarFlag::CPY_NAME);
		pCVar = X_NEW(CVarString<CVarBaseHeap>, &varArena_, 
			"CVarString<H>")(this, Name, Value, Flags, desc);
	}
	else
	{
		pCVar = X_NEW(CVarString<CVarBaseConst>, &varArena_, 
			"CVarString")(this, Name, Value, Flags, desc);
	}
	RegisterVar(pCVar, pChangeFunc);
	return pCVar;
}

ICVar* XConsole::RegisterInt(const char* Name, int Value, int Min, 
	int Max, int Flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_ASSERT_NOT_NULL(Name);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	pCVar = X_NEW(CVarInt<CVarBaseConst>, &varArena_, "CVarInt")(this, Name, Value, Min, Max, Flags, desc);
	RegisterVar(pCVar, pChangeFunc);
	return pCVar;
}

ICVar* XConsole::RegisterFloat(const char* Name, float Value, float Min,
	float Max, int Flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_ASSERT_NOT_NULL(Name);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	pCVar = X_NEW(CVarFloat<CVarBaseConst>, &varArena_, "CVarFloat")(this, Name, Value, Min, Max, Flags, desc);
	RegisterVar(pCVar, pChangeFunc);
	return pCVar;
}

ICVar* XConsole::ConfigRegisterString(const char* Name, const char* Value, int flags, 
	const char* desc)
{
	X_ASSERT_NOT_NULL(Name);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	pCVar = X_NEW(CVarString<CVarBaseHeap>, &varArena_, "CVarStringConfig")(this, Name, Value, flags, desc);
	RegisterVar(pCVar, nullptr);
	return pCVar;
}

ICVar* XConsole::ConfigRegisterInt(const char* Name, int Value, int Min, 
	int Max, int flags, const char* desc)
{
	X_ASSERT_NOT_NULL(Name);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	pCVar = X_NEW(CVarInt<CVarBaseHeap>, &varArena_, "CVarIntConfig")(this, Name, Value, Min, Max, flags, desc);
	RegisterVar(pCVar, nullptr);
	return pCVar;
}

ICVar* XConsole::ConfigRegisterFloat(const char* Name, float Value, float Min, 
	float Max, int flags, const char* desc)
{
	X_ASSERT_NOT_NULL(Name);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	pCVar = X_NEW(CVarFloat<CVarBaseHeap>, &varArena_, "CVarFloatConfig")(this, Name, Value, Min, Max, flags, desc);
	RegisterVar(pCVar, nullptr);
	return pCVar;
}


ICVar* XConsole::Register(const char* Name, float* src, float defaultvalue, 
	float Min, float Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_ASSERT_NOT_NULL(Name);
	X_ASSERT_NOT_NULL(src);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	*src = defaultvalue;

	pCVar = X_NEW(CVarFloatRef, &varArena_, "CVarRefFloat")(this, Name, src, Min, Max, flags, desc);
	RegisterVar(pCVar, pChangeFunc);
	return pCVar;
}

ICVar* XConsole::Register(const char* Name, int* src, int defaultvalue, 
	int Min, int Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_ASSERT_NOT_NULL(Name);
	X_ASSERT_NOT_NULL(src);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	*src = defaultvalue;

	pCVar = X_NEW(CVarIntRef, &varArena_, "CVarRefInt")(this, Name, src, Min, Max, flags, desc);
	RegisterVar(pCVar, pChangeFunc);
	return pCVar;
}

ICVar* XConsole::Register(const char* Name, Color* src, Color defaultvalue, 
	int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_ASSERT_NOT_NULL(Name);
	X_ASSERT_NOT_NULL(src);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	*src = defaultvalue;

	pCVar = X_NEW(CVarColRef, &varArena_, "CVarRefCol")(this, Name, src, flags, desc);
	RegisterVar(pCVar, pChangeFunc);
	return pCVar;
	
}

ICVar* XConsole::Register(const char* Name, Vec3f* src, Vec3f defaultvalue, 
	int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_ASSERT_NOT_NULL(Name);
	X_ASSERT_NOT_NULL(src);

	ICVar *pCVar = GetCVarForRegistration(Name);
	if (pCVar) {
		return pCVar;
	}

	*src = defaultvalue;

	pCVar = X_NEW(CVarVec3Ref, &varArena_, "CVarRefVec3")(this, Name, src, flags, desc);
	RegisterVar(pCVar, pChangeFunc);
	return pCVar;

}


ICVar* XConsole::GetCVar(const char* name)
{
	ConsoleVarMapItor it;

	it = VarMap_.find(name);
	if (it != VarMap_.end()) {
		return it->second;
	}
	return nullptr;
}

void XConsole::UnregisterVariable(const char* sVarName)
{
	ConsoleVarMapItor itor;
	itor = VarMap_.find(sVarName);

	if (itor == VarMap_.end()) {
		return;
	}

	ICVar *pCVar = itor->second;

	VarMap_.erase(sVarName);

	X_DELETE(pCVar, &varArena_);
}

// Commands :)

void XConsole::AddCommand(const char* Name, ConsoleCmdFunc func, int Flags, const char* desc)
{
	X_ASSERT_NOT_NULL(Name);

	ConsoleCommand cmd;

	cmd.Name = Name;
	cmd.Flags = Flags;
	cmd.pFunc = func;
	if (desc) {
		cmd.Desc = desc;
	}

	// pass cmd.Name instead of Name, saves creating a second core::string
	if (CmdMap_.find(cmd.Name) != CmdMap_.end())
	{
		X_WARNING("Console", "command already exsists: %s", Name);
	}

	CmdMap_.insert(std::make_pair(cmd.Name, cmd));
}


void XConsole::RemoveCommand(const char* Name)
{
	ConsoleCmdMapItor it = CmdMap_.find(X_CONST_STRING(Name));
	if (it != CmdMap_.end()) {
		CmdMap_.erase(it);
	}
}


void XConsole::Exec(const char* command, const bool DeferExecution)
{
	X_ASSERT_NOT_NULL(command);

	if (!DeferExecution)
	{
		ExecuteStringInternal(command, ExecSource::SYSTEM, false);
		return;
	}

	core::StackString<1024> temp(command);
	temp.trimLeft();

	// Unroll the exec command
	bool unroll = false; 

	if (temp.length() >= 4)
	{
		if (core::strUtil::FindCaseInsensitive(temp.begin(), temp.begin() + 4, "exec")) {
			unroll = true;
		}
	}

	if (unroll)
	{
		ExecuteStringInternal(temp.c_str(), ExecSource::SYSTEM, false);
	}
	else
	{
		deferredCmds_.push_back(DeferredCommand(core::string(temp.c_str()), false));
	}
}

bool XConsole::LoadConfig(const char* fileName)
{
	core::Path<char> path;

	path /= "config/";
	path /= fileName;
	path.setExtension(CONFIG_FILE_EXTENSION);

	core::XFileScoped file;
	size_t bytes;

	X_LOG0("Config", "Loading config: \"%s\"", fileName);

	if (file.openFile(path.c_str(), fileMode::READ))
	{
		bytes = file.remainingBytes();

		if (bytes > 0)
		{
			// 16 byte align it for faster strlen and find. (SSE)
			char* pData = X_NEW_ARRAY_ALIGNED(char, bytes + 2, g_coreArena, "ConfigFileData", 16);

			if (file.read(pData, bytes) == bytes)
			{
				// 2 bytes at end so the multiline search can be more simple.
				// and not have to worrie about reading out of bounds.
				pData[bytes] = '\0';
				pData[bytes + 1] = '\0';

				// execute all the data in the file.
				// it's parsed in memory.
				// remove comments here.
				char* begin = pData;
				char* end = pData + bytes;
				const char* pComment;

				// we support // and /* */ so loook for a '/'
				while ((pComment = core::strUtil::Find(begin, end, '/')) != nullptr)
				{
					// wee need atleast 1 more char.
					if (pComment >= (end - 1)) {
						break;
					}

					begin = const_cast<char*>(++pComment);

					if (*begin == '*')
					{
						begin[-1] = ' ';
						*begin++ = ' ';


						while (*begin != '*' && begin[1] != '/' && begin < end)
						{
							*begin++ = ' ';
						}
					}
					else if (*begin == '/')
					{
						// signle line.
						begin[-1] = ' ';
						*begin++ = ' ';

						while (*begin != '\n' && begin < end)
						{
							*begin++ = ' ';
						}
					}
					else
					{
						++begin;
					}
				}


				ConfigExec(pData);
			}

			X_DELETE_ARRAY(pData, g_coreArena);
		}
	}
	else
	{
		X_ERROR("Config", "failed to load: \"%s\"", path.c_str());
		return false;
	}
	return true;
}

// IXHotReload
bool XConsole::OnFileChange(const char* name)
{
#if X_ENABLE_CONFIG_HOT_RELOAD
	core::Path<char> temp(name);

	LoadConfig(temp.fileName());
#else
	X_UNUSED(name);
#endif // !X_ENABLE_CONFIG_HOT_RELOAD
	return true;
}
// ~IXHotReload

void XConsole::ConfigExec(const char* command)
{
	// if it's from config, should i limit what commands can be used?
	// for now i'll let any be used

	ExecuteStringInternal(command, ExecSource::CONFIG, false);
}

bool XConsole::CvarModifyBegin(ICVar* pCVar, ExecSource::Enum source)
{
	X_ASSERT_NOT_NULL(pCVar);

	ICVar::FlagType flags = pCVar->GetFlags();

	if (flags.IsSet(VarFlag::READONLY)) {
		X_ERROR("Console", "can't set value of read only cvar: %s", pCVar->GetName());
		return false;
	}

	if (source == ExecSource::CONFIG)
	{
		flags.Set(VarFlag::CONFIG);
		pCVar->SetFlags(flags);
	}

	return true;
}

void XConsole::ExecuteStringInternal(const char* pCommand, ExecSource::Enum source,
	const bool bSilentMode)
{
	X_ASSERT_NOT_NULL(pCommand);

	ConsoleCmdMapItor itrCmd;
	ConsoleVarMapItor itrVar;

	core::StackString512 name;
	core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS> value;
	core::StringRange<char> range(nullptr, nullptr);
	CommandParser parser(pCommand);
	const char* pPos;

	while (parser.extractCommand(range))
	{
		// work out name / value
		pPos = range.Find('=');

		if (pPos) {
			name.set(range.GetStart(), pPos);
		}
		else if ((pPos = range.Find(' ')) != nullptr)
		{
			name.set(range.GetStart(), pPos);
		}
		else
		{
			name.set(range.GetStart(), range.GetEnd());
		}

		name.trim();

		if (name.isEmpty())
			continue;

		// === Check if is a command ===
		itrCmd = CmdMap_.find(X_CONST_STRING(name.c_str()));
		if (itrCmd != CmdMap_.end())
		{
			value.set(range.GetStart(), range.GetEnd());
			ExecuteCommand((itrCmd->second), value);
			continue;
		}

		// === check for var ===
		itrVar = VarMap_.find(X_CONST_STRING(name.c_str()));
		if (itrVar != VarMap_.end())
		{
			ICVar* pCVar = itrVar->second;

			if (pPos) // is there a space || = symbol (meaning there is a possible value)
			{
				value.set(pPos + 1, range.GetEnd());
				value.trim();

				if (value.isEmpty())
				{
					// no value was given so assume they wanted
					// to print the value.
					DisplayVarInfo(pCVar);
				}
				else if (CvarModifyBegin(pCVar, source))
				{
					pCVar->Set(value.c_str());
					DisplayVarValue(pCVar);
				}
			}
			else
			{
				DisplayVarInfo(pCVar);
			}		

			continue;
		}

		if (!bSilentMode)
		{
			if (source == ExecSource::CONFIG) {
				X_WARNING("Config", "Unknown command/var: %s", name.c_str());
			}
			else {
				X_WARNING("Console", "Unknown command: %s", name.c_str());
			}
		}
	}

}

void XConsole::DisplayVarValue(ICVar* pVar)
{
	if (!pVar) {
		return;
	}

	X_LOG0("Dvar", "\"%s\" = %s", pVar->GetName(), pVar->GetString());
}

void XConsole::DisplayVarInfo(ICVar* pVar)
{
	if (!pVar) {
		return;
	}

	ICVar::FlagType::Description dsc;

	X_LOG0("Dvar", "\"%s\" = %s [%s]", pVar->GetName(), pVar->GetString(), 
		pVar->GetFlags().ToString(dsc));
}


void XConsole::ExecuteCommand(ConsoleCommand &cmd, 
	core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& str)
{
	str.replace('"', '\'');

	X_LOG_BULLET;

	if (cmd.Flags.IsSet(VarFlag::CHEAT))
	{
		X_WARNING("Console", "Cmd(%s) is cheat protected", cmd.Name.c_str());
		return;
	}

	if (cmd.pFunc)
	{
		// This is function command, execute it with a list of parameters.
		ConsoleCommandArgs cmdArgs(str);

		if (console_debug)
			X_LOG0("Console", "Running command \"%s\"", cmdArgs.GetArg(0));

		cmd.pFunc(&cmdArgs);
		return;
	}
}

/// ------------------------------------------------------


ICVar* XConsole::GetCVarForRegistration(const char* Name)
{
	ICVar* pCVar = nullptr;

	ConsoleVarMap::const_iterator it = VarMap_.find(X_CONST_STRING(Name));
	if (it != VarMap_.end()) {
		pCVar = it->second;
	}

	if (pCVar)
	{
		X_WARNING("Console", "var(%s) is already registerd", Name);
	}
	return pCVar;
}


void XConsole::RegisterVar(ICVar *pCVar, ConsoleVarFunc pChangeFunc)
{
	if (pChangeFunc) {
		pCVar->SetOnChangeCallback(pChangeFunc);
	}

	VarMap_.insert(ConsoleVarMapItor::value_type(pCVar->GetName(), pCVar));
}


void XConsole::ExecuteDeferredCommands()
{
	if (WaitSeconds_.GetValue())
	{
		if (WaitSeconds_ > gEnv->pTimer->GetFrameStartTime())
		{
			return;
		}

		WaitSeconds_.SetValue(0);
	}

	DeferredCmdList::iterator it;

	while (!deferredCmds_.empty())
	{
		it = deferredCmds_.begin();

		ExecuteStringInternal(it->command.c_str(), ExecSource::SYSTEM, it->silentMode);

		deferredCmds_.pop_front();
	}
}

void XConsole::OnFrameBegin(void)
{
	ExecuteDeferredCommands();

	if (!isVisable())
	{
		repeatEvent_.keyId = input::KeyId::UNKNOWN;
	}
	else if (repeatEvent_.keyId != input::KeyId::UNKNOWN)
	{
		repeatEventTimer_ -= gEnv->pTimer->GetFrameTime();

		if (repeatEventTimer_ <= 0.0f)
		{
		//	if (repeatEventTimer_ < -repeatEventInterval_)
		//	{
		//		repeatEvent_.keyId = input::KeyId::UNKNOWN;
		//	}
		//	else
			{

				if (repeatEvent_.action == input::InputState::CHAR) {
					OnInputEventChar(repeatEvent_);
				}
				else {
					ProcessInput(repeatEvent_);
				}

				repeatEventTimer_ = repeatEventInterval_;
			}
		}
	}
}


void XConsole::Draw()
{
	cursor_.curTime += gEnv->pTimer->GetFrameTime();

	if (cursor_.curTime > cursor_.displayTime)
	{
		cursor_.draw = !cursor_.draw; // toggle it
		cursor_.curTime = 0.f; // reset
	}

	DrawBuffer();
}

consoleState::Enum XConsole::getVisState(void) const
{
	return consoleState_;
}

size_t XConsole::MaxVisibleLogLines(void) const
{
	size_t height = pRender_->getHeight() - 40;

	size_t scaledLogHeight = static_cast<size_t>(
		static_cast<float>(CONSOLE_LOG_LINE_HIEGHT)* 0.8f);

	return height / scaledLogHeight;
}

void XConsole::DrawBuffer(void)
{
	if (this->consoleState_ == consoleState::CLOSED) {
		return;
	}

	font::XTextDrawConect ctx;
	ctx.SetColor(Col_Khaki);
	ctx.SetProportional(false);
	ctx.SetSize(Vec2f(20, static_cast<float>(CONSOLE_LOG_LINE_HIEGHT)));
	ctx.SetCharWidthScale(0.5f);
//	ctx.SetScaleFrom800x600(true);

	float width = pRender_->getWidthf() - 10;
	float height = pRender_->getHeightf() - 40;

	if (pRender_)
	{
		pRender_->Set2D(true);
		pRender_->DrawQuad(5, 5, width, 24, console_input_box_color);
		pRender_->DrawRect(5, 5, width, 24, console_input_box_color_border);

	//	pRender_->DrawQuad(5, 35, 790, 24, console_input_box_color);


		if (isExpanded()) {

			// draw a channel colum?
			if (console_output_draw_channel)
				pRender_->DrawQuad(5, 35, 11 * ctx.GetCharWidthScaled(), height, console_output_box_channel_color);

			pRender_->DrawQuad(5, 35, width, height, console_output_box_color);
			pRender_->DrawRect(5, 35, width, height, console_output_box_color_border);

			DrawScrollBar();
		}

		pRender_->Set2D(false);	
	}

	if (pFont_ && pRender_)
	{
		const char* txt = X_ENGINE_NAME " Engine " X_BUILD_STRING ">";

		Vec2f pos(10, 5);
		Vec2f txtwidth = pFont_->GetTextSize(txt, ctx);
		float fCharHeight = 0.8f * ctx.GetCharHeight();
		int	  CharHeight = static_cast<int>(fCharHeight);

		ctx.SetEffectId(pFont_->GetEffectId("drop"));
		
		pFont_->DrawString(pos, txt, ctx);
	
		ctx.SetDefaultEffect();
		ctx.SetColor(Col_White);

		pos.x += txtwidth.x;
		pos.x += 2;

		if (cursor_.draw)
		{
			core::StackString<256> temp(InputBuffer_.c_str(), InputBuffer_.c_str() + CursorPos_);
			float Lwidth = pFont_->GetTextSize(temp.c_str(), ctx).x + 3; // 3px offset from engine txt.

			pFont_->DrawString(Vec2f(pos.x + Lwidth, pos.y), "_", ctx);
		}

		size_t numDraw = 0;

		// the log.
		if (isExpanded())
		{
			ctx.SetColor(Col_White);
			ctx.SetSize(Vec2f(12,12));
			ctx.SetCharWidthScale(0.75f);

			ConsoleBufferRItor ritor;
			ritor = ConsoleLog_.rbegin();
			float xPos = 8;
			float yPos = height + 15; // 15 uints up
			int nScroll = 0;


			while (ritor != ConsoleLog_.rend() && yPos >= 30) // max 30 below top(not bottom)
			{
				if (nScroll >= ScrollPos_)
				{
					const char *buf = ritor->c_str();

					pFont_->DrawString(xPos, yPos, buf, ctx);
					yPos -= CharHeight;
					numDraw++;
				}
				nScroll++;
				++ritor;
			}
		}

		// draw the auto complete
		DrawInputTxt(pos);

	}

}

void XConsole::DrawScrollBar(void)
{
	if (!isExpanded()) {
		return;
	}

	if(pFont_ && pRender_)
	{
		// oooo shit nuggger wuggger.
		float width = pRender_->getWidthf() - 10;
		float height = pRender_->getHeightf() - 40;

		const float barWidth = 6;
		const float marging = 5;
		const float sliderHeight = 20;

		float start_x = (width + 5) - (barWidth + marging);
		float start_y = 35 + marging;
		float barHeight = height - (marging * 2);

		float slider_x = start_x;
		float slider_y = start_y;
		float slider_width = barWidth ;
		float slider_height = sliderHeight;

		// work out the possition of slider.
		// we take the height of the bar - slider and divide.
		size_t visibleNum = MaxVisibleLogLines();
		size_t scrollableLines = ConsoleLog_.size() - (visibleNum += 2);

		float positionPercent = PercentageOf(ScrollPos_, scrollableLines) * 0.01f;
		float offset = (barHeight - slider_height) * positionPercent;

		slider_y += (barHeight - slider_height - offset);
		if (slider_y < start_y) {
			slider_y = start_y;
		}

		pRender_->DrawQuad(start_x, start_y, barWidth, barHeight, console_output_scroll_bar_color);
		pRender_->DrawQuad(slider_x, slider_y, slider_width, slider_height, console_output_scroll_bar_slider_color);

	}
}

void XConsole::DrawInputTxt(const Vec2f& start)
{
	struct AutoResult
	{
		AutoResult() : name(nullptr), var(nullptr) {}
		AutoResult(const char* name, ICVar* var) : name(name), var(var) {}
		const char* name;
		ICVar* var;

		X_INLINE bool operator<(const AutoResult& oth) {
			return strcmp(name, oth.name) < 0;
		}
	};

	const size_t max_auto_complete_results = 32;
	typedef core::FixedArray<AutoResult, max_auto_complete_results> Results;
	Results results;

	Color txtCol = Col_White;
	Vec2f txtPos = start;

	if (pFont_ && pRender_)
	{
		const char* inputBegin = InputBuffer_.begin();
		const char* inputEnd = InputBuffer_.end();
		const char* Name;
		size_t NameLen, inputLen;


		if (InputBuffer_.isEmpty()) {
			ResetAutoCompletion();
			return;
		}

		// check for vreset
		if (const char* vreset = core::strUtil::Find(inputBegin, inputEnd, "vreset "))
		{
			// check if we have atleast 1 char.
			if (InputBuffer_.length() > 7) {
				inputBegin = core::Min(vreset + 7, inputEnd); // cap search.	
				txtCol = Col_Darkblue;
			}
		}

		// check for spaces.
		if (const char* space = core::strUtil::Find(inputBegin, inputEnd, ' '))
		{
		//	if (space == (inputEnd -1))
				inputEnd = space; // cap search. (-1 will be safe since must be 1 char in string)
		}


		inputLen = inputEnd - inputBegin;
		if (inputLen == 0) {
			return;
		}

		typedef core::traits::Function<bool(const char*, const char*,
			const char*, const char*)> MyComparisionFunc;

		MyComparisionFunc::Pointer pComparison = core::strUtil::IsEqual;
		if (!console_case_sensitive) {
			pComparison = core::strUtil::IsEqualCaseInsen;
		}

		// try find and cmd's / dvars that match the current input.
		ConsoleVarMapItor it = VarMap_.begin();

		for (; it != VarMap_.end(); ++it)
		{
			Name = it->second->GetName();
			NameLen = core::strUtil::strlen(Name);

			// if var name shorter than search leave it !
			if (NameLen < inputLen) {
				continue;
			}

			// we search same length.
			if (pComparison(Name, Name + inputLen, inputBegin, inputEnd))
			{
				results.push_back(AutoResult(Name, it->second));
			}

			if (results.size() == results.capacity()) {
				break;
			}
		}

		if (results.size() < results.capacity())
		{
			// do the commands baby!
			ConsoleCmdMapItor cmdIt = CmdMap_.begin();
			for (; cmdIt != CmdMap_.end(); ++cmdIt)
			{
				Name = cmdIt->second.Name.c_str();
				NameLen = cmdIt->second.Name.length();

				// if cmd name shorter than search leave it !
				if (NameLen < inputLen) {
					continue;
				}

				// we search same length.
				if (pComparison(Name, Name + inputLen, inputBegin, inputEnd))
				{
					results.push_back(AutoResult(Name, nullptr));
				}

				if (results.size() == results.capacity()) {
					break;
				}
			}
		}


		// sort them?
		std::sort(results.begin(), results.end());

		// Font contex
		font::XTextDrawConect ctx;
		ctx.SetProportional(false);
		ctx.SetSize(Vec2f(14, 14));
		ctx.SetCharWidthScale(0.65f);
	//	ctx.SetScaleFrom800x600(true);


		// Autocomplete
		if (autoCompleteNum_ != safe_static_cast<int, size_t>(results.size())) {
			autoCompleteIdx_ = -1;
		}

		autoCompleteNum_ = safe_static_cast<int,size_t>(results.size());
		autoCompleteIdx_ = core::Min(autoCompleteIdx_, autoCompleteNum_ - 1);
		autoCompleteSelect_ = autoCompleteIdx_ >= 0 ? autoCompleteSelect_ : false;

		if (autoCompleteSelect_) {
			if (InputBuffer_.find("vreset ")) {
				InputBuffer_ = "vreset ";
				InputBuffer_ += results[autoCompleteIdx_].name;
			} else {
				InputBuffer_ = results[autoCompleteIdx_].name;
			}
		//	if (results[autoCompleteIdx_].var) // for var only?
				InputBuffer_ += ' '; // add a space
			CursorPos_ = safe_static_cast<int32,size_t>(InputBuffer_.length());
			autoCompleteIdx_ = -1;
			autoCompleteSelect_ = false;
		}
		// ~AutoComplete

		if (!results.isEmpty())
		{
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

			if (isSingleCmd)
			{
				// if what they have entered fully matches a cmd,
				// change the txt color to darkblue.
				// we need to check if it's only a substring match currently.
				core::StackString<128> temp(inputBegin, inputEnd);
				const char* fullName = results[0].name;

				temp.trim();

				if (temp.isEqual(fullName)) {
					txtCol = Col_Darkblue;
				}
			}
			else if (results.size() > 1)
			{
				// if there is a space after the cmd's name.
				// we check if the input has a complete match
				// to any of the results.
				// if so only show that.
				string::const_str pos = InputBuffer_.find(' ');
				if (pos) // != string::npos) //== (InputBuffer_.length() - 1))
				{
					Results::const_iterator resIt;
					core::StackString<128> temp(InputBuffer_.begin(), pos);

					resIt = results.begin();
					for (; resIt != results.end(); ++resIt)
					{
						if (core::strUtil::IsEqual(temp.c_str(), resIt->name))
						{
							// ok remove / add.
							AutoResult res = *resIt;

							results.clear();
							results.append(res);
							goto resultsChanged;
						}
					}
				}
			}

			if (isSingleVar)
			{
				core::StackString<128> nameStr;
				core::StackString<32>  defaultStr;
				core::StackString<128> value;		// split the value and name for easy alignment.
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

				value.appendFmt("%s", pCvar->GetString());
				defaultValue.appendFmt("%s", pCvar->GetDefaultStr());

				description.append(pCvar->GetDesc()); // dose string length for us / caps.

				if (pCvar->GetType() == VarFlag::INT)
				{
					domain.appendFmt("Domain is any interger between: %d and %d",
						static_cast<int>(pCvar->GetMin()),
						static_cast<int>(pCvar->GetMax()));
				}
				else if (pCvar->GetType() == VarFlag::FLOAT)
				{
					domain.appendFmt("Domain is any real number between: %g and %g",
						pCvar->GetMin(),
						pCvar->GetMax());
				}
				else if (pCvar->GetType() == VarFlag::COLOR)
				{
					domain.appendFmt("Domain is 1 or 4 real numbers between: 0.0 and 1.0");
				}

				height = fCharHeight * 2.5f;
				width = core::Max(
					width,
					pFont_->GetTextSize(nameStr.c_str(), ctx).x,
					pFont_->GetTextSize(defaultStr.c_str(), ctx).x
					);

				width += nameValueSpacing; // name - value spacing.
				xposValueOffset = width;
				width += core::Max(
					pFont_->GetTextSize(value.c_str(), ctx).x,
					pFont_->GetTextSize(defaultValue.c_str(), ctx).x
					);

				float box2Offset = 5;
				float height2 = fCharHeight * (domain.isEmpty() ? 1.5f : 2.5f);
				float width2 = core::Max(
					width,
					pFont_->GetTextSize(description.c_str(), ctx).x,
					pFont_->GetTextSize(domain.c_str(), ctx).x
					);


				width += 15; // add a few pixels.
				width2 += 15; // add a few pixels.

				if (isColorVar) {
					width += colorBoxWidth + colorBoxPadding;
				}

				// Draw the boxes
				pRender_->Set2D(true);
				pRender_->DrawQuad(xpos, ypos, width, height, col);
				pRender_->DrawRect(xpos, ypos, width, height, console_input_box_color_border);
				pRender_->DrawQuad(xpos, ypos + height + box2Offset, width2, height2, col);
				pRender_->DrawRect(xpos, ypos + height + box2Offset, width2, height2, console_input_box_color_border);

				if (isColorVar)
				{
					float colxpos = xpos + width - (colorBoxPadding + colorBoxWidth);
					float colypos = ypos + (colorBoxPadding * 2.f);
					float colHeight = fCharHeight - colorBoxPadding * 2;

					// draw the colors :D !
					CVarColRef* PColVar = static_cast<CVarColRef*>(pCvar);

					pRender_->DrawQuad(colxpos, colypos, colorBoxWidth, colHeight, PColVar->GetColor());
					pRender_->DrawRect(colxpos, colypos, colorBoxWidth, colHeight, Col_Black);

					// 2nd box.
					colypos += fCharHeight;

					pRender_->DrawQuad(colxpos, colypos, colorBoxWidth, colHeight, PColVar->GetDefaultColor());
					pRender_->DrawRect(colxpos, colypos, colorBoxWidth, colHeight, Col_Black);


					// How about a preview of the new color?
					string::const_str pos = InputBuffer_.find(nameStr.c_str());
					if (pos)
					{
						static core::StackString<64> lastValue;
						core::StackString<64> colorStr(&pos[nameStr.length()],
							InputBuffer_.end());
						
						colorStr.trim();

						// save a lex if string is the same.
						// slap myself, then it's not drawn lol.
						// add some checks if still needs drawing if the lex time is a issue.
						if (!colorStr.isEmpty()) // && colorStr != lastValue)
						{
							lastValue = colorStr;

							// parse it.
							Color previewCol;
							if (CVarColRef::ColorFromString(colorStr.c_str(), previewCol))
							{
								// draw a box on the end cus i'm a goat.
								pRender_->DrawQuad(xpos + width + 5, ypos, height, height, previewCol);
								pRender_->DrawRect(xpos + width + 5, ypos, height, height, Col_Black);
							}
						}
					}
				}
				pRender_->Set2D(false);

				// draw da text baby!
				xpos += 5;

				ctx.SetColor(Col_Whitesmoke);
				pFont_->DrawString(xpos, ypos, nameStr.c_str(), ctx);
				pFont_->DrawString(xpos + xposValueOffset, ypos, value.c_str(), ctx);

				ctx.SetColor(Col_Darkgray);
				ypos += fCharHeight;
				pFont_->DrawString(xpos, ypos, defaultStr.c_str(), ctx);
				pFont_->DrawString(xpos + xposValueOffset, ypos, defaultValue.c_str(), ctx);

				ypos += fCharHeight * 1.5f;
				ypos += box2Offset;

				ctx.SetColor(Col_Darkgray);
				pFont_->DrawString(xpos, ypos, description.c_str(), ctx);

				if (!domain.isEmpty())
				{
					ypos += fCharHeight;
					pFont_->DrawString(xpos, ypos, domain.c_str(), ctx);
				}
			}
			else
			{
				Results::const_iterator resIt;

				resIt = results.begin();
				for (; resIt != results.end(); ++resIt)
				{
					width = core::Max(width, pFont_->GetTextSize(resIt->name, ctx).x);
					height += fCharHeight;
				}

				width += 10; // add a few pixels.
		//		height += 5; // add a few pixels.

				pRender_->Set2D(true);
				pRender_->DrawQuadImage(xpos, ypos, width, height, this->pBackground_->getTexID(), col);
				pRender_->DrawRect(xpos, ypos, width, height, console_output_box_color_border);
				pRender_->Set2D(false);

				xpos += 5;

				resIt = results.begin();
				
				for (int idx = 0; resIt != results.end(); ++resIt, idx++)
				{
					if (autoCompleteIdx_ >= 0 && autoCompleteIdx_ == idx)
						ctx.SetColor(Col_Darkturquoise);
					else if (resIt->var)
						ctx.SetColor(Col_Whitesmoke);
					else
						ctx.SetColor(Col_Darkblue);

					pFont_->DrawString(xpos, ypos, resIt->name, ctx);

					ypos += fCharHeight;
				}

			}
		}

		// the input
		if (!InputBuffer_.isEmpty())
		{
			ctx.SetSize(Vec2f(20, 20));
			ctx.SetCharWidthScale(0.5f);
			ctx.SetColor(txtCol);

			core::string::const_str space = InputBuffer_.find(' ');
			if (space)
			{
				core::StackString<64> temp(InputBuffer_.begin(), space);
#if 1
				// preserve any colors.
				core::StackString<128> temp2;
				
				if(const char* pCol = temp.find('^')) {
					if(core::strUtil::IsDigit(pCol[1])) {
						temp2.appendFmt("^%c", pCol[1]);
					}
				}

			//	temp2.append(&InputBuffer_[space]);
				temp2.append(space);


				pFont_->DrawString(txtPos, temp.c_str(), ctx);
				ctx.SetColor(Col_White);
				txtPos.x += pFont_->GetTextSize(temp.c_str(), ctx).x;
				pFont_->DrawString(txtPos, temp2.c_str(), ctx);
#else
				pFont_->DrawString(txtPos, temp.c_str(), ctx);
				ctx.SetColor(Col_White);
				txtPos.x += pFont_->GetTextSize(temp.c_str(), ctx).x;

				pFont_->DrawString(txtPos, &InputBuffer_[space], ctx);
#endif
			}
			else
			{
				pFont_->DrawString(txtPos, InputBuffer_.c_str(), ctx);
			}
		}
	}
}


void XConsole::ResetAutoCompletion(void)
{
	autoCompleteIdx_ = -1;
	autoCompleteNum_ = 0;
	autoCompleteSelect_ = false;
}

void XConsole::addLineToLog(const char* pStr, uint32_t length)
{
	X_UNUSED(length);
	ConsoleLog_.push_back(core::string(pStr));

	int bufferSize = console_buffer_size;

	if (safe_static_cast<int,size_t>(ConsoleLog_.size()) > bufferSize)
	{
		ConsoleLog_.pop_front();

		size_t noneScroll = MaxVisibleLogLines();

		// move scroll wheel with the moving items?
		if (ScrollPos_ > 0 && ScrollPos_ < safe_static_cast<int32_t,size_t>(ConsoleLog_.size() - noneScroll)) {
			ScrollPos_++;
		}
	}
	else
	{
		if (ScrollPos_ > 0) {
			ScrollPos_++;
		}
	}
}

int XConsole::getLineCount(void) const
{
	return safe_static_cast<int, size_t>(ConsoleLog_.size());
}

///////////////////////////////////////////////////////////

void XConsole::ListCommands(const char* searchPatten)
{
	ConsoleCmdMapItor itrCmd, itrCmdEnd = CmdMap_.end();
	ConsoleCommand::FlagType::Description dsc;

	core::Array<ConsoleCommand*> sorted_cmds(g_coreArena);
	sorted_cmds.setGranularity(CmdMap_.size());

	for (itrCmd = CmdMap_.begin(); itrCmd != itrCmdEnd; ++itrCmd)
	{
		ConsoleCommand &cmd = itrCmd->second;

		if (!searchPatten || strUtil::WildCompare(searchPatten, cmd.Name))
		{
			sorted_cmds.append(&cmd);
		}
	}

	sortCmdsByName(sorted_cmds);

	X_LOG0("Console", "------------ ^8Commands(%i)^7 ------------", sorted_cmds.size());

	core::Array<ConsoleCommand*>::ConstIterator it = sorted_cmds.begin();
	for (; it != sorted_cmds.end(); ++it)
	{
		const ConsoleCommand* cmd = *it;
		X_LOG0("Command", "^2\"%s\"^7 [^1%s^7] Desc: \"%s\"", cmd->Name.c_str(), 
			cmd->Flags.ToString(dsc), cmd->Desc.c_str());
	}

	X_LOG0("Console", "------------ ^8Commands End^7 ------------");
}



void XConsole::ListVariables(const char* searchPatten)
{
	ConsoleVarMapItor itrVar, itrVarEnd = VarMap_.end();
	ICVar::FlagType::Description dsc;

	// i'm not storing the vars in a ordered map since it's slow to get them.
	// and i only need order when priting them.
	// so it's not biggy doing the sorting here.
	core::Array<ICVar*> sorted_vars(g_coreArena);
	sorted_vars.setGranularity(VarMap_.size());

	for (itrVar = VarMap_.begin(); itrVar != itrVarEnd; ++itrVar)
	{
		ICVar* var = itrVar->second;

		if (!searchPatten || strUtil::WildCompare(searchPatten, var->GetName()))
		{
			sorted_vars.push_back(var);
		}
	}

	sortVarsByName(sorted_vars);

	X_LOG0("Console", "-------------- ^8Vars(%i)^7 --------------", sorted_vars.size());

	core::Array<ICVar*>::ConstIterator it = sorted_vars.begin();
	for (; it != sorted_vars.end(); ++it)
	{
		const ICVar* var = *it;
		X_LOG0("Dvar", "^2\"%s\"^7 [^1%s^7] Desc: \"%s\"", var->GetName(), var->GetFlags().ToString(dsc), var->GetDesc());
	}


	X_LOG0("Console", "-------------- ^8Vars End^7 --------------");
}


void XConsole::Copy(void)
{
	core::clipboard::setText(InputBuffer_.c_str());
}

void XConsole::Paste(void)
{
	const char* txt = core::clipboard::getText();

	if (txt)
	{
		// insert it at current pos.
		InputBuffer_.insert(CursorPos_, txt);
		// add to length
		CursorPos_ += safe_static_cast<int32_t, size_t>(core::strUtil::strlen(txt));
	}
	else
	{
		X_LOG1("Console", "Failed to paste text to console");
	}
}

// ==================================================================
// ==================================================================



XConsoleNULL::XConsoleNULL()
{

}

XConsoleNULL::~XConsoleNULL()
{

}

void XConsoleNULL::Startup(ICore* pCore)
{
	X_UNUSED(pCore);

}

void XConsoleNULL::ShutDown(void)
{

}

void XConsoleNULL::SaveChangedVars(void)
{

}

void XConsoleNULL::unregisterInputListener(void)
{

}

void XConsoleNULL::freeRenderResources(void)
{

}

void XConsoleNULL::Draw(void)
{

}

consoleState::Enum XConsoleNULL::getVisState(void) const
{
	return consoleState::CLOSED;
}

ICVar* XConsoleNULL::RegisterString(const char* Name, const char* Value, int Flags, 
	const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Flags);
	X_UNUSED(desc);
	X_UNUSED(pChangeFunc);
	return nullptr;
}

ICVar* XConsoleNULL::RegisterInt(const char* Name, int Value, int Min, int Max, 
	int Flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(Flags);
	X_UNUSED(desc);
	X_UNUSED(pChangeFunc);
	return nullptr;
}

ICVar* XConsoleNULL::RegisterFloat(const char* Name, float Value, float Min, float Max,
	int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);
	X_UNUSED(pChangeFunc);
	return nullptr;
}


ICVar* XConsoleNULL::ConfigRegisterString(const char* Name, const char* Value, int flags,
	const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(flags);
	X_UNUSED(desc);
	return nullptr;
}

ICVar* XConsoleNULL::ConfigRegisterInt(const char* Name, int Value, int Min, int Max, 
	int flags, const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);
	return nullptr;
}

ICVar* XConsoleNULL::ConfigRegisterFloat(const char* Name, float Value, float Min, 
	float Max, int flags, const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);
	return nullptr;
}



	// refrenced based, these are useful if we want to use the value alot so we just register it's address.
ICVar* XConsoleNULL::Register(const char* name, float* src, float defaultvalue, 
	float Min, float Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);
	X_UNUSED(pChangeFunc);

	return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, int* src, int defaultvalue, 
	int Min, int Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);
	X_UNUSED(pChangeFunc);

	return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, Color* src, Color defaultvalue, 
	int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(flags);
	X_UNUSED(desc);
	X_UNUSED(pChangeFunc);

	return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, Vec3f* src, Vec3f defaultvalue, 
	int flags, const char* desc, ConsoleVarFunc pChangeFunc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(flags);
	X_UNUSED(desc);
	X_UNUSED(pChangeFunc);

	return nullptr;
}

ICVar* XConsoleNULL::GetCVar(const char* name)
{
	X_UNUSED(name);

	return nullptr;
}


void XConsoleNULL::UnregisterVariable(const char* sVarName)
{
	X_UNUSED(sVarName);

}


void XConsoleNULL::AddCommand(const char* Name, ConsoleCmdFunc func, int Flags,
	const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(func);
	X_UNUSED(Flags);
	X_UNUSED(desc);

}


void XConsoleNULL::RemoveCommand(const char* Name)
{
	X_UNUSED(Name);

}


void XConsoleNULL::Exec(const char* command, const bool DeferExecution)
{
	X_UNUSED(command);
	X_UNUSED(DeferExecution);

}


bool XConsoleNULL::LoadConfig(const char* fileName)
{
	X_UNUSED(fileName);
	return true;
}

/*
void XConsoleNULL::ConfigExec(const char* command)
{
	X_UNUSED(command);

}*/


void XConsoleNULL::OnFrameBegin()
{

}


// Loggging
void XConsoleNULL::addLineToLog(const char* pStr, uint32_t length)
{
	X_UNUSED(pStr);
	X_UNUSED(length);
}

int XConsoleNULL::getLineCount(void) const
{
	return 0;
}
// ~Loggging


X_NAMESPACE_END