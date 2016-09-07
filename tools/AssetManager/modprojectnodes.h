#ifndef MODPROJECTNODES_H
#define MODPROJECTNODES_H

#include "assetdbnodes.h"



class ModProject;

class ModProjectNode : public AssetExplorer::ProjectNode
{
public:
    ModProjectNode(ModProject* pProject);

    virtual bool canAddSubProject(const QString &proFilePath) const override;
    virtual bool addSubProjects(const QStringList &proFilePaths) override;
    virtual bool removeSubProjects(const QStringList &proFilePaths) override;

private:

private:
    ModProject* pProject_;
};


#endif // MODPROJECTNODES_H
