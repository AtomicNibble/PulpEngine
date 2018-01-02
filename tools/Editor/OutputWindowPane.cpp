#include "stdafx.h"
#include "OutputWindowPane.h"

#include "OutputWindowWidget.h"

X_NAMESPACE_BEGIN(editor)



OutputWindowPane::OutputWindowPane(OutputWindow* pWidget) :
	pWidget_(pWidget)
{
	pWidget_->setReadOnly(true);
}

OutputWindowPane::~OutputWindowPane()
{
}

QList<QWidget*> OutputWindowPane::toolBarWidgets(void) const
{
	return QList<QWidget*>();
}

bool OutputWindowPane::hasFocus(void) const
{
	return pWidget_->hasFocus();
}

bool OutputWindowPane::canFocus(void) const
{
	return true;
}

void OutputWindowPane::setFocus(void)
{
	pWidget_->setFocus();
}

void OutputWindowPane::clearContents(void)
{
	pWidget_->clear();
}

QWidget *OutputWindowPane::outputWidget(QWidget *parent)
{
	pWidget_->setParent(parent);
	return pWidget_;
}

QString OutputWindowPane::displayName(void) const
{
	return tr("Log");
}

void OutputWindowPane::visibilityChanged(bool /*b*/)
{
}

void OutputWindowPane::append(const QString& text)
{
	pWidget_->appendMessage(text);
}

bool OutputWindowPane::canNavigate(void) const
{
	return false;
}



X_NAMESPACE_END