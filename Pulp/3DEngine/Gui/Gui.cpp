#include "stdafx.h"
#include "Gui.h"

#include <String\Lexer.h>
#include <String\XParser.h>

#include <IFileSys.h>
#include <IRender.h>
#include <IPrimativeContext.h>
#include <IFont.h>

#include "GuiManger.h"

#include <Hashing\crc32.h>

#include "XWindow.h"


X_NAMESPACE_BEGIN(engine)

using namespace input;

namespace gui
{

	XGui::XGui(XGuiManager& guiMan) :
		guiMan_(guiMan),
		pDesktop_(nullptr),
		sourceCrc32_(0)
	{

	}

	XGui::~XGui()
	{

	}

	void XGui::Redraw(engine::IPrimativeContext* pDrawCon)
	{
		if (isDeskTopValid()) {
			pDesktop_->reDraw(pDrawCon);
		}
	}

	void XGui::DrawCursor(engine::IPrimativeContext* pDrawCon)
	{
		// windows call this function when they want the cursor draw.
		// the position of the mouse is: cursorPos_
		// GuiManger has the texture object.
		// so that multiple gui's can share the same pointer.
		// or maybe Gui should own it and it's just ref counted.

		auto* pCursorArrow = guiMan_.GetCursor();
		auto* pRender = gEnv->pRender;

		core::StackString<64> posStr;
		posStr.appendFmt("Pos: %g x %g", cursorPos_.x, cursorPos_.y);

		font::TextDrawContext ctx;
		ctx.col = Col_Red;

		pDrawCon->drawText(Vec3f(300, 10, 1), ctx, posStr.begin(), posStr.end());

		Vec2f rect;
		rect = pRender->getDisplayRes();

		const float width = rect.x;
		const float height = rect.y;


		pDrawCon->drawQuadSS(
			cursorPos_.x / width, cursorPos_.y / height,
			0.1f, 0.1f,
			pCursorArrow,
			Col_White
		);
	}

	const char* XGui::Activate(bool activate, int time)
	{
		X_UNUSED(activate);
		X_UNUSED(time);


		return nullptr;
	}

	bool XGui::OnInputEvent(const input::InputEvent& event)
	{
		if (event.deviceType == InputDeviceType::MOUSE)
		{
			if (event.action == InputState::CHANGED)
			{
				if (event.keyId == KeyId::MOUSE_X) {
					cursorPos_.x += event.value;
				}
				else if (event.keyId == KeyId::MOUSE_Y) {
					cursorPos_.y += event.value;
				}
			}
		}

		if (isDeskTopValid()) {
			pDesktop_->OnInputEvent(event);
		}

		return false;
	}

	bool XGui::OnInputEventChar(const input::InputEvent& event)
	{
		if (isDeskTopValid()) {
			pDesktop_->OnInputEventChar(event);
		}

		return false;
	}

	// -------------------------------------------

	bool XGui::InitFromFile(const char* name)
	{
		core::Path<char> path, pathBinary;
		core::XFileMemScoped file;
		core::XFileScoped fileBinary;
		core::fileModeFlags mode;
		FileHdr hdr;

		// TODO remove extension.?
		this->name_ = name;

		mode.Set(core::fileMode::READ);

		core::Crc32* pCrc32 = gEnv->pCore->GetCrc32();

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

		pathBinary = "gui\\compiled\\";
		pathBinary.setFileName(name);
		pathBinary.setExtension(GUI_BINARY_FILE_EXTENSION);

		// none binary
		path = "gui\\";
		path.setFileName(name);
		path.setExtension(GUI_FILE_EXTENSION);

		// first we check if a binary file exsists.
		if (gEnv->pFileSys->fileExists(pathBinary.c_str()))
		{
			// does a source version even exist?
			if (gEnv->pFileSys->fileExists(path.c_str()))
			{
				// TODO: check last modified.


				// read the binary header.
				if (!fileBinary.openFile(pathBinary.c_str(), mode))
				{
					X_ERROR("Gui", "Failed to open the compiled gui file, trying source.");
					goto SourceLoad;
				}

				if (!fileBinary.readObj(hdr))
				{
					X_ERROR("Gui", "failed to read compiled gui header, trying source.");
					goto SourceLoad;
				}

				if (!hdr.IsValid())
				{
					X_ERROR("Gui", "compiled gui file header is corrupt, trying source");
					goto SourceLoad;
				}

				// check the crc32 of the source.
				if (file.openFile(path.c_str(), mode))
				{
					sourceCrc32_ = pCrc32->GetCRC32(file->getBufferStart(),
						safe_static_cast<size_t, uint64_t>(file->getSize()));

					if (hdr.crc32 != sourceCrc32_)
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
				return ParseBinaryFile(hdr, fileBinary.GetFile());
			}

		}

	SourceLoad:
		fileBinary.close();

		if (!file.IsOpen())
		{
			if (!file.openFile(path.c_str(), mode))
			{
				X_DELETE_AND_NULL(pDesktop_, g_3dEngineArena);
				X_ERROR("Gui", "failed to open gui file: \"%s\"", path.c_str());
				return false;
			}
		}

		// make sure we have a crc
		if (sourceCrc32_ == 0)
		{
			sourceCrc32_ = pCrc32->GetCRC32(file->getBufferStart(),
				safe_static_cast<size_t, uint64_t>(file->getSize()));
		}

		X_LOG0("Gui", "parsing: \"%s\"", path.c_str());
		return ParseTextFile(file->getBufferStart(), file->getBufferEnd());
	}


	bool XGui::ParseBinaryFile(const FileHdr& hdr, core::XFile* pFile)
	{
		// should i just load the whole file and provides a meory cursor?
		// i think i should just make it a memory file so that i can read form 
		// that and it's provided th\t same functionatlity.
		X_UNUSED(hdr);

		return pDesktop_->Parse(pFile);
	}

	bool XGui::ParseTextFile(const char* begin, const char* end)
	{
		core::XLexer::LexFlags flags;
		flags.Set(core::LexFlag::NOFATALERRORS);
		flags.Set(core::LexFlag::NOSTRINGCONCAT);
		flags.Set(core::LexFlag::ALLOWMULTICHARLITERALS);
		flags.Set(core::LexFlag::ALLOWBACKSLASHSTRINGCONCAT);

		core::XParser lex(begin, end, "", flags, g_3dEngineArena);

		lex.setFlags(flags);

		// we have a window def first.
		if (lex.ExpectTokenString("windowDef"))
		{
			if (pDesktop_->Parse(lex))
			{
				pDesktop_->FixUpParms();
				//		SaveBinaryVersion();
				return true;
			}
		}

		return false;
	}

	bool XGui::SaveBinaryVersion(void)
	{
		core::Path<char> path;
		core::XFileScoped file;
		core::fileModeFlags mode;
		FileHdr hdr;

		mode.Set(core::fileMode::WRITE);
		mode.Set(core::fileMode::RECREATE);
		mode.Set(core::fileMode::RANDOM_ACCESS);

		path = "gui\\compiled\\";
		path.setFileName(getName());
		path.setExtension(GUI_BINARY_FILE_EXTENSION);

#if 1
		if (!gEnv->pFileSys->createDirectoryTree(path.c_str()))
		{
			X_ERROR("Gui", "failed to create directory for saving binary version: %s", path.c_str());
			return false;
		}
#endif

		if (file.openFile(path.c_str(), mode))
		{
			hdr.Magic = GUI_BINARY_MAGIC;
			hdr.version = GUI_BINARY_VERSION;
			hdr.crc32 = sourceCrc32_;
			hdr.fileSize = 0; // set after

			file.writeObj(hdr);

			// seralise all the chickens.
			if (!pDesktop_->WriteToFile(file.GetFile()))
			{
				X_ERROR("Gui", "failed to save binary vesion of the following gui: %s", getName());
				return false;
			}

			hdr.fileSize = safe_static_cast<uint32_t, uint64_t>(file.tell());

			file.seek(0, core::SeekMode::SET);

			return file.writeObj(hdr) > 0;
		}
		return false;
	}

} // namespace

X_NAMESPACE_END