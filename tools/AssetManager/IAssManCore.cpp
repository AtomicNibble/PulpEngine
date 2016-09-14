#include "stdafx.h"
#include "ICore.h"

#include "assetmanager.h"

X_NAMESPACE_BEGIN(assman)

namespace
{

	static ICore* pInstance = nullptr;
	static AssetManager* pMainwindow = nullptr;

} // namespace 


ICore::ICore(AssetManager* pMainWindow)
{
	pInstance = this;
	pMainwindow = pMainWindow;

}

ICore::~ICore()
{
	pInstance = nullptr;
	pMainwindow = nullptr;
}


ICore* ICore::instance(void)
{
	return pInstance;
}

QString ICore::versionString(void)
{
	return tr("Lady Bug %1").arg(QLatin1String(Constants::IDE_VERSION_LONG));
}

QWidget* ICore::mainWindow(void)
{
	return pMainwindow;
}

QWidget* ICore::dialogParent(void)
{
	QWidget *active = QApplication::activeModalWidget();
	return active ? active : pMainwindow;
}


void ICore::raiseWindow(QWidget *widget)
{
	if (!widget) {
		return;
	}

	QWidget* pWindow = widget->window();
	if (pWindow == pMainwindow) {
		pMainwindow->raiseWindow();
	}
	else {
		pWindow->raise();
		pWindow->activateWindow();
	}
}


IContext* ICore::currentContextObject(void)
{
	return pMainwindow->currentContextObject();
}

void ICore::addContextObject(IContext* context)
{
	pMainwindow->addContextObject(context);
}

void ICore::removeContextObject(IContext* context)
{
	pMainwindow->removeContextObject(context);
}


void ICore::registerListner(ICoreListener* listener)
{
	pInstance->listeners_.push_back(listener);
}

const QList<ICoreListener *> ICore::getCoreListners(void)
{
	return pInstance->listeners_;
}

bool ICore::callCoreCloseListners(QCloseEvent* event)
{
	for(ICoreListener* pListener : pInstance->listeners_) {
		if (!pListener->coreAboutToClose()) {
			event->ignore();
			return false;
		}
	}
	return true;
}





X_NAMESPACE_END