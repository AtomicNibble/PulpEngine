#include "Editor.h"
#include <QApplication>
#include <QFile>
#include <qstylefactory.h>


#include "logging.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>


HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")


X_FORCE_LINK_FACTORY("XEngineModule_ConverterLib");
X_FORCE_LINK_FACTORY("XEngineModule_AssetDB");


#endif // !X_LIB


X_LINK_LIB("engine_AssetDb")
X_LINK_LIB("engine_ConverterLib")
X_LINK_LIB("engine_MaterialLib")


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
> AssetManagerArena;

core::MemoryArenaBase* g_arena = nullptr;

X_USING_NAMESPACE;


#define REDIRECT_QT_LOGGS 0

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
  //  a.setApplicationDisplayName("Editor");
//    a.setApplicationName("Editor");
    a.setApplicationVersion("0.0.0.1");
	a.setOrganizationName("Tom Crowley");
	a.setOrganizationDomain(X_ENGINE_NAME " - Engine");
	a.setWindowIcon(QIcon(":/misc/img/icon.png"));
	a.setStyle(QStyleFactory::create("Fusion"));

#if 0
	QPalette palette = qApp->palette();
	palette.setColor(QPalette::Window, QColor(53, 53, 53));
	palette.setColor(QPalette::WindowText, Qt::white);
	palette.setColor(QPalette::Base, QColor(15, 15, 15));
	palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
	palette.setColor(QPalette::ToolTipBase, Qt::white);
	palette.setColor(QPalette::ToolTipText, Qt::white);
	palette.setColor(QPalette::Text, Qt::white);
	palette.setColor(QPalette::Button, QColor(53, 53, 53));
	palette.setColor(QPalette::ButtonText, Qt::white);
	palette.setColor(QPalette::BrightText, Qt::red);

//	palette.setColor(QPalette::Highlight, QColor(142, 45, 197).lighter());
//	palette.setColor(QPalette::HighlightedText, Qt::black);

	palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
	qApp->setPalette(palette);
#endif

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
#if REDIRECT_QT_LOGGS 
			const QtMessageHandler oldMsgHandler = qInstallMessageHandler(redirectQtMsgToEngineLog);
#endif // REDIRECT_QT_LOGGS 

			editor::Editor w;
			w.show();

			res = a.exec();

#if REDIRECT_QT_LOGGS 
			qInstallMessageHandler(oldMsgHandler);
#endif // !REDIRECT_QT_LOGGS 
		}
	}

	g_arena = nullptr;
	return res;
}
