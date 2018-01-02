#pragma once

#include <QObject>
#include "IOutputPane.h"

X_NAMESPACE_BEGIN(editor)

class OutputWindow;

class OutputWindowPane : public IOutputPane
{
	Q_OBJECT

public:
	OutputWindowPane(OutputWindow* pWidget);
	~OutputWindowPane();

	QWidget *outputWidget(QWidget *parent);
	QList<QWidget*> toolBarWidgets(void) const;


	QString displayName(void) const;
	void clearContents(void);
	void visibilityChanged(bool visible);

	void append(const QString& text);
	bool canFocus(void) const;
	bool hasFocus(void) const;
	void setFocus(void);

	bool canNavigate(void) const;

private:
	OutputWindow* pWidget_;
};


X_NAMESPACE_END