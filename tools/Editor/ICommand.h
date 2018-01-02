#pragma once

#include <QObject>
#include "Context.h"

X_NAMESPACE_BEGIN(editor)

class ICommand : public QObject
{
	Q_OBJECT

public:
	enum CommandAttribute {
		Hide = 1,
		UpdateText = 2,
		UpdateIcon = 4,
		NonConfigurable = 8
	};

	Q_DECLARE_FLAGS(CommandAttributes, CommandAttribute)

public:
	virtual Id id(void) const X_ABSTRACT;

	virtual QAction* action(void) const X_ABSTRACT;
	virtual QShortcut* shortcut(void) const X_ABSTRACT;
	virtual Context context(void) const X_ABSTRACT;

	virtual void setAttribute(CommandAttribute attr) X_ABSTRACT;
	virtual void removeAttribute(CommandAttribute attr) X_ABSTRACT;
	virtual bool hasAttribute(CommandAttribute attr) const X_ABSTRACT;

	virtual bool isActive(void) const X_ABSTRACT;

	virtual void setKeySequence(const QKeySequence& key) X_ABSTRACT;
	virtual QString stringWithAppendedShortcut(const QString& str) const X_ABSTRACT;

	virtual void setDefaultKeySequence(const QKeySequence& key) X_ABSTRACT;
	virtual QKeySequence defaultKeySequence(void) const X_ABSTRACT;
	virtual QKeySequence keySequence(void) const X_ABSTRACT;

	// explicitly set the description (used e.g. in shortcut settings)
	// default is to use the action text for actions, or the whatsThis for shortcuts,
	// or, as a last fall back if these are empty, the command ID string
	// override the default e.g. if the text is context dependent and contains file names etc
	virtual void setDescription(const QString& text) X_ABSTRACT;
	virtual QString description(void) const X_ABSTRACT;

signals:
	void keySequenceChanged();
	void activeStateChanged();
};





X_NAMESPACE_END