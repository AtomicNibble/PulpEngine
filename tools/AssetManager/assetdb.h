#ifndef ASSETDB_H
#define ASSETDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QDir>

#include <functional>
#include <array>

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

    struct AssetInfo
    {
        int32_t id;
        int32_t parentId;
        QString name;
    };



    typedef X_NAMESPACE(assetDb)::AssetType AssetType;

    typedef std::array<int32_t, AssetType::ENUM_COUNT> AssetTypeCountsArray;

public:
    AssetDb();

    bool OpenDB(const QString& path);
    void CloseDB(void);

    inline bool isOpen(void) const;

    ModId GetModId(const QString& name);

    bool getAssetTypeCounts(ModId modId, AssetTypeCountsArray& counts);

    bool getAssetList(ModId modId, AssetType::Enum type, QList<AssetInfo>& assetsOut);

public:
    bool IterateMods(std::function<bool(ModId id, const QString& name, QDir& outDir)> func);
    bool IterateAssets(std::function<bool(AssetType::Enum type, const QString& name)> func);
    bool IterateAssets(ModId modId, std::function<bool(AssetType::Enum, const QString& name)> func);
    bool IterateAssets(AssetType::Enum type, std::function<bool(AssetType::Enum, const QString& name)> func);

private:
    QSqlDatabase db_;
    bool isOpen_;
};


inline bool AssetDb::isOpen(void) const
{
    return isOpen_;
}

#endif // ASSETDB_H
