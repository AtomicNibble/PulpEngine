#pragma once

#include <QObject>
#include "Context.h"



X_NAMESPACE_BEGIN(assman)

class Command;
class ActionContainer;
class Action;


class ActionManagerPrivate : public QObject
{
	Q_OBJECT

public:
	typedef QHash<Id, Command*> IdCmdMap;
	typedef QHash<Id, ActionContainer*> IdContainerMap;

public:
	explicit ActionManagerPrivate();
	~ActionManagerPrivate();

	void initialize(void);

	void setContext(const Context& context);
	bool hasContext(int32_t context) const;

	void saveSettings(QSettings* pSettings);

	void showShortcutPopup(const QString& shortcut);
	bool hasContext(const Context& context) const;
	Action* overridableAction(Id id);

public slots:
	void containerDestroyed(void);

	void actionTriggered(void);
	void shortcutTriggered(void);

public:
	IdCmdMap idCmdMap_;
	IdContainerMap idContainerMap_;

	Context context_;

	QLabel* pPresentationLabel_;
	QTimer presentationLabelTimer_;
};


X_NAMESPACE_END