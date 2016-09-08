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


// -------------------------------------------------------------------


ModVirtualFolderNode::ModVirtualFolderNode(const QString &name, int priority, const QString& displayName,
                              AssetType::Enum assType, int32_t numAssets) :
    VirtualFolderNode(name, priority),
    displayName_(displayName),
    assetType_(assType),
    numAssets_(numAssets)
{

}

QString ModVirtualFolderNode::displayName() const
{
    return displayName_;
}

QString ModVirtualFolderNode::tooltip() const
{
    return QString("%1 (%2)").arg(AssetType::ToString(assetType_), QString::number(numAssets_));
}


bool ModVirtualFolderNode::hasLazyChildren(void) const
{
    return numAssets_ > 0;
}
