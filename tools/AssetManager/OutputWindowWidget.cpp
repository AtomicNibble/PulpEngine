#include "stdafx.h"
#include "OutputWindowWidget.h"

#include "ActionManager.h"
#include "Constants.h"

X_NAMESPACE_BEGIN(assman)



OutputWindow::OutputWindow(Context context, QWidget *parent) : 
	QPlainTextEdit(parent),
	pOutputWindowContext_(nullptr),
	enforceNewline_(false),
	scrollToBottom_(false),
	linksActive_(true),
	mousePressed_(false),
	maxLineCount_(10000),
	cursor_(document())
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setFrameShape(QFrame::NoFrame);
	setMouseTracking(true);
	setUndoRedoEnabled(false);

	// i want monospace bitch.
	const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	setFont(fixedFont);

	pOutputWindowContext_ = new IContext;
	pOutputWindowContext_->setContext(context);
	pOutputWindowContext_->setWidget(this);
	ICore::addContextObject(pOutputWindowContext_);

//	QAction* undoAction = new QAction(this);
//	QAction* redoAction = new QAction(this);
//	QAction* cutAction = new QAction(this);
	QAction* copyAction = new QAction(this);
//	QAction* pasteAction = new QAction(this);
	QAction* selectAllAction = new QAction(this);


//	ActionManager::registerAction(undoAction, Constants::EDIT_UNDO, context);
//	ActionManager::registerAction(redoAction, Constants::EDIT_REDO, context);
//	ActionManager::registerAction(cutAction, Constants::EDIT_CUT, context);
	ActionManager::registerAction(copyAction, Constants::EDIT_COPY, context);
//	ActionManager::registerAction(pasteAction, Constants::EDIT_PASTE, context);
	ActionManager::registerAction(selectAllAction, Constants::EDIT_SELECTALL, context);

//	connect(undoAction, &QAction::triggered, this, &QPlainTextEdit::undo);
//	connect(redoAction, &QAction::triggered, this, &QPlainTextEdit::redo);
//	connect(cutAction,	&QAction::triggered, this, &QPlainTextEdit::cut);
	connect(copyAction, &QAction::triggered, this, &QPlainTextEdit::copy);
//	connect(pasteAction, &QAction::triggered, this, &QPlainTextEdit::paste);
	connect(selectAllAction, &QAction::triggered, this, &QPlainTextEdit::selectAll);

//	connect(this, &QPlainTextEdit::undoAvailable, undoAction, &QAction::setEnabled);
//	connect(this, &QPlainTextEdit::redoAvailable, redoAction, &QAction::setEnabled);
//	connect(this, &QPlainTextEdit::copyAvailable, cutAction,  &QAction::setEnabled);  // OutputWindow never read-only
	connect(this, &QPlainTextEdit::copyAvailable, copyAction, &QAction::setEnabled);

//	undoAction->setEnabled(false);
//	redoAction->setEnabled(false);
//	cutAction->setEnabled(false);
	copyAction->setEnabled(false);
}

OutputWindow::~OutputWindow()
{
	ICore::removeContextObject(pOutputWindowContext_);
	delete pOutputWindowContext_;
}


void OutputWindow::appendBlockBegin(bool& atBottomOut)
{
	atBottomOut = isScrollbarAtBottom();
}

void OutputWindow::appendMessage(const QString& out, const QTextCharFormat& format)
{
	cursor_.movePosition(QTextCursor::End);
	cursor_.beginEditBlock();
	cursor_.insertText(out, format);
	cursor_.endEditBlock();
}

void OutputWindow::appendBlockEnd(bool atBottom)
{
	if (atBottom) {
		scrollToBottom();
	}
}


void OutputWindow::appendMessage(const QString& output)
{
	const bool atBottom = isScrollbarAtBottom();

	if (enforceNewline_) {
		appendPlainText("\n");
	}

	appendPlainText(output);

	if (atBottom) {
		scrollToBottom();
	}

//	enableUndoRedo();
}

void OutputWindow::appendMessage(const char* pLine, int32_t length)
{
	const bool atBottom = isScrollbarAtBottom();

	// fooking conn a code!
	QLatin1String tmpLatin(pLine, length);
	QString tmp(tmpLatin);

	// provide default format.
	QTextCharFormat format;

	cursor_.movePosition(QTextCursor::End);
	cursor_.beginEditBlock();
	cursor_.insertText(tmp, format);
	cursor_.endEditBlock();

	if (atBottom) {
		scrollToBottom();
	}
}

void OutputWindow::appendMessage(const char* pLine, int32_t length, const QTextCharFormat& format)
{
	const bool atBottom = isScrollbarAtBottom();

	// fooking conn a code!
	QLatin1String tmpLatin(pLine, length);
	QString tmp(tmpLatin);

	cursor_.movePosition(QTextCursor::End);
	cursor_.beginEditBlock();
	cursor_.insertText(tmp, format);
	cursor_.endEditBlock();

	if (atBottom) {
		scrollToBottom();
	}
}

void OutputWindow::clear(void)
{
	enforceNewline_ = false;
	QPlainTextEdit::clear();
}



void OutputWindow::setMaxLineCount(int32_t count)
{
	maxLineCount_ = count;
	setMaximumBlockCount(maxLineCount_);
}


int32_t OutputWindow::maxLineCount(void) const
{
	return maxLineCount_;
}

void OutputWindow::showEvent(QShowEvent *e)
{
	QPlainTextEdit::showEvent(e);
	if (scrollToBottom_) {
		verticalScrollBar()->setValue(verticalScrollBar()->maximum());
	}
	scrollToBottom_ = false;
}


void OutputWindow::setWordWrapEnabled(bool wrap)
{
	if (wrap) {
		setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	}
	else {
		setWordWrapMode(QTextOption::NoWrap);
	}
}


void OutputWindow::scrollToBottom(void)
{
	const auto max = verticalScrollBar()->maximum();

	verticalScrollBar()->setValue(max);
	verticalScrollBar()->setValue(max);
}

bool OutputWindow::isScrollbarAtBottom(void) const
{
	return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}




void OutputWindow::mousePressEvent(QMouseEvent * e)
{
	mousePressed_ = true;
	QPlainTextEdit::mousePressEvent(e);
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *e)
{
	mousePressed_ = false;

	if (linksActive_) {
		const QString href = anchorAt(e->pos());
	}

	// Mouse was released, activate links again
	linksActive_ = true;

	QPlainTextEdit::mouseReleaseEvent(e);
}

void OutputWindow::mouseMoveEvent(QMouseEvent *e)
{
	// Cursor was dragged to make a selection, deactivate links
	if (mousePressed_ && textCursor().hasSelection()) {
		linksActive_ = false;
	}

	if (!linksActive_ || anchorAt(e->pos()).isEmpty()) {
		viewport()->setCursor(Qt::IBeamCursor);
	}
	else {
		viewport()->setCursor(Qt::PointingHandCursor);
	}

	QPlainTextEdit::mouseMoveEvent(e);
}

void OutputWindow::resizeEvent(QResizeEvent *e)
{
	bool atBottom = isScrollbarAtBottom();
	QPlainTextEdit::resizeEvent(e);
	if (atBottom) {
		scrollToBottom();
	}
}

void OutputWindow::keyPressEvent(QKeyEvent *ev)
{
	QPlainTextEdit::keyPressEvent(ev);

	//Ensure we scroll also on Ctrl+Home or Ctrl+End
	if (ev->matches(QKeySequence::MoveToStartOfDocument)) {
		verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
	}
	else if (ev->matches(QKeySequence::MoveToEndOfDocument)) {
		verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
	}
}



void OutputWindow::enableUndoRedo(void)
{
	setMaximumBlockCount(0);
	setUndoRedoEnabled(true);
}



X_NAMESPACE_END