#ifndef ASSETDBNODES_H
#define ASSETDBNODES_H


X_NAMESPACE_BEGIN(assman)

class ConverterHost;

namespace AssetExplorer
{


	enum class NodeType {
		FileNodeType = 1,
		FolderNodeType,
		VirtualFolderNodeType,
		ProjectNodeType,
		SessionNodeType
	};

	enum class ProjectAction {
		// Special value to indicate that the actions are handled by the parent
		InheritedFromParent,
		AddSubProject,
		RemoveSubProject,
		// Let's the user select to which project file
		// the file is added
		AddNewFile,
		// DeleteFile is a define on windows...
		EraseFile,
		Rename,
		// hides actions that use the path(): Open containing folder, open terminal here and Find in Directory
		HidePathActions,
		HasSubProjectRunConfigurations
	};

	class Node;
	class FileNode;
	class FileContainerNode;
	class FolderNode;
	class ProjectNode;
	class NodesWatcher;
	class NodesVisitor;
	class SessionManager;



	class Node : public QObject
	{
		Q_OBJECT
	public:
		NodeType nodeType(void) const;
		ProjectNode* projectNode(void) const;     // managing project
		FolderNode* parentFolderNode(void) const; // parent folder or project
		QString name(void) const;                 // name
		QIcon icon(void) const;
		virtual QString displayName(void) const;
		virtual QString tooltip(void) const;
		virtual bool isEnabled(void) const;

		virtual QList<ProjectAction> supportedActions(Node *node) const;

		virtual bool build(ConverterHost& conHost, bool force = false) const X_ABSTRACT;

		void setName(const QString& name);
		void setIcon(const QIcon& icon);
		void emitNodeUpdated(void);

	protected:
		Node(NodeType nodeType, const QString& name);

		void setNodeType(NodeType type);
		void setProjectNode(ProjectNode* pProject);
		void setParentFolderNode(FolderNode* pParentFolder);

		void emitNodeSortKeyAboutToChange(void);
		void emitNodeSortKeyChanged(void);

	private:
		NodeType nodeType_;
		ProjectNode* pProjectNode_;
		FolderNode* pFolderNode_;
		QString name_;
		mutable QIcon icon_;
	};



	class FileNode : public Node
	{
		Q_OBJECT

		typedef assetDb::AssetType AssetType;

	public:
		FileNode(const QString& name, const assetDb::AssetType::Enum assetType);

		AssetType::Enum assetType(void) const;

	private:
		// managed by ProjectNode
		friend class FolderNode;
		friend class ProjectNode;

		AssetType::Enum assetType_;
	};


	class FolderNode : public Node
	{
		Q_OBJECT
	public:
		explicit FolderNode(const QString& name, NodeType nodeType = NodeType::FolderNodeType);
		virtual ~FolderNode();

		QString displayName(void) const override;
		QIcon icon(bool expanded) const;
		QIcon iconExpanded(void) const;

		QList<FileNode*> fileNodes(void) const;
		QList<FolderNode*> subFolderNodes(void) const;

		void setIconExpanded(const QIcon& icon);
		void setDisplayName(const QString& name);

		FileNode* findFile(const QString& path, assetDb::AssetType::Enum type);
		FolderNode* findSubFolder(const QString& path);

		virtual bool hasUnLoadedChildren(void) const;
		virtual bool loadChildren(void);

		void addFileNodes(const QList<FileNode*>& files);
		void removeFileNodes(const QList<FileNode*>& files);

		void addFolderNodes(const QList<FolderNode*>& subFolders);
		void removeFolderNodes(const QList<FolderNode*>& subFolders);

		virtual bool addFile(const core::string& name, assetDb::AssetType::Enum type);
		virtual bool removeFile(const core::string& name, assetDb::AssetType::Enum type);

	protected:
		QList<FolderNode*> subFolderNodes_;
		QList<FileNode*> fileNodes_;

	private:
		// managed by ProjectNode
		friend class ProjectNode;
		QString displayName_;
		mutable QIcon iconexpanded_;
	};


	class VirtualFolderNode : public FolderNode
	{
		Q_OBJECT
	public:
		explicit VirtualFolderNode(const QString& name, int32_t priority);

		int32_t priority(void) const;
	private:
		int32_t priority_;
	};

	// virtual folder node for a asset type.
	class AssetTypeVirtualFolderNode : public VirtualFolderNode
	{
		Q_OBJECT

		typedef assetDb::AssetType AssetType;

	public:
		explicit AssetTypeVirtualFolderNode(const QString &name, int32_t priority, AssetType::Enum assType);

		AssetType::Enum assetType(void) const;

	private:
		AssetType::Enum assetType_;
	};


	class ProjectNode : public FolderNode
	{
		Q_OBJECT

	public:

		// all subFolders that are projects
		QList<ProjectNode*> subProjectNodes(void) const;

		// determines if the project will be shown in the flat view
		// TODO find a better name

		virtual bool canAddSubProject(const QString& projectName) const X_ABSTRACT;
		virtual bool addSubProjects(const QStringList& projectNames) X_ABSTRACT;
		virtual bool removeSubProjects(const QStringList& projectNames) X_ABSTRACT;

		virtual bool clean(ConverterHost& conHost) const X_ABSTRACT;


		QList<NodesWatcher*> watchers(void) const;
		void registerWatcher(NodesWatcher* pWatcher);
		void unregisterWatcher(NodesWatcher* pWatcher);

		bool isEnabled(void) const X_OVERRIDE;

		// to be called in implementation of
		// the corresponding public functions
		void addProjectNodes(const QList<ProjectNode*>& subProjects);
		void removeProjectNodes(const QList<ProjectNode*>& subProjects);

	protected:
		// this is just the in-memory representation, a subclass
		// will add the persistent stuff
		explicit ProjectNode(const QString& projectName);

	private slots:
		void watcherDestroyed(QObject* pWatcher);

	private:
		QList<ProjectNode*> subProjectNodes_;
		QList<NodesWatcher*> watchers_;

		// let SessionNode call setParentFolderNode
		friend class SessionNode;
	};



	class SessionNode : public FolderNode
	{
		Q_OBJECT
			friend class SessionManager;
	public:
		SessionNode(QObject* pParentObject);

		QList<ProjectAction> supportedActions(Node* pNode) const;

		QList<ProjectNode*> projectNodes(void) const;
		QList<NodesWatcher*> watchers(void) const;

		void registerWatcher(NodesWatcher* pWatcher);
		void unregisterWatcher(NodesWatcher* pWatcher);

		bool isEnabled(void) const X_OVERRIDE;

		bool build(ConverterHost& conHost, bool force) const X_OVERRIDE;

	protected:
		void addProjectNodes(const QList<ProjectNode*>& projectNodes);
		void removeProjectNodes(const QList<ProjectNode*>& projectNodes);

	private slots:
		void watcherDestroyed(QObject* pWatcher);

	private:
		QList<ProjectNode*> projectNodes_;
		QList<NodesWatcher*> watchers_;
	};


	class NodesWatcher : public QObject
	{
		Q_OBJECT
	public:
		explicit NodesWatcher(QObject *parent = 0);

	signals:
		// Emited whenever the model needs to send a update signal.
		void nodeUpdated(Node *node);

		// folders&  projects
		void foldersAboutToBeAdded(FolderNode *parentFolder,
			const QList<FolderNode*>& newFolders);
		void foldersAdded(void);

		void foldersAboutToBeRemoved(FolderNode *parentFolder,
			const QList<FolderNode*>& staleFolders);
		void foldersRemoved(void);

		// files
		void filesAboutToBeAdded(FolderNode *folder,
			const QList<FileNode*>& newFiles);
		void filesAdded(void);

		void filesAboutToBeRemoved(FolderNode *folder,
			const QList<FileNode*>& staleFiles);
		void filesRemoved(void);
		void nodeSortKeyAboutToChange(Node *node);
		void nodeSortKeyChanged(void);

	private:

		// let project&  session emit signals
		friend class ProjectNode;
		friend class FolderNode;
		friend class SessionNode;
		friend class Node;
	};



	template<class T1, class T3>
	X_INLINE bool isSorted(const T1 &list, T3 sorter)
	{
		typename T1::const_iterator it, iit, end;
		end = list.constEnd();
		it = list.constBegin();
		if (it == end) {
			return true;
		}

		iit = list.constBegin();
		++iit;

		while (iit != end) {
			if (!sorter(*it, *iit)) {
				return false;
			}
			it = iit++;
		}
		return true;
	}


	// compares old and new, and returns removed itemd and added items.
	template <class T1, class T2, class T3>
	X_INLINE void compareSortedLists(T1 oldList, T2 newList, T1 &removedList, T2 &addedList, T3 sorter)
	{
#if X_DEBUG
		Q_ASSERT(isSorted(oldList, sorter));
		Q_ASSERT(isSorted(newList, sorter));
#endif // !X_DEBUG

		typename T1::const_iterator oldIt, oldEnd;
		typename T2::const_iterator newIt, newEnd;

		oldIt = oldList.constBegin();
		oldEnd = oldList.constEnd();

		newIt = newList.constBegin();
		newEnd = newList.constEnd();

		addedList.reserve(newList.size());

		while (oldIt != oldEnd && newIt != newEnd) {
			if (sorter(*oldIt, *newIt)) {
				removedList.append(*oldIt);
				++oldIt;
			}
			else if (sorter(*newIt, *oldIt)) {
				addedList.append(*newIt);
				++newIt;
			}
			else {
				++oldIt;
				++newIt;
			}
		}

		while (oldIt != oldEnd) {
			removedList.append(*oldIt);
			++oldIt;
		}

		while (newIt != newEnd) {
			addedList.append(*newIt);
			++newIt;
		}
	}


} // namespace AssetExplorer

X_NAMESPACE_END

#endif // ASSETDBNODES_H
