#include "assetmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationDisplayName("AssetManager");
    a.setApplicationName("AssetManager");
    a.setApplicationVersion("0.0.0.1");

    AssetManager w;
    w.show();

    return a.exec();
}
