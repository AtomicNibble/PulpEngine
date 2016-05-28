#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>

class ProjectNode;


namespace AssetExplorer {


class Project : public QObject
{
    Q_OBJECT
public:
    typedef uint32_t Id;

    Project();
    virtual ~Project();

    virtual QString displayName() const = 0;
    Id id(void) const;
    virtual ProjectNode *rootProjectNode() const = 0;

signals:
    void displayNameChanged();

protected:
     void setId(Id id);

     Id m_id;
     QString m_Path;
};

} // namespace AssetExplorer

#endif // PROJECT_H
