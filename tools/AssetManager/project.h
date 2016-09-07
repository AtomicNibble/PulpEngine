#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>


class ModProjectNode;

namespace AssetExplorer {

class ProjectNode;

class Project : public QObject
{
    Q_OBJECT
public:
    typedef uint32_t Id;

public:
    Project(Id id);
    virtual ~Project();

    virtual QString displayName(void) const = 0;
    Id id(void) const;
    virtual ProjectNode* rootProjectNode() const = 0;

signals:
    void displayNameChanged();
    void fileListChanged();

protected:
    Id id_;
};


} // namespace AssetExplorer

#endif // PROJECT_H
