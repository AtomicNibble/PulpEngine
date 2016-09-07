#ifndef ASSETDB_H
#define ASSETDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QDir>

#include <functional>

class AssetDb
{
public:
    typedef int32_t ModId;
    static const ModId INVALID_MOD_ID = -1;

    enum class Result {
        OK,
        NOT_FOUND,
        NAME_TAKEN,
        UNCHANGED,
        HAS_REFS,
        ERROR
    };

    enum class AssetType {
        MODEL,
        ANIM,
        MATERIAL
    };

public:
    AssetDb();

    bool OpenDB(const QString& path);
    void CloseDB(void);

    inline bool isOpen(void) const;

    ModId GetModId(const QString& name);

public:
    bool IterateMods(std::function<bool(ModId id, const QString& name, QDir& outDir)> func);
    bool IterateAssets(std::function<bool(AssetType type, const QString& name)> func);
    bool IterateAssets(ModId modId, std::function<bool(AssetType, const QString& name)> func);
    bool IterateAssets(AssetType type, std::function<bool(AssetType, const QString& name)> func);

private:
    QSqlDatabase db_;
    bool isOpen_;
};


inline bool AssetDb::isOpen(void) const
{
    return isOpen_;
}

#endif // ASSETDB_H
