#pragma once


X_NAMESPACE_BEGIN(editor)

namespace AssetExplorer 
{
	namespace Constants
	{
		// Context
		const char C_ASSETDB_EXPLORER[] = "AssetDB Context";


		const char NEW_ASSET[] = "AssetDB.NewAsset";
		const char NEW_ASSET_TYPE[] = "AssetDB.NewAssetType";
		const char OPEN_ASSET[] = "AssetDB.OpenAsset";
		const char DELETE_ASSET[] = "AssetDB.DeleteAsset";
		const char RENAME_ASSET[] = "AssetDB.RenameAsset";
		const char COPY_ASSET[] = "AssetDB.CopyAsset";
		const char COPY_ASSET_NAME[] = "AssetDB.CopyAsserName";
		const char CUT_ASSET[] = "AssetDB.CutAsset";
		const char PASTE_ASSET[] = "AssetDB.PasteAsset";
		const char BUILD[] = "AssetDB.Build";
		const char REBUILD[] = "AssetDB.ReBuild";
		const char CLEAN[] = "AssetDB.Clean";
		const char SETSTARTUP[] = "AssetDB.SetStartup";
		const char PROJECTTREE_COLLAPSE_ALL[] = "AssetDB.CollapseAll";
		const char PROJECTTREE_EXPAND_ALL[] = "AssetDB.ExpandAll";
		const char PROJECTTREE_EXPAND_BELOW[] = "AssetDB.ExpandBelow";




		// Context menus
		const char M_SESSIONCONTEXT[] = "Project.Menu.Session";
		const char M_PROJECTCONTEXT[] = "Project.Menu.Project";
		const char M_FOLDERCONTEXT[] = "Project.Menu.Folder";
		const char M_FILECONTEXT[] = "Project.Menu.File";

		// Context menu groups
		const char G_SESSION_FILES[] = "Session.Group.Files";
		const char G_SESSION_BUILD[] = "Session.Group.Build";
		const char G_SESSION_REBUILD[] = "Session.Group.Rebuild";

		const char G_PROJECT_FIRST[] = "Project.Group.Open";
		const char G_PROJECT_FILES[] = "Project.Group.Files";
		const char G_PROJECT_LAST[] = "Project.Group.Last";
		const char G_PROJECT_TREE[] = "Project.Group.Tree";

		const char G_FOLDER_FILES[] = "ProjectFolder.Group.Files";
		const char G_FOLDER_COMPILE[] = "ProjectFolder.Group.Compile";
		const char G_FOLDER_CONFIG[] = "ProjectFolder.Group.Config";


		const char G_FILE_OPEN[] = "ProjectFile.Group.Open";
		const char G_FILE_NEW[] = "ProjectFile.Group.New";
		const char G_FILE_COMPILE[] = "ProjectFile.Group.Compile";
		const char G_FILE_OTHER[] = "ProjectFile.Group.Other";



	} // namespace Constants

} // namespace AssetExplorer

X_NAMESPACE_END