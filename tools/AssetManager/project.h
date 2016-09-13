#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class ModProjectNode;

namespace AssetExplorer
{

	class ProjectNode;

	class Project : public QObject
	{
		Q_OBJECT
	public:
		typedef uint32_t Id;

	public:
		explicit Project(Id id);
		virtual ~Project();

		virtual QString displayName(void) const X_ABSTRACT;
		Id id(void) const;
		virtual ProjectNode* rootProjectNode(void) const X_ABSTRACT;

	signals:
		void displayNameChanged(void);
		void fileListChanged(void);

	protected:
		Id id_;
	};


} // namespace AssetExplorer

X_NAMESPACE_END


#endif // PROJECT_H
