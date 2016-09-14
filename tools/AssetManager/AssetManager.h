#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QMainWindow>

class QGridLayout;

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)

class ActionManager;

namespace AssetExplorer {
    class AssetDbViewWidget;
    class AssetExplorer;
}

class AssetManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit AssetManager(QWidget* pParent = nullptr);
    ~AssetManager();

	IContext* currentContextObject(void) const;
	IContext* contextObject(QWidget* pWidget);
	void addContextObject(IContext* pContex);
	void removeContextObject(IContext* pContex);

private:
	void createActions(void);
	void createStatusBar(void);
	void createDockWindows(void);

private:
	void updateContextObject(const QList<IContext*>& context);
	void updateContext(void);


public slots:
	void raiseWindow(void);


private:
    QGridLayout* pLayout_;

	ICore*  pCoreImpl_;
	Context additionalContexts_;

	ActionManager* pActionManager_;

	// context baby, do you speak it!
	QList<IContext*>           activeContext_;
	QMap<QWidget*, IContext*> contextWidgets_;

	assetDb::AssetDB* pDb_;
	AssetExplorer::AssetDbViewWidget* pAssetViewWidget_;
    AssetExplorer::AssetExplorer* pAssetDbexplorer_;
};

X_NAMESPACE_END

#endif // ASSETMANAGER_H
