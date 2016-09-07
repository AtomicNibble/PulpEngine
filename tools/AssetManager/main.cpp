#include "assetmanager.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "logging.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationDisplayName("AssetManager");
    a.setApplicationName("AssetManager");
    a.setApplicationVersion("0.0.0.1");
    a.setOrganizationName("Tom Crowley");
    a.setOrganizationDomain("Potato - Engine");

#if 1
   QFile f("qdarkstyle\\style.qss");
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

    AssetManager w;
    w.show();

    return a.exec();
}
