#include "modprojectnodes.h"
#include "modproject.h"

ModProjectNode::ModProjectNode(ModProject* pProject) :
    ProjectNode(""),
    pProject_(pProject)
{
    setDisplayName(pProject->displayName());

    setIcon(QIcon(":/assetDb/img/Mod_Project_Node_32.png"));
}


bool ModProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath);
    return false;
}

bool ModProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool ModProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}
