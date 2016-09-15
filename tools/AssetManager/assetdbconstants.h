#pragma once


X_NAMESPACE_BEGIN(assman)

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
		const char PROJECTTREE_COLLAPSE_ALL[] = "AssetDB.CollapseAll";



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