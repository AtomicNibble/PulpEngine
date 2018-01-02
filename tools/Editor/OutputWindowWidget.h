#pragma once

#include <QObject>
#include <QPlainTextEdit>

X_NAMESPACE_BEGIN(editor)

class OutputWindow : public QPlainTextEdit
{
	Q_OBJECT

public:
	OutputWindow(Context context, QWidget *parent = 0);
	~OutputWindow();

	void appendBlockBegin(bool& atBottomOut);
	void appendMessage(const QString& out, const QTextCharFormat& format);
	void appendBlockEnd(bool atBottom);


	void appendMessage(const QString& out);
	void appendMessage(const char* pLine, int32_t legnth);
	void appendMessage(const char* pLine, int32_t legnth, const QTextCharFormat& format);
	void clear(void);
	void scrollToBottom(void);

	void setMaxLineCount(int32_t count);
	int32_t maxLineCount(void) const;


	void showEvent(QShowEvent*);

public slots:
	void setWordWrapEnabled(bool wrap);

protected:
	bool isScrollbarAtBottom(void) const;

protected:
	void mousePressEvent(QMouseEvent *e) X_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) X_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) X_OVERRIDE;
	void resizeEvent(QResizeEvent *e) X_OVERRIDE;
	void keyPressEvent(QKeyEvent *ev) X_OVERRIDE;

private:
	void enableUndoRedo(void);

private:
	IContext* pOutputWindowContext_;

	bool enforceNewline_;
	bool scrollToBottom_;
	bool linksActive_;
	bool mousePressed_;
	int32_t maxLineCount_;

	QTextCursor cursor_;
};


X_NAMESPACE_END