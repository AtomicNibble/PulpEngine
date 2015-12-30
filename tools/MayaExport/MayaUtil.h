#pragma once


namespace MayaUtil
{
	void SetStartOfBlock(bool start);

	void MayaPrintError(const char *fmt, ...);
	void MayaPrintWarning(const char *fmt, ...);
	void MayaPrintMsg(const char *fmt, ...);


	void SetProgressCtrl(const MString& str);
	MStatus ShowProgressDlg(void);
	MStatus HideProgressDlg(void);
	void SetProgressText(const MString& str);

} // namespace MayaUtil