#include "assetdb.h"

AssetDB::AssetDB()
{

}



 bool AssetDB::connect(const QString& path)
 {
     db_ = QSqlDatabase::addDatabase("QSQLITE");
     db_.setDatabaseName(path);



 }
