#include "stdafx.h"
#include "ActionContainer.h"
#include "ActionManager.h"
#include "ICommand.h"

#include <QDebug>
#include <QTimer>
#include <QAction>
#include <QMenuBar>

X_NAMESPACE_BEGIN(assman)


ActionContainer::ActionContainer(Id id) :
	onAllDisabledBehavior_(OnAllDisabledBehavior::Disable),
	id_(id), 
	updateRequested_(false)
{
	appendGroup(Constants::G_DEFAULT_ONE);
	appendGroup(Constants::G_DEFAULT_TWO);
	appendGroup(Constants::G_DEFAULT_THREE);
	scheduleUpdate();
}

void ActionContainer::setOnAllDisabledBehavior(OnAllDisabledBehavior behavior)
{
	onAllDisabledBehavior_ = behavior;
}

ActionContainer::OnAllDisabledBehavior ActionContainer::onAllDisabledBehavior(void) const
{
	return onAllDisabledBehavior_;
}

void ActionContainer::appendGroup(Id groupId)
{
	groups_.append(Group(groupId));
}

void ActionContainer::insertGroup(Id before, Id groupId)
{
	QList<Group>::iterator it = groups_.begin();
	while (it != groups_.end()) {
		if (it->id == before) {
			groups_.insert(it, Group(groupId));
			break;
		}
		++it;
	}
}

QList<ActionContainer::Group>::const_iterator ActionContainer::findGroup(Id groupId) const
{
	QList<Group>::const_iterator it = groups_.constBegin();
	while (it != groups_.constEnd()) {
		if (it->id == groupId) {
			break;
		}
		++it;
	}
	return it;
}


QAction* ActionContainer::insertLocation(Id groupId) const
{
	QList<Group>::const_iterator it = findGroup(groupId);

	X_ASSERT(it != groups_.constEnd(), "Failed to find group")();
	return insertLocation(it);
}

QAction* ActionContainer::insertLocation(QList<Group>::const_iterator group) const
{
	if (group == groups_.constEnd()) {
		return nullptr;
	}

	++group;
	while (group != groups_.constEnd()) 
	{
		if (!group->items.isEmpty()) 
		{
			QObject* pItem = group->items.first();
			if (ICommand* pCmd = qobject_cast<ICommand*>(pItem)) {
				return pCmd->action();
			}
			else if (ActionContainer* pContainer = qobject_cast<ActionContainer*>(pItem)) {
				if (pContainer->menu()) {
					return pContainer->menu()->menuAction();
				}
			}

			X_ASSERT_UNREACHABLE();
		}
		++group;
	}
	return 0;
}

void ActionContainer::addAction(ICommand *command, Id groupId)
{
	if (!canAddAction(command)) {
		return;
	}

	const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
	QList<Group>::const_iterator groupIt = findGroup(actualGroupId);


	QAction *beforeAction = insertLocation(groupIt);
	groups_[groupIt - groups_.constBegin()].items.append(command);

	connect(command, SIGNAL(activeStateChanged()), this, SLOT(scheduleUpdate()));
	connect(command, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
	insertAction(beforeAction, command->action());
	scheduleUpdate();
}

void ActionContainer::addMenu(IActionContainer* pMenu, Id groupId)
{
	ActionContainer *containerPrivate = static_cast<ActionContainer*>(pMenu);
	if (!containerPrivate->canBeAddedToMenu()) {
		return;
	}

	MenuActionContainer *container = static_cast<MenuActionContainer*>(containerPrivate);
	const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
	QList<Group>::const_iterator groupIt = findGroup(actualGroupId);

	//    BUG_ASSERT(groupIt != groups_.constEnd(), return);

	QAction* pBeforeAction = insertLocation(groupIt);
	groups_[groupIt - groups_.constBegin()].items.append(pMenu);

	connect(pMenu, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
	insertMenu(pBeforeAction, container->menu());
	scheduleUpdate();
}

void ActionContainer::addMenu(IActionContainer *before, IActionContainer* pMenu, Id groupId)
{
	ActionContainer *containerPrivate = static_cast<ActionContainer*>(pMenu);
	if (!containerPrivate->canBeAddedToMenu()) {
		return;
	}

	MenuActionContainer *container = static_cast<MenuActionContainer *>(containerPrivate);
	const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
	QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
	//    BUG_ASSERT(groupIt != groups_.constEnd(), return);
	QAction* pBeforeAction = before->menu()->menuAction();
	groups_[groupIt - groups_.constBegin()].items.append(pMenu);

	connect(pMenu, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
	insertMenu(pBeforeAction, container->menu());
	scheduleUpdate();
}

ICommand *ActionContainer::addSeparator(const Context &context, Id group, QAction **outSeparator)
{
	static int32_t separatorIdCount = 0;

	QAction* pSeparator = new QAction(this);
	pSeparator->setSeparator(true);

	Id sepId = id().withSuffix(".Separator.").withSuffix(++separatorIdCount);

	ICommand* pCmd = ActionManager::registerAction(pSeparator, sepId, context);
	addAction(pCmd, group);

	if (outSeparator) {
		*outSeparator = pSeparator;
	}

	return pCmd;
}

void ActionContainer::clear(void)
{
	QMutableListIterator<Group> it(groups_);
	while (it.hasNext()) 
	{
		Group& group = it.next();
		for(QObject* pItem : group.items) 
		{
			if (ICommand* pCommand = qobject_cast<ICommand*>(pItem)) {
				removeAction(pCommand->action());
				disconnect(pCommand, SIGNAL(activeStateChanged()), this, SLOT(scheduleUpdate()));
				disconnect(pCommand, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
			}
			else if (ActionContainer* pContainer = qobject_cast<ActionContainer*>(pItem)) {
				pContainer->clear();
				disconnect(pContainer, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
				removeMenu(pContainer->menu());
			}
		}
		group.items.clear();
	}
	scheduleUpdate();
}

void ActionContainer::itemDestroyed(void)
{
	QObject* obj = sender();

	QMutableListIterator<Group> it(groups_);
	while (it.hasNext())
	{
		Group& group = it.next();
		if (group.items.removeAll(obj) > 0) {
			break;
		}
	}
}

Id ActionContainer::id(void) const
{
	return id_;
}

QMenu* ActionContainer::menu(void) const
{
	return nullptr;
}

QMenuBar* ActionContainer::menuBar(void) const
{
	return nullptr;
}

bool ActionContainer::canAddAction(ICommand* pAction) const
{
	return pAction && pAction->action();
}

void ActionContainer::scheduleUpdate(void)
{
	if (updateRequested_) {
		return;
	}

	updateRequested_ = true;
	QTimer::singleShot(0, this, SLOT(update()));
}

void ActionContainer::update(void)
{
	updateInternal();
	updateRequested_ = false;
}

// ---------- MenuActionContainer ------------


MenuActionContainer::MenuActionContainer(Id id) :
	ActionContainer(id), 
	pMenu_(0)
{
	setOnAllDisabledBehavior(OnAllDisabledBehavior::Disable);
}

void MenuActionContainer::setMenu(QMenu* pMenu)
{
	pMenu_ = pMenu;
}

QMenu* MenuActionContainer::menu(void) const
{
	return pMenu_;
}

void MenuActionContainer::insertAction(QAction *before, QAction *action)
{
	pMenu_->insertAction(before, action);
}

void MenuActionContainer::insertMenu(QAction *before, QMenu* pMenu)
{
	pMenu_->insertMenu(before, pMenu);
}

void MenuActionContainer::removeAction(QAction *action)
{
	pMenu_->removeAction(action);
}

void MenuActionContainer::removeMenu(QMenu* pMenu)
{
	pMenu_->removeAction(pMenu->menuAction());
}

static bool menuInMenuBar(const QMenu* pMenu)
{
	for(const QWidget* pWidget : pMenu->menuAction()->associatedWidgets()) {
		if (qobject_cast<const QMenuBar *>(pWidget)) {
			return true;
		}
	}
	return false;
}

bool MenuActionContainer::updateInternal(void)
{
	if (onAllDisabledBehavior() == OnAllDisabledBehavior::Show) {
		return true;
	}

	bool hasitems = false;
	QList<QAction*> actions = pMenu_->actions();

	QListIterator<Group> it(groups_);
	while (it.hasNext())
	{
		const Group& group = it.next();
		for(QObject* pItem : group.items) 
		{
			if (ActionContainer* pContainer = qobject_cast<ActionContainer*>(pItem)) 
			{
				actions.removeAll(pContainer->menu()->menuAction());
				if (pContainer == this)
				{
					QByteArray warning = Q_FUNC_INFO + QByteArray(" container '");
					if (this->menu()) {
						warning += this->menu()->title().toLocal8Bit();
					}
					warning += "' contains itself as subcontainer";
					qWarning("%s", warning.constData());
					continue;
				}
				if (pContainer->updateInternal()) {
					hasitems = true;
					break;
				}
			}
			else if (ICommand* pCommand = qobject_cast<ICommand*>(pItem)) {
				actions.removeAll(pCommand->action());
				if (pCommand->isActive()) {
					hasitems = true;
					break;
				}
			}
			else {
				X_ASSERT_UNREACHABLE();
				continue;
			}
		}
		if (hasitems) {
			break;
		}
	}
	if (!hasitems) {
		// look if there were actions added that we don't control and check if they are enabled
		for(const QAction* pAction : actions) {
			if (!pAction->isSeparator() && pAction->isEnabled()) {
				hasitems = true;
				break;
			}
		}
	}

	if (onAllDisabledBehavior() == OnAllDisabledBehavior::Hide) {
		pMenu_->menuAction()->setVisible(hasitems);
	}
	else if (onAllDisabledBehavior() == OnAllDisabledBehavior::Disable) {
		pMenu_->menuAction()->setEnabled(hasitems);
	}

	return hasitems;
}

bool MenuActionContainer::canBeAddedToMenu(void) const
{
	return true;
}


// ---------- MenuBarActionContainer ------------


MenuBarActionContainer::MenuBarActionContainer(Id id) :
	ActionContainer(id),
	pMenuBar_(nullptr)
{
	setOnAllDisabledBehavior(OnAllDisabledBehavior::Show);
}

void MenuBarActionContainer::setMenuBar(QMenuBar *menuBar)
{
	pMenuBar_ = menuBar;
}

QMenuBar *MenuBarActionContainer::menuBar(void) const
{
	return pMenuBar_;
}

void MenuBarActionContainer::insertAction(QAction *before, QAction *action)
{
	pMenuBar_->insertAction(before, action);
}

void MenuBarActionContainer::insertMenu(QAction *before, QMenu* pMenu)
{
	pMenuBar_->insertMenu(before, pMenu);
}

void MenuBarActionContainer::removeAction(QAction *action)
{
	pMenuBar_->removeAction(action);
}

void MenuBarActionContainer::removeMenu(QMenu* pMenu)
{
	pMenuBar_->removeAction(pMenu->menuAction());
}

bool MenuBarActionContainer::updateInternal(void)
{
	if (onAllDisabledBehavior() == OnAllDisabledBehavior::Show) {
		return true;
	}

	bool hasitems = false;
	QList<QAction*> actions = pMenuBar_->actions();
	for (int32_t i = 0; i<actions.size(); ++i) {
		if (actions.at(i)->isVisible()) {
			hasitems = true;
			break;
		}
	}

	if (onAllDisabledBehavior() == OnAllDisabledBehavior::Hide) {
		pMenuBar_->setVisible(hasitems);
	}
	else if (onAllDisabledBehavior() == OnAllDisabledBehavior::Disable) {
		pMenuBar_->setEnabled(hasitems);
	}

	return hasitems;
}

bool MenuBarActionContainer::canBeAddedToMenu(void) const
{
	return false;
}


X_NAMESPACE_END