#pragma once

#include <QObject>

class QAction;
class QSettings;
class QShortcut;
class QString;

X_NAMESPACE_BEGIN(assman)

class ActionContainer;
class ICommand;
class Context;
class AssetManager;

class ActionManager : public QObject
{
	Q_OBJECT

	friend class AssetManager;

private:
	ActionManager(QObject *parent = 0);
	~ActionManager();

	void initialize(void);
	void saveSettings(QSettings* pSettings);
	void setContext(const Context& context);

public:
	static ActionManager* instance(void);

	static ActionContainer* createMenu(Id id);
	static ActionContainer* createMenuBar(Id id);

	static ICommand* registerAction(QAction *action, Id id, const Context& context);
	static ICommand* registerShortcut(QShortcut *shortcut, Id id, const Context& context);

	static ICommand* command(Id id);
	static ActionContainer* actionContainer(Id id);

	static QList<ICommand*> commands(void);

	static void unregisterAction(QAction* pAction, Id id);
	static void unregisterShortcut(Id id);

signals:
	void commandListChanged(void);
	void commandAdded(const QString& id);

};





X_NAMESPACE_END