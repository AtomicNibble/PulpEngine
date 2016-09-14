#include "stdafx.h"
#include "ActionContainer.h"
#include "ActionManagerPrivate.h"
#include "Command.h"

X_NAMESPACE_BEGIN(assman)



ActionManagerPrivate::ActionManagerPrivate()
	: pPresentationLabel_(nullptr)
{
	presentationLabelTimer_.setInterval(1000);
}

ActionManagerPrivate::~ActionManagerPrivate()
{
	// first delete containers to avoid them reacting to command deletion
	for (ActionContainer* pContainer : idContainerMap_) {
		disconnect(pContainer, SIGNAL(destroyed()), this, SLOT(containerDestroyed()));
	}

	qDeleteAll(idContainerMap_);
	qDeleteAll(idCmdMap_);
}


void ActionManagerPrivate::setContext(const Context& context)
{
	// here are possibilities for speed optimization if necessary:
	// let commands (de-)register themselves for contexts
	// and only update commands that are either in old or new contexts
	context_ = context;

	for (const auto& cmd : idCmdMap_) {
		cmd->setCurrentContext(context_);
	}
}

bool ActionManagerPrivate::hasContext(const Context& context) const
{
	for (int32_t i = 0; i < context_.size(); ++i) {
		if (context.contains(context_.at(i))) {
			return true;
		}
	}
	return false;
}

void ActionManagerPrivate::containerDestroyed(void)
{
	ActionContainer* pContainer = static_cast<ActionContainer*>(sender());
	idContainerMap_.remove(idContainerMap_.key(pContainer));
}

void ActionManagerPrivate::actionTriggered(void)
{
	QAction* pAction = qobject_cast<QAction*>(QObject::sender());
	if (pAction) {
		showShortcutPopup(pAction->shortcut().toString());
	}
}

void ActionManagerPrivate::shortcutTriggered(void)
{
	QShortcut* pSc = qobject_cast<QShortcut *>(QObject::sender());
	if (pSc) {
		showShortcutPopup(pSc->key().toString());
	}
}


void ActionManagerPrivate::showShortcutPopup(const QString& shortcut)
{
	if (shortcut.isEmpty()) {
		return;
	}

	pPresentationLabel_->setText(shortcut);
	pPresentationLabel_->adjustSize();

	QPoint p = ICore::mainWindow()->mapToGlobal(ICore::mainWindow()->rect().center() - pPresentationLabel_->rect().center());
	pPresentationLabel_->move(p);

	pPresentationLabel_->show();
	pPresentationLabel_->raise();
	presentationLabelTimer_.start();
}

Action *ActionManagerPrivate::overridableAction(Id id)
{
	Action* pAction = nullptr;

	if (Command* pCmd = idCmdMap_.value(id, 0)) {
		pAction = qobject_cast<Action*>(pCmd);
		if (!pAction) {
			qWarning() << "registerAction: id" << id.name()
				<< "is registered with a different command type.";
			return nullptr;
		}
	}
	else {
		pAction = new Action(id);
		idCmdMap_.insert(id, pAction);
		ICore::mainWindow()->addAction(pAction->action());
		pAction->action()->setObjectName(id.toString());
		pAction->action()->setShortcutContext(Qt::ApplicationShortcut);
		pAction->setCurrentContext(context_);
	}

	return pAction;
}


void ActionManagerPrivate::initialize(void)
{



}


void ActionManagerPrivate::saveSettings(QSettings* pSettings)
{
	X_UNUSED(pSettings);
	X_ASSERT_NOT_IMPLEMENTED();
}

X_NAMESPACE_END