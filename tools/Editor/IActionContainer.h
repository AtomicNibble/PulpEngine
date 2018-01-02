#pragma once

#include <QObject>
#include "Id.h"

class QMenu;
class QMenuBar;
class QAction;


X_NAMESPACE_BEGIN(editor)

class Context;
class ICommand;

class IActionContainer : public QObject
{
	Q_OBJECT

public:
	enum class OnAllDisabledBehavior {
		Disable,
		Hide,
		Show
	};

public:
	virtual ~IActionContainer() X_OVERRIDE = default;

	// This clears this menu and submenus from all actions and submenus.
	// It does not destroy the submenus and commands, just removes them from their parents.
	virtual void clear(void) X_ABSTRACT;

	virtual Id id(void) const X_ABSTRACT;

	virtual QMenu* menu(void) const X_ABSTRACT;
	virtual QMenuBar* menuBar(void) const X_ABSTRACT;

	virtual QAction* insertLocation(Id group) const X_ABSTRACT;
	virtual void appendGroup(Id group) X_ABSTRACT;
	virtual void insertGroup(Id before, Id group) X_ABSTRACT;
	virtual void addAction(ICommand* pAction, Id group = Id()) X_ABSTRACT;
	virtual void addMenu(IActionContainer* pMenu, Id group = Id()) X_ABSTRACT;
	virtual void addMenu(IActionContainer* pBefore, IActionContainer* pMenu, Id group = Id()) X_ABSTRACT;
	virtual ICommand *addSeparator(const Context& context, Id group = Id(), QAction** pOutSeparator = nullptr) X_ABSTRACT;


	virtual void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior) X_ABSTRACT;
	virtual OnAllDisabledBehavior onAllDisabledBehavior(void) const X_ABSTRACT;

};



X_NAMESPACE_END
