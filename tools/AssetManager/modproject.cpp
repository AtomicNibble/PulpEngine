#include "modproject.h"
#include "modprojectnodes.h"

// ----------------------------------

ModProject::ModProject(const QString &name, int32_t id) :
    Project(id),
    name_(name),
    rootNode_(nullptr)
{

    rootNode_ = new ModProjectNode(this);
}

ModProject::~ModProject()
{

}

QString ModProject::displayName(void) const
{
    return name_;
}


AssetExplorer::ProjectNode* ModProject::rootProjectNode() const
{
    return rootNode_;
}



