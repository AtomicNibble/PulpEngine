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

	bool addFile(const core::string& name, assetDb::AssetType::Enum type) X_OVERRIDE;
	bool removeFile(const core::string& name, assetDb::AssetType::Enum type) X_OVERRIDE;

	bool clean(ConverterHost& conHost) const X_OVERRIDE;

	ModProject* getModProject(void);

private:
	bool build(ConverterHost& conHost, bool force) const X_OVERRIDE;

private:
	ModProject* pProject_;
};


class ModVirtualFolderNode : public AssetExplorer::AssetTypeVirtualFolderNode
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

	bool addFile(const core::string& name, assetDb::AssetType::Enum type) X_OVERRIDE;
	bool removeFile(const core::string& name, assetDb::AssetType::Enum type) X_OVERRIDE;

private:
	bool build(ConverterHost& conHost, bool force) const X_OVERRIDE;

private:
	QString displayName_;
	int32_t numAssets_;
};

class ModFolderNode : public AssetExplorer::FolderNode
{
	typedef assetDb::AssetType AssetType;

public:
	explicit ModFolderNode(const QString& name);

private:
	bool build(ConverterHost& conHost, bool force) const X_OVERRIDE;

private:
};


class ModFileNode : public AssetExplorer::FileNode
{
	typedef assetDb::AssetType AssetType;

public:
	explicit ModFileNode(const QString& displayName, const QString& name, AssetType::Enum type);

private:
	bool build(ConverterHost& conHost, bool force) const X_OVERRIDE;


private:
	QString name_;
};



X_NAMESPACE_END

#endif // MODPROJECTNODES_H
