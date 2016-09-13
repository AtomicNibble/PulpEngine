#include "assetmanager.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include "logging.h"


#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>


HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_SqLite")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_AssetDB@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_SqLite@@0V12@A")

#endif // !X_LIB


X_LINK_LIB("engine_AssetDb")


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::NoMemoryTracking,
	core::SimpleMemoryTagging
> AssetManagerArena;

core::MemoryArenaBase* g_arena = nullptr;

X_USING_NAMESPACE;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationDisplayName("AssetManager");
    a.setApplicationName("AssetManager");
    a.setApplicationVersion("0.0.0.1");
	a.setOrganizationName("Tom Crowley");
	a.setOrganizationDomain("Potato - Engine");

#if 1
	QFile f("style\\style.qss");
	if (!f.exists())
	{
		qDebug() << "Can't load style sheet";
	}
	else
	{
		f.open(QFile::ReadOnly | QFile::Text);
		QTextStream ts(&f);
		a.setStyleSheet(ts.readAll());
	}

#else
	app.setStyleSheet(style_str);
#endif


	// this is a engine app now :)
	core::MallocFreeAllocator allocator;
	AssetManagerArena arena(&allocator, "AssetManagerArena");
	g_arena = &arena;

	int32_t res = -1;
	{
		EngineApp app; // needs to clear up before arena.

		if (app.Init(::GetCommandLineW()))
		{
			assman::AssetManager w;
			w.show();

			res = a.exec();
		}
	}

	g_arena = nullptr;
	return res;
}
