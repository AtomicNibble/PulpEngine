#pragma once

#include <QObject>
#include "IActionContainer.h"

class ICommand;

X_NAMESPACE_BEGIN(editor)


class ActionContainer : public IActionContainer
{
	Q_OBJECT

protected:
	struct Group
	{
		X_INLINE Group(const Id& id) : 
			id(id) 
		{
		}

		Id id;
		QList<QObject*> items;
	};

public:
	ActionContainer(Id id);
	~ActionContainer() X_OVERRIDE = default;
 
public:
	void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior);
	ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior(void) const;

	QAction *insertLocation(Id groupId) const;
	void appendGroup(Id id);
	void insertGroup(Id before, Id groupId);
	void addAction(ICommand* pAction, Id group = Id());
	void addMenu(IActionContainer* pMenu, Id group = Id());
	void addMenu(IActionContainer* pBefore, IActionContainer* pMenu, Id group = Id());
	ICommand* addSeparator(const Context& context, Id group = Id(), QAction **outSeparator = nullptr);
	virtual void clear(void);

	Id id(void) const;

	QMenu* menu(void) const;
	QMenuBar* menuBar(void) const;

	virtual void insertAction(QAction* pBefore, QAction* pAction) X_ABSTRACT;
	virtual void insertMenu(QAction* pBefore, QMenu* pMenu) X_ABSTRACT;

	virtual void removeAction(QAction* pAction) X_ABSTRACT;
	virtual void removeMenu(QMenu* pMenu) X_ABSTRACT;

	virtual bool updateInternal(void) X_ABSTRACT;

protected:
	bool canAddAction(ICommand* action) const;
	bool canAddMenu(ActionContainer* pMenu) const;
	virtual bool canBeAddedToMenu(void) const X_ABSTRACT;


private slots:
	void scheduleUpdate(void);
	void update(void);
	void itemDestroyed(void);

private:
	QList<Group>::const_iterator findGroup(Id groupId) const;
	QAction *insertLocation(QList<Group>::const_iterator group) const;


private:
	Id id_;
	OnAllDisabledBehavior onAllDisabledBehavior_;
protected:
	QList<Group> groups_;
private:
	bool updateRequested_;
};



class MenuActionContainer : public ActionContainer
{
public:
	explicit MenuActionContainer(Id id);
	~MenuActionContainer() X_OVERRIDE = default;

public:
	void setMenu(QMenu* pMenu);
	QMenu* menu(void) const;

	void insertAction(QAction* pBefore, QAction* pAction) X_OVERRIDE;
	void insertMenu(QAction* pBefore, QMenu* pMenu) X_OVERRIDE;

	void removeAction(QAction* pAction) X_OVERRIDE;
	void removeMenu(QMenu* pMenu) X_OVERRIDE;

protected:
	bool canBeAddedToMenu(void) const X_OVERRIDE;
	bool updateInternal(void) X_OVERRIDE;

private:
	QMenu* pMenu_;
};


class MenuBarActionContainer : public ActionContainer
{
public:
	explicit MenuBarActionContainer(Id id);
	~MenuBarActionContainer() X_OVERRIDE = default;

public:
	void setMenuBar(QMenuBar* pMenuBar);
	QMenuBar* menuBar(void) const;

	void insertAction(QAction* pBefore, QAction* pAction) X_OVERRIDE;
	void insertMenu(QAction* pBefore, QMenu* pMenu) X_OVERRIDE;

	void removeAction(QAction* pAction) X_OVERRIDE;
	void removeMenu(QMenu* pMenu) X_OVERRIDE;

protected:
	bool canBeAddedToMenu(void) const X_OVERRIDE;
	bool updateInternal(void) X_OVERRIDE;

private:
	QMenuBar* pMenuBar_;
};



X_NAMESPACE_END
