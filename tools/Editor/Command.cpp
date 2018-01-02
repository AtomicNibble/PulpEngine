#include "stdafx.h"
#include "Command.h"
#include "ProxyAction.h"

X_NAMESPACE_BEGIN(assman)




Command::Command(Id id) :
	attributes_(0), 
	id_(id),
	isKeyInitialized_(false)
{
}

void Command::setDefaultKeySequence(const QKeySequence &key)
{
	if (!isKeyInitialized_) {
		setKeySequence(key);
	}
	defaultKey_ = key;
}

QKeySequence Command::defaultKeySequence(void) const
{
	return defaultKey_;
}

void Command::setKeySequence(const QKeySequence &key)
{
	Q_UNUSED(key)
	isKeyInitialized_ = true;
}

void Command::setDescription(const QString &text)
{
	defaultText_ = text;
}

QString Command::description(void) const
{
	if (!defaultText_.isEmpty()) {
		return defaultText_;
	}

	if (action()) {
		QString text = action()->text();
		text.remove(QRegExp(QLatin1String("&(?!&)")));
		if (!text.isEmpty())
			return text;
	}
	else if (shortcut()) {
		if (!shortcut()->whatsThis().isEmpty()) {
			return shortcut()->whatsThis();
		}
	}
	return id().toString();
}

Id Command::id(void) const
{
	return id_;
}

Context Command::context(void) const
{
	return context_;
}

void Command::setAttribute(CommandAttribute attr)
{
	attributes_ |= attr;
}

void Command::removeAttribute(CommandAttribute attr)
{
	attributes_ &= ~attr;
}

bool Command::hasAttribute(CommandAttribute attr) const
{
	return (attributes_ & attr);
}

QString Command::stringWithAppendedShortcut(const QString& str) const
{
	Q_UNUSED(str);
	return ""; // Utils::ProxyAction::stringWithAppendedShortcut(str, keySequence());
}

// ---------- Shortcut ------------


Shortcut::Shortcut(Id id) :
	Command(id), 
	pShortcut_(0), 
	scriptable_(false)
{}

void Shortcut::setShortcut(QShortcut *shortcut)
{
	pShortcut_ = shortcut;
}

QShortcut* Shortcut::shortcut(void) const
{
	return pShortcut_;
}

void Shortcut::setContext(const Context& context)
{
	context_ = context;
}

Context Shortcut::context(void) const
{
	return context_;
}

void Shortcut::setKeySequence(const QKeySequence& key)
{
	Command::setKeySequence(key);
	pShortcut_->setKey(key);
	emit keySequenceChanged();
}

QKeySequence Shortcut::keySequence(void) const
{
	return pShortcut_->key();
}

void Shortcut::setCurrentContext(const Context& context)
{
	foreach(Id id, context_) 
	{
		if (context.contains(id)) {
			if (!pShortcut_->isEnabled()) {
				pShortcut_->setEnabled(true);
				emit activeStateChanged();
			}
			return;
		}
	}
	if (pShortcut_->isEnabled()) {
		pShortcut_->setEnabled(false);
		emit activeStateChanged();
	}
	return;
}

bool Shortcut::isActive(void) const
{
	return pShortcut_->isEnabled();
}


// ---------- Action ------------


Action::Action(Id id) :
	Command(id),
	pAction_(new ProxyAction(this)),
	active_(false),
	contextInitialized_(false)
{
	pAction_->setShortcutVisibleInToolTip(true);
	connect(pAction_, SIGNAL(changed()), this, SLOT(updateActiveState()));
}

QAction *Action::action(void) const
{
	return pAction_;
}

void Action::setKeySequence(const QKeySequence &key)
{
	Command::setKeySequence(key);
	pAction_->setShortcut(key);
	emit keySequenceChanged();
}

QKeySequence Action::keySequence(void) const
{
	return pAction_->shortcut();
}

void Action::setCurrentContext(const Context &context)
{
	context_ = context;

	QAction *currentAction = nullptr;
	for (int i = 0; i < context_.size(); ++i) {
		if (QAction *a = contextActionMap_.value(context_.at(i), 0)) {
			currentAction = a;
			break;
		}
	}

	pAction_->setAction(currentAction);
	updateActiveState();
}


void Action::updateActiveState()
{
	setActive(pAction_->isEnabled() && pAction_->isVisible() && !pAction_->isSeparator());
}

static QString msgActionWarning(QAction *newAction, Id id, QAction *oldAction)
{
	QString msg;
	QTextStream str(&msg);
	str << "addOverrideAction " << newAction->objectName() << '/' << newAction->text()
		<< ": Action ";
	if (oldAction) {
		str << oldAction->objectName() << '/' << oldAction->text();
	}
	str << " is already registered for context " << id.uniqueIdentifier() << ' '
		<< id.toString() << '.';
	return msg;
}

void Action::addOverrideAction(QAction *action, const Context& context)
{
	if (isEmpty()) {
		pAction_->initialize(action);
	}

	if (context.isEmpty()) {
		contextActionMap_.insert(0, action);
	}
	else {
		for (int32_t i = 0; i < context.size(); ++i) {
			Id id = context.at(i);
			if (contextActionMap_.contains(id)) {
				qWarning("%s", qPrintable(msgActionWarning(action, id, contextActionMap_.value(id, 0))));
			}
			contextActionMap_.insert(id, action);
		}
	}

	setCurrentContext(context_);
}

void Action::removeOverrideAction(QAction *action)
{
	QMutableMapIterator<Id, QPointer<QAction> > it(contextActionMap_);
	while (it.hasNext()) {
		it.next();
		if (it.value() == nullptr) {
			it.remove();
		}
		else if (it.value() == action) {
			it.remove();
		}
	}
	setCurrentContext(context_);
}

bool Action::isActive(void) const
{
	return active_;
}

void Action::setActive(bool state)
{
	if (state != active_) {
		active_ = state;
		emit activeStateChanged();
	}
}

bool Action::isEmpty(void) const
{
	return contextActionMap_.isEmpty();
}


void Action::setAttribute(CommandAttribute attr)
{
	Command::setAttribute(attr);
	switch (attr) {
	case Command::Hide:
		pAction_->setAttribute(ProxyAction::Hide);
		break;
	case Command::UpdateText:
		pAction_->setAttribute(ProxyAction::UpdateText);
		break;
	case Command::UpdateIcon:
		pAction_->setAttribute(ProxyAction::UpdateIcon);
		break;
	case Command::NonConfigurable:
		break;
	}
}

void Action::removeAttribute(CommandAttribute attr)
{
	Command::removeAttribute(attr);
	switch (attr) {
	case Command::Hide:
		pAction_->removeAttribute(ProxyAction::Hide);
		break;
	case Command::UpdateText:
		pAction_->removeAttribute(ProxyAction::UpdateText);
		break;
	case Command::UpdateIcon:
		pAction_->removeAttribute(ProxyAction::UpdateIcon);
		break;
	case Command::NonConfigurable:
		break;
	}
}



X_NAMESPACE_END