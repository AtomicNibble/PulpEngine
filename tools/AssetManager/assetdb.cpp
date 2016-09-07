#include "assetdb.h"
#include "logging.h"

#include <QSqlQuery>
#include <QSqlError>

AssetDb::AssetDb() :
    isOpen_(false)
{

}

bool AssetDb::OpenDB(const QString& path)
{
    if(isOpen_) {
        return true;
    }

    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(path);

    bool ok = db_.open();

    if(!ok) {
        const auto le = db_.lastError();
        qCCritical(logCatAssetDb) << "Failed to open the AssetDB(" << le.type() << "): " <<
                                     le.text();
    }

    isOpen_ = ok;
    return ok;
}


void AssetDb::CloseDB(void)
{
    db_.close();
    isOpen_ = false;
}


bool AssetDb::IterateMods(std::function<bool(ModId id, const QString& name, QDir& outDir)> func)
{
    if(!isOpen()) {
        qCCritical(logCatAssetDb) << "Can't iterate mods, the db is not open";
        return false;
    }

    QSqlQuery query(db_);

    bool res = query.exec("SELECT mod_id, name, out_dir FROM mods");
    if(res)
    {
        while (query.next()) {
             ModId id = query.value(0).toInt();
             QString name = query.value(1).toString();
             QString dir = query.value(2).toString();

             func(id, name, QDir(dir));
         }
    }else {
        qCCritical(logCatAssetDb) << "Failed to iterateMods: " << query.lastError().text();
    }
    return res;
}

bool AssetDb::IterateAssets(std::function<bool(AssetType type, const QString& name)> func)
{
    if(!isOpen()) {
        qCCritical(logCatAssetDb) << "Can't iterate assets, the db is not open";
        return false;
    }

    QSqlQuery query(db_);

    bool res = query.exec("SELECT name, type, lastUpdateTime FROM file_ids");
    if(res)
    {
        while (query.next()) {
             QString name = query.value(0).toString();
             AssetType type = static_cast<AssetType>(query.value(1).toInt());

             func(type, name);
         }
    }else {
        qCCritical(logCatAssetDb) << "Failed to iterateAssets: " << query.lastError().text();
    }
    return res;
}

bool AssetDb::IterateAssets(ModId modId, std::function<bool(AssetType, const QString& name)> func)
{
    if(!isOpen()) {
        qCCritical(logCatAssetDb) << "Can't iterate assets, the db is not open";
        return false;
    }

    QSqlQuery query(db_);
    query.prepare("SELECT name, type, lastUpdateTime FROM file_ids WHERE mod_id = ?");
    query.bindValue(0, modId);

    bool res = query.exec();
    if(res)
    {
        while (query.next()) {
             QString name = query.value(0).toString();
             AssetType type = static_cast<AssetType>(query.value(1).toInt());

             func(type, name);
         }
    }else {
        qCCritical(logCatAssetDb) << "Failed to iterateAssets: " << query.lastError().text();
    }
    return res;
}

bool AssetDb::IterateAssets(AssetType type, std::function<bool(AssetType, const QString& name)> func)
{
    if(!isOpen()) {
        qCCritical(logCatAssetDb) << "Can't iterate assets, the db is not open";
        return false;
    }

    QSqlQuery query(db_);
    query.prepare("SELECT name, lastUpdateTime FROM file_ids WHERE type = ?");
    query.bindValue(0, static_cast<int>(type));

    bool res = query.exec();
    if(res)
    {
        while (query.next()) {
             QString name = query.value(0).toString();
             AssetType type = static_cast<AssetType>(query.value(1).toInt());

             func(type, name);
         }
    } else {
        qCCritical(logCatAssetDb) << "Failed to iterateAssets: " << query.lastError().text();
    }
    return res;
}



