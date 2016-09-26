#include "stdafx.h"
#include "ProxyAction.h"

X_NAMESPACE_BEGIN(assman)



ProxyAction::ProxyAction(QObject *parent) :
	QAction(parent),
	action_(0),
	attributes_(0),
	showShortcut_(false),
	block_(false)
{
	connect(this, SIGNAL(changed()), this, SLOT(updateToolTipWithKeySequence()));
	updateState();
}

void ProxyAction::setAction(QAction *pAction)
{
	if (action_ == pAction) {
		return;
	}

	disconnectAction();
	action_ = pAction;
	connectAction();
	updateState();
}

void ProxyAction::updateState(void)
{
	if (action_) {
		update(action_, false);
	}
	else {
		// no active/delegate pAction, "visible" pAction is not enabled/visible
		if (hasAttribute(Hide))
			setVisible(false);
		setEnabled(false);
	}
}

void ProxyAction::disconnectAction(void)
{
	if (action_) {
		disconnect(action_, SIGNAL(changed()), this, SLOT(actionChanged()));
		disconnect(this, SIGNAL(triggered(bool)), action_, SIGNAL(triggered(bool)));
		disconnect(this, SIGNAL(toggled(bool)), action_, SLOT(setChecked(bool)));
	}
}

void ProxyAction::connectAction(void)
{
	if (action_) {
		connect(action_, SIGNAL(changed()), this, SLOT(actionChanged()));
		connect(this, SIGNAL(triggered(bool)), action_, SIGNAL(triggered(bool)));
		connect(this, SIGNAL(toggled(bool)), action_, SLOT(setChecked(bool)));
	}
}

QAction* ProxyAction::action(void) const
{
	return action_;
}

void ProxyAction::setAttribute(ProxyAction::Attribute attribute)
{
	attributes_ |= attribute;
	updateState();
}

void ProxyAction::removeAttribute(ProxyAction::Attribute attribute)
{
	attributes_ &= ~attribute;
	updateState();
}

bool ProxyAction::hasAttribute(ProxyAction::Attribute attribute)
{
	return (attributes_ & attribute);
}

void ProxyAction::actionChanged(void)
{
	update(action_, false);
}

void ProxyAction::initialize(QAction* pAction)
{
	update(pAction, true);
}

void ProxyAction::update(QAction* pAction, bool initialize)
{
	if (!pAction) {
		return;
	}

	disconnectAction();
	disconnect(this, SIGNAL(changed()), this, SLOT(updateToolTipWithKeySequence()));
	if (initialize) {
		setSeparator(pAction->isSeparator());
		setMenuRole(pAction->menuRole());
	}
	if (hasAttribute(UpdateIcon) || initialize) {
		setIcon(pAction->icon());
		setIconText(pAction->iconText());
		setIconVisibleInMenu(pAction->isIconVisibleInMenu());
	}
	if (hasAttribute(UpdateText) || initialize) {
		setText(pAction->text());
		toolTip_ = pAction->toolTip();
		updateToolTipWithKeySequence();
		setStatusTip(pAction->statusTip());
		setWhatsThis(pAction->whatsThis());
	}

	setCheckable(pAction->isCheckable());

	if (!initialize) {
		setChecked(pAction->isChecked());
		setEnabled(pAction->isEnabled());
		setVisible(pAction->isVisible());
	}
	connectAction();
	connect(this, SIGNAL(changed()), this, SLOT(updateToolTipWithKeySequence()));
}

bool ProxyAction::shortcutVisibleInToolTip(void) const
{
	return showShortcut_;
}

void ProxyAction::setShortcutVisibleInToolTip(bool visible)
{
	showShortcut_ = visible;
	updateToolTipWithKeySequence();
}

void ProxyAction::updateToolTipWithKeySequence(void)
{
	if (block_) {
		return;
	}

	block_ = true;

	if (!showShortcut_ || shortcut().isEmpty()) {
		setToolTip(toolTip_);
	}
	else {
		setToolTip(stringWithAppendedShortcut(toolTip_, shortcut()));
	}

	block_ = false;
}

QString ProxyAction::stringWithAppendedShortcut(const QString &str, const QKeySequence &shortcut)
{
	return QString::fromLatin1("%1 <span style=\"color: gray; font-size: small\">%2</span>").
		arg(str, shortcut.toString(QKeySequence::NativeText));
}


X_NAMESPACE_END