#pragma once

#include <QObject>
#include "Context.h"
#include "ICommand.h"

X_NAMESPACE_BEGIN(editor)

class ProxyAction;

class Command : public ICommand
{
	Q_OBJECT
public:
	Command(Id id);
	virtual ~Command() X_OVERRIDE = default;

	Id id(void) const X_OVERRIDE;
	Context context(void) const X_OVERRIDE;

	void setDefaultKeySequence(const QKeySequence& key) X_OVERRIDE;
	QKeySequence defaultKeySequence(void) const X_OVERRIDE;

	void setKeySequence(const QKeySequence& key) X_OVERRIDE;

	void setDescription(const QString& text) X_OVERRIDE;
	QString description(void) const X_OVERRIDE;


	void setAttribute(CommandAttribute attr) X_OVERRIDE;
	void removeAttribute(CommandAttribute attr) X_OVERRIDE;
	bool hasAttribute(CommandAttribute attr) const X_OVERRIDE;

	QString stringWithAppendedShortcut(const QString& str) const X_OVERRIDE;

	virtual void setCurrentContext(const Context& context) X_ABSTRACT;

protected:
	Context context_;
	CommandAttributes attributes_;
	Id id_;
	QKeySequence defaultKey_;
	QString defaultText_;
	bool isKeyInitialized_;
};

class Shortcut : public Command
{
	Q_OBJECT
public:
	Shortcut(Id id);
	virtual ~Shortcut() X_OVERRIDE = default;


	void setKeySequence(const QKeySequence& key) X_OVERRIDE;
	QKeySequence keySequence(void) const X_OVERRIDE;

	void setShortcut(QShortcut *shortcut);

	X_INLINE QAction* action(void) const X_OVERRIDE;
	X_INLINE QShortcut* shortcut(void) const X_OVERRIDE;

	void setContext(const Context& context);
	Context context(void) const;
	void setCurrentContext(const Context& context) X_OVERRIDE;

	bool isActive(void) const;

private:
	QShortcut* pShortcut_;
	bool scriptable_;
};

class Action : public Command
{
	Q_OBJECT
public:
	Action(Id id);
	virtual ~Action() X_OVERRIDE = default;


	void setKeySequence(const QKeySequence& key);
	QKeySequence keySequence(void) const;

	QAction* action(void) const X_OVERRIDE;
	X_INLINE QShortcut* shortcut(void) const X_OVERRIDE;

	void setCurrentContext(const Context& context) X_OVERRIDE;
	bool isActive(void) const;
	void addOverrideAction(QAction* pAction, const Context& context);
	void removeOverrideAction(QAction* pAction);
	bool isEmpty(void) const;

	void setAttribute(CommandAttribute attr) X_OVERRIDE;
	void removeAttribute(CommandAttribute attr) X_OVERRIDE;

private slots:
	void updateActiveState(void);

private:
	void setActive(bool state);

private:
	ProxyAction* pAction_;
	QString toolTip_;

	QMap<Id, QPointer<QAction> > contextActionMap_;
	QMap<QAction*, bool> scriptableMap_;
	bool active_;
	bool contextInitialized_;
};


X_NAMESPACE_END


#include "Command.inl"