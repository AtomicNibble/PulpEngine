#include "stdafx.h"
#include "MayaUtil.h"

#include <maya\MGlobal.h>
#include <maya/MProgressWindow.h>


namespace MayaUtil
{
	namespace
	{
		bool g_StartOfBlock = false; // used for logging formating.
		bool s_progressActive = false;

		MString s_progressCntl;
	}

	void SetStartOfBlock(bool start)
	{
		g_StartOfBlock = start;
	}

	void MayaPrintError(const char *fmt, ...)
	{
		va_list	argptr;
		char	msg[2048];

		va_start(argptr, fmt);
		vsnprintf_s(msg, sizeof(msg), fmt, argptr);
		va_end(argptr);


		if (g_StartOfBlock) {
			std::cout << "\n";
			g_StartOfBlock = false;
		}

		std::cerr << "Error: " << msg << std::endl;
		MGlobal::displayError(msg);
	}

	void MayaPrintWarning(const char *fmt, ...)
	{
		va_list	argptr;
		char	msg[2048];

		va_start(argptr, fmt);
		vsnprintf_s(msg, sizeof(msg), fmt, argptr);
		va_end(argptr);

		if (g_StartOfBlock) {
			std::cout << "\n";
			g_StartOfBlock = false;
		}

		std::cerr << "Warning: " << msg << std::endl;
		MGlobal::displayWarning(msg);
	}

	void MayaPrintMsg(const char *fmt, ...)
	{
		va_list	argptr;
		char	msg[2048];

		va_start(argptr, fmt);
		vsnprintf_s(msg, sizeof(msg), fmt, argptr);
		va_end(argptr);

		if (g_StartOfBlock) {
			std::cout << "\n";
			g_StartOfBlock = false;
		}

		std::cout << msg << std::endl;
	}


	void SetProgressCtrl(const MString& str)
	{
		s_progressCntl = str;
	}

	MStatus ShowProgressDlg(void)
	{
		using namespace std;
		if (s_progressCntl.length() > 0) {
			MGlobal::executeCommand("progressBar -e -pr 0 " + s_progressCntl);
		}
		else if (!s_progressActive) {
			MString title = X_ENGINE_NAME" Engine - Saving Model";
			MString process = "Starting...                                            ";

			int amount = 0;
			int maxProgress = 8;

			if (!MProgressWindow::reserve()) {
				MGlobal::displayError("Progress window already in use.");
				return MS::kFailure;
			}

			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgressRange(amount, maxProgress))
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setTitle(title));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setInterruptable(false));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgress(amount));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgressStatus(process));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::startProgress());

			CHECK_MSTATUS(MProgressWindow::advanceProgress(1));


			s_progressActive = true;
		}
		return MS::kSuccess;
	}


	MStatus HideProgressDlg(void)
	{
		using namespace std;
		if (s_progressActive) {
			s_progressActive = false;
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::endProgress());
		}

		return MS::kSuccess;
	}

	void SetProgressText(const MString& str)
	{
		MayaUtil::SetStartOfBlock(true);

		if (s_progressCntl.length() > 0) {
			MGlobal::executeCommand("progressBar -e -s 1 " + s_progressCntl);
		}
		else {
			MProgressWindow::setProgressStatus(str);
			MProgressWindow::advanceProgress(1);
		}
		std::cout << str.asChar() << ":";
	}


} // namespace MayaUtil