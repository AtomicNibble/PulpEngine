#include "stdafx.h"
#include "ActionManager.h"
#include "ActionManagerPrivate.h"
#include "ActionContainer.h"
#include "Command.h"

X_NAMESPACE_BEGIN(assman)

namespace
{

	static ActionManager* pInstance = nullptr;
	static ActionManagerPrivate* pPrivateInst = nullptr;

} // namespace


ActionManager::ActionManager(QObject *parent)
	: QObject(parent)
{
	pInstance = this;
	pPrivateInst = new ActionManagerPrivate;
}

ActionManager::~ActionManager()
{
	delete pPrivateInst;
}

ActionManager* ActionManager::instance(void)
{
	return pInstance;
}


ActionContainer* ActionManager::createMenu(Id id)
{
	const auto it = pPrivateInst->idContainerMap_.constFind(id);
	if (it != pPrivateInst->idContainerMap_.constEnd()) {
		return it.value();
	}

	QMenu* pMenu = new QMenu(ICore::mainWindow());
	pMenu->setObjectName(QLatin1String(id.name()));

	MenuActionContainer* pMc = new MenuActionContainer(id);
	pMc->setMenu(pMenu);

	pPrivateInst->idContainerMap_.insert(id, pMc);
	connect(pMc, SIGNAL(destroyed()), pPrivateInst, SLOT(containerDestroyed()));

	return pMc;
}

ActionContainer* ActionManager::createMenuBar(Id id)
{
	const auto it = pPrivateInst->idContainerMap_.constFind(id);
	if (it != pPrivateInst->idContainerMap_.constEnd()) {
		return it.value();
	}

	QMenuBar* pMb = new QMenuBar; // No parent (System menu bar on Mac OS X)
	pMb->setObjectName(id.toString());

	MenuBarActionContainer* pMbac = new MenuBarActionContainer(id);
	pMbac->setMenuBar(pMb);

	pPrivateInst->idContainerMap_.insert(id, pMbac);
	connect(pMbac, SIGNAL(destroyed()), pPrivateInst, SLOT(containerDestroyed()));

	return pMbac;
}


ICommand* ActionManager::registerAction(QAction *action, Id id, const Context &contex)
{
	Action* pAction = pPrivateInst->overridableAction(id);
	if (pAction) {
		pAction->addOverrideAction(action, contex);
		emit pInstance->commandListChanged();
		emit pInstance->commandAdded(id.toString());
	}
	return pAction;
}


ICommand* ActionManager::registerShortcut(QShortcut *shortcut, Id id, const Context &context)
{
	BUG_CHECK(!context.isEmpty());
	Shortcut* pSc = nullptr;

	if (Command* pC = pPrivateInst->idCmdMap_.value(id, 0))
	{
		pSc = qobject_cast<Shortcut*>(pC);
		if (!pSc) {
			qWarning() << "registerShortcut: id" << id.name()
				<< "is registered with a different command type.";
			return pSc;
		}
	}
	else 
	{
		pSc = new Shortcut(id);
		pPrivateInst->idCmdMap_.insert(id, pSc);
	}

	if (pSc->shortcut()) {
		qWarning() << "registerShortcut: action already registered, id" << id.name() << ".";
		return pSc;
	}

	if (!pPrivateInst->hasContext(context)) {
		shortcut->setEnabled(false);
	}

	shortcut->setObjectName(id.toString());
	shortcut->setParent(ICore::mainWindow());
	shortcut->setContext(Qt::ApplicationShortcut);
	pSc->setShortcut(shortcut);
	//    sc->setScriptable(scriptable);
	pSc->setContext(context);

	emit pInstance->commandListChanged();
	emit pInstance->commandAdded(id.toString());

	//    if (isPresentationModeEnabled())
	//        connect(sc->shortcut(), SIGNAL(activated()), d, SLOT(shortcutTriggered()));
	return pSc;
}


ICommand* ActionManager::command(Id id)
{
	const auto it = pPrivateInst->idCmdMap_.constFind(id);
	if (it == pPrivateInst->idCmdMap_.constEnd()) {
			qWarning() << "ActionManagerPrivate::command(): failed to find :"
			<< id.name();
		return nullptr;
	}
	return it.value();
}

ActionContainer* ActionManager::actionContainer(Id id)
{
	const auto it = pPrivateInst->idContainerMap_.constFind(id);
	if (it == pPrivateInst->idContainerMap_.constEnd()) {
			qWarning() << "ActionManager::actionContainer(): failed to find :"
				<< id.name();
		return nullptr;
	}
	return it.value();
}


QList<ICommand*> ActionManager::commands(void)
{
	// transform list of Command into list of ICommand
	QList<ICommand*> result;
	for (ICommand* pCmd : pPrivateInst->idCmdMap_) {
		result << pCmd;
	}
	return result;
}


void ActionManager::setContext(const Context& context)
{
	pPrivateInst->setContext(context);
}

void ActionManager::initialize(void)
{
	pPrivateInst->initialize();
}

void ActionManager::saveSettings(QSettings* pSettings)
{
	pPrivateInst->saveSettings(pSettings);
}




X_NAMESPACE_END