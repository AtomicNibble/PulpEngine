#pragma once


X_NAMESPACE_BEGIN(assman)

namespace AssetExplorer 
{
	namespace Constants
	{
		// Context
		const char C_ASSETDB_EXPLORER[] = "AssetDB Context";


		const char ADDNEWFILE[] = "ProjectExplorer.AddNewFile";


		const char OPENFILE[] = "ProjectExplorer.OpenFile";
		const char REMOVEFILE[] = "ProjectExplorer.RemoveFile";
		const char DELETEFILE[] = "ProjectExplorer.DeleteFile";
		const char RENAMEFILE[] = "ProjectExplorer.RenameFile";
		const char BUILD[] = "ProjectExplorer.Build";
		const char PROJECTTREE_COLLAPSE_ALL[] = "ProjectExplorer.CollapseAll";



		// Context menus
		const char M_PROJECTCONTEXT[] = "Project.Menu.Project";
		const char M_FOLDERCONTEXT[] = "Project.Menu.Folder";
		const char M_FILECONTEXT[] = "Project.Menu.File";

		// Context menu groups
		const char G_PROJECT_FIRST[] = "Project.Group.Open";
		const char G_PROJECT_FILES[] = "Project.Group.Files";
		const char G_PROJECT_LAST[] = "Project.Group.Last";
		const char G_PROJECT_TREE[] = "Project.Group.Tree";

		const char G_FOLDER_FILES[] = "ProjectFolder.Group.Files";
		const char G_FOLDER_OTHER[] = "ProjectFolder.Group.Other";
		const char G_FOLDER_CONFIG[] = "ProjectFolder.Group.Config";

		const char G_FILE_OPEN[] = "ProjectFile.Group.Open";
		const char G_FILE_OTHER[] = "ProjectFile.Group.Other";



	} // namespace Constants

} // namespace AssetExplorer

X_NAMESPACE_END