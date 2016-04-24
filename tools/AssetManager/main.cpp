#include "assetmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AssetManager w;
    w.show();

    return a.exec();
}
