#ifndef ASSETDB_H
#define ASSETDB_H

#include <QSqlDatabase>

class AssetDB
{
public:
    AssetDB();

    bool connect(const QString& path);


private:
   QSqlDatabase db_;
};

#endif // ASSETDB_H
