#ifndef MODPROJECTNODES_H
#define MODPROJECTNODES_H

#include "assetdbnodes.h"

X_NAMESPACE_BEGIN(assman)


class ModProject;

class ModProjectNode : public AssetExplorer::ProjectNode
{
    Q_OBJECT

public:
    ModProjectNode(ModProject* pProject);

    virtual bool canAddSubProject(const QString &proFilePath) const override;
    virtual bool addSubProjects(const QStringList &proFilePaths) override;
    virtual bool removeSubProjects(const QStringList &proFilePaths) override;


    ModProject* getModProject(void);
private:

private:
    ModProject* pProject_;
};


class ModVirtualFolderNode : public AssetExplorer::VirtualFolderNode
{
    typedef assetDb::AssetType AssetType;

public:
    explicit ModVirtualFolderNode(const QString &name, int priority, const QString& displayName,
                                  AssetType::Enum assType, int32_t numAssets);

    QString displayName() const override;
    QString tooltip() const override;
    bool hasUnLoadedChildren(void) const override;
    bool loadChildren(void) override;

private:
    QString displayName_;
    AssetType::Enum assetType_;
    int32_t numAssets_;
};

class ModFolderNode : public AssetExplorer::FolderNode
{
	typedef assetDb::AssetType AssetType;

public:
	explicit ModFolderNode(const QString &name);

//	QString tooltip() const override;


private:
};


X_NAMESPACE_END

#endif // MODPROJECTNODES_H
