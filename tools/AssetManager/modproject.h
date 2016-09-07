#ifndef MODPROJECT_H
#define MODPROJECT_H


#include <QObject>

#include "project.h"

class ModProjectNode;

class ModProject : public AssetExplorer::Project
{
    Q_OBJECT

public:
    ModProject(const QString &name, int32_t id);
    ~ModProject() override;

    QString displayName(void) const override;
    AssetExplorer::ProjectNode* rootProjectNode() const override;

private:
    QString name_;
    ModProjectNode* rootNode_;
};



#endif // MODPROJECT_H
