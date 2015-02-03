#include "stdafx.h"
#include "Gui.h"

#include <String\Lexer.h>
#include <String\XParser.h>

#include <IFileSys.h>

#include "XWindow.h"


X_NAMESPACE_BEGIN(gui)

using namespace input;

XGui::XGui() : 
	pDesktop_(nullptr)
{

}

XGui::~XGui()
{

}

void XGui::Redraw()
{
	if (isDeskTopValid())
		pDesktop_->reDraw();
}

void XGui::DrawCursor(void)
{


}

const char* XGui::Activate(bool activate, int time)
{


	return nullptr;
}

bool XGui::OnInputEvent(const input::InputEvent& event)
{
	if (event.deviceId == InputDevice::MOUSE)
	{
		if (event.action == InputState::CHANGED)
		{
			if (event.keyId == KeyId::MOUSE_X)
				cursorPos_.x += event.value;
			else if (event.keyId == KeyId::MOUSE_X)
				cursorPos_.y += event.value;
		}
	}

	if (isDeskTopValid())
	{
		pDesktop_->OnInputEvent(event);
	}
	return false;
}

bool XGui::OnInputEventChar(const input::InputEvent& event)
{
	if (isDeskTopValid())
	{
		pDesktop_->OnInputEventChar(event);
	}
	return false;
}

// -------------------------------------------

bool XGui::InitFromFile(const char* name)
{
	core::Path path, pathBinary;
	core::XFileMemScoped file;
	core::XFileScoped fileBinary;
	core::fileModeFlags mode;

	// TODO remove extension.?
	this->name_ = name;

	mode.Set(core::fileMode::READ);


	// already init?
	if (isDeskTopValid())
	{
		// we are reloading most likley, warn if diffrent.
		if (name_ != name)
		{
			X_WARNING("Gui", "gui item is already init for menu: \"%s\"",
				name_.c_str());
		}

		// just delete the last one?
		// would be cool if like state was preserved across a reload.
		// so colors and sizes could be changed, and moving items still
		// be on same path.
		X_DELETE_AND_NULL(pDesktop_, g_3dEngineArena);

		pDesktop_ = X_NEW(XWindow, g_3dEngineArena, "MenuWindow")(this);
		pDesktop_->setFlag(WindowFlag::DESKTOP);
		// for reloading do we want to ignore compiled ones?
		// i think so.
		goto SourceLoad;
	}

	pDesktop_ = X_NEW(XWindow, g_3dEngineArena, "MenuWindow")(this);
	pDesktop_->setFlag(WindowFlag::DESKTOP);

	pathBinary = "gui/compiled/";
	pathBinary.setFileName(name);
	pathBinary.setExtension(GUI_BINARY_FILE_EXTENSION);

	// none binary
	path = "gui/";
	path.setFileName(name);
	path.setExtension(GUI_FILE_EXTENSION);

	// first we check if a binary file exsists.
	FileHdr hdr;

	if(gEnv->pFileSys->fileExists(pathBinary.c_str()))
	{
		uint32_t sourceCrc32;
		core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();

		// does a source version even exist?
		if(gEnv->pFileSys->fileExists(path))
		{
			// TODO: check last modified.


			// read the binary header.
			if(!fileBinary.openFile(pathBinary.c_str(),mode))
			{
				X_ERROR("Gui", "Failed to open the compiled gui file, trying source.");
				goto SourceLoad;
			}

			if(!fileBinary.readObj(hdr))
			{
				X_ERROR("Gui", "failed to read compiled gui header, trying source.");
				goto SourceLoad;
			}

			if(!hdr.isValid())
			{
				X_ERROR("Gui", "compiled gui file header is corrupt, trying source");
				goto SourceLoad;
			}

			// check the crc32 of the source.
			if(file.openFile(path.c_str(), mode))
			{
				sourceCrc32 = pCrc32->GetCRC32(file.getBufferStart(), file.getSize());

				if(hdr.crc32 != sourceCrc32)
				{
					goto SourceLoad;
				}
			}
			else
			{
				X_ERROR("Gui", "Failed to read source version, trying binary.");
				// try load binary?
			}
			
			// we load the binary version if we are here.
			return ParseBinaryFile(hdr, fileBinary);
		}

	}

SourceLoad:

	if(!file.IsOpen())
	{
		if (file.openFile(path.c_str(), mode))
		{
			X_DELETE_AND_NULL(pDesktop_, g_3dEngineArena);
			X_ERROR("Gui", "failed to open gui file: \"%s\"", path.c_str());
			return false;
		}
	}

	X_LOG0("Gui", "parsing: \"%s\"", path.c_str());
	return ParseTextFile(file->getBufferStart(), file->getBufferEnd());
}


bool XGui::ParseBinaryFile(const FileHdr& hdr, core::XFileScoped& file)
{
	// if we have no windows in the gui file we still class it as a valid file.
	if(hdr.numWindows > 0)
	{
		return pDesktop_->Parse(hdr, file->GetFile());
	}
	return true;
}

bool XGui::ParseTextFile(const char* begin, const char* end)
{
	core::XLexer::LexFlags flags;
	flags.Set(core::LexFlag::NOFATALERRORS);
	flags.Set(core::LexFlag::NOSTRINGCONCAT);
	flags.Set(core::LexFlag::ALLOWMULTICHARLITERALS);
	flags.Set(core::LexFlag::ALLOWBACKSLASHSTRINGCONCAT);

	core::XParser lex(begin,end, "", flags, g_3dEngineArena);

	lex.setFlags(flags);

	// we have a window def first.
	if (lex.ExpectTokenString("windowDef"))
	{
		if (pDesktop_->Parse(lex))
		{
			pDesktop_->FixUpParms();
			return true;
		}
	}

	return false;
}

X_NAMESPACE_END