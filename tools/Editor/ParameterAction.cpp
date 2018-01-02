#include "stdafx.h"
#include "ParameterAction.h"


X_NAMESPACE_BEGIN(editor)

/*
The ParameterAction class is intended for actions that act on a 'current',
string - type parameter(typically a file name), for example 'Save file %1'.
*/

ParameterAction::ParameterAction(const QString &emptyText, const QString &parameterText, EnablingMode mode, QObject* parent) :
	QAction(emptyText, parent),
	emptyText_(emptyText),
	parameterText_(parameterText),
	enablingMode_(mode)
{
}

QString ParameterAction::emptyText(void) const
{
	return emptyText_;
}

void ParameterAction::setEmptyText(const QString &t)
{
	emptyText_ = t;
}

QString ParameterAction::parameterText(void) const
{
	return parameterText_;
}

void ParameterAction::setParameterText(const QString &t)
{
	parameterText_ = t;
}

ParameterAction::EnablingMode ParameterAction::enablingMode(void) const
{
	return enablingMode_;
}

void ParameterAction::setEnablingMode(EnablingMode m)
{
	enablingMode_ = m;
}

void ParameterAction::setParameter(const QString &p)
{
	const bool enabled = !p.isEmpty();
	if (enabled) {
		setText(parameterText_.arg(p));
	}
	else {
		setText(emptyText_);
	}
	if (enablingMode_ == EnablingMode::EnabledWithParameter) {
		setEnabled(enabled);
	}
}


X_NAMESPACE_END