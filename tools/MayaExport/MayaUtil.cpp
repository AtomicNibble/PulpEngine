#include "stdafx.h"
#include "MayaUtil.h"

#include <maya\MGlobal.h>
#include <maya/MProgressWindow.h>


namespace MayaUtil
{
	namespace
	{
		bool g_StartOfBlock = false; // used for logging formating.
		bool g_Verbose = false;
		bool s_progressActive = false;

		int32_t gProgressMin = 0;
		int32_t gProgressMax = 0;

		MString s_progressCntl;
	}

	void SetStartOfBlock(bool start)
	{
		g_StartOfBlock = start;
	}

	void SetVerbose(bool verbose)
	{
		g_Verbose = verbose;
	}

	bool IsVerbose(void)
	{
		return g_Verbose;
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

		std::cerr << msg << std::endl;
		MGlobal::displayInfo(msg);
	}


	void MayaPrintVerbose(const char *fmt, ...)
	{
		if (!g_Verbose) {
			return;
		}
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

			std::cerr << msg << std::endl;
			MGlobal::displayInfo(msg);
		}
	}

	void PrintMatrix(const char* pName, const Matrix33f& mat)
	{
		auto colx = mat.getColumn(0);
		auto coly = mat.getColumn(1);
		auto colz = mat.getColumn(2);

		MayaUtil::MayaPrintVerbose("%s matrix:", pName);
		MayaUtil::MayaPrintVerbose("x (%f,%f,%f)", colx.x, colx.y, colx.z);
		MayaUtil::MayaPrintVerbose("y (%f,%f,%f)", coly.x, coly.y, coly.z);
		MayaUtil::MayaPrintVerbose("z (%f,%f,%f)", colz.x, colz.y, colz.z);
	}

	void PrintMatrix(const char* pName, const MMatrix& mat)
	{
		MayaUtil::MayaPrintVerbose("%s matrix:", pName);
		MayaUtil::MayaPrintVerbose("x (%f,%f,%f,%f)", mat(0, 0), mat(1, 0), mat(2, 0), mat(3, 0));
		MayaUtil::MayaPrintVerbose("y (%f,%f,%f,%f)", mat(0, 1), mat(1, 1), mat(2, 1), mat(3, 1));
		MayaUtil::MayaPrintVerbose("z (%f,%f,%f,%f)", mat(0, 2), mat(1, 2), mat(2, 2), mat(3, 2));
		MayaUtil::MayaPrintVerbose("w (%f,%f,%f,%f)", mat(0, 3), mat(1, 3), mat(2, 3), mat(3, 3));
	}

	void SetProgressCtrl(const MString& str)
	{
		s_progressCntl = str;
	}

	MStatus ShowProgressDlg(int32_t progressMin, int32_t progressMax)
	{
		gProgressMin = progressMin;
		gProgressMax = progressMax;

		using namespace std;
		if (s_progressCntl.length() > 0) 
		{
			MString min, max;
			min += progressMin;
			max += progressMax;

			MGlobal::executeCommand("progressBar -e -pr 0 -min " + min + " -max " + progressMax + " " + s_progressCntl);
		}
		else if (!s_progressActive) {
			MString title = X_ENGINE_NAME" Engine - Saving Model";
			MString process = "Starting...                                            ";

			if (!MProgressWindow::reserve()) {
				MGlobal::displayError("Progress window already in use.");
				return MS::kFailure;
			}

			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgressRange(progressMin, progressMax))
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setTitle(title));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setInterruptable(false));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgress(progressMin));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgressStatus(process));
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::startProgress());

			CHECK_MSTATUS(MProgressWindow::advanceProgress(1));


			s_progressActive = true;
		}
		return MS::kSuccess;
	}


	MStatus SetProgressRange(int32_t progressMin, int32_t progressMax)
	{
		using namespace std;

		gProgressMin = progressMin;
		gProgressMax = progressMax;

		if (s_progressCntl.length() > 0)
		{
			MString min, max;
			min += progressMin;
			max += progressMax;

			MGlobal::executeCommand("progressBar -e -min " + min + " -max " + progressMax + " " + s_progressCntl);
		}
		else if (s_progressActive)
		{
			CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgressRange(progressMin, progressMax))
		}

		return MS::kSuccess;
	}

	MStatus IncProcess(void)
	{
		if (s_progressCntl.length() > 0) {
			MGlobal::executeCommand("progressBar -e -s 1 " + s_progressCntl);
		}
		else {
			MProgressWindow::advanceProgress(1);
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

	void SetProgressText(const MString& str, bool advProgress)
	{
		if (g_StartOfBlock) {
			std::cout << "\n";
		}

		MayaUtil::SetStartOfBlock(true);

		if (advProgress)
		{
			if (s_progressCntl.length() > 0) {
				MGlobal::executeCommand("progressBar -e -s 1 " + s_progressCntl);
			}
			else {
				MProgressWindow::setProgressStatus(str);
				MProgressWindow::advanceProgress(1);
			}
		}

		std::cout << str.asChar() << ":";
	}


	MString RemoveNameSpace(const MString& str)
	{
		const int cIndex(str.rindex(':'));
		if (cIndex >= 0)
		{
			const int l(str.length());
			if (cIndex + 1 < l)
			{
				return str.substring(cIndex + 1, l - 1);
			}
		}

		return str;
	}

} // namespace MayaUtil