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

	QList<AssetExplorer::ProjectAction> supportedActions(Node* pNode) const X_OVERRIDE;

	bool canAddSubProject(const QString& projectName) const X_OVERRIDE;
	bool addSubProjects(const QStringList& projectNames) X_OVERRIDE;
	bool removeSubProjects(const QStringList& projectNames) X_OVERRIDE;

	ModProject* getModProject(void);

private:
	ModProject* pProject_;
};


class ModVirtualFolderNode : public AssetExplorer::VirtualFolderNode
{
	typedef assetDb::AssetType AssetType;

public:
	explicit ModVirtualFolderNode(const QString &name, int32_t priority, const QString& displayName,
		AssetType::Enum assType, int32_t numAssets);

	QList<AssetExplorer::ProjectAction> supportedActions(Node* pNode) const X_OVERRIDE;

	QString displayName(void) const X_OVERRIDE;
	QString tooltip(void) const X_OVERRIDE;
	bool hasUnLoadedChildren(void) const X_OVERRIDE;
	bool loadChildren(void) X_OVERRIDE;

private:
	QString displayName_;
	AssetType::Enum assetType_;
	int32_t numAssets_;
};

class ModFolderNode : public AssetExplorer::FolderNode
{
	typedef assetDb::AssetType AssetType;

public:
	explicit ModFolderNode(const QString& name);

private:
};


class ModFileNode : public AssetExplorer::FileNode
{
	typedef assetDb::AssetType AssetType;

public:
	explicit ModFileNode(const QString& displayName, const QString& name, AssetType::Enum type);

private:
	QString name_;
};



X_NAMESPACE_END

#endif // MODPROJECTNODES_H
