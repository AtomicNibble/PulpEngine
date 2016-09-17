#pragma once


X_NAMESPACE_BEGIN(assman)


namespace Constants
{

	const char* const ASSMAN_VERSION_LONG = "1.0.0.0";
	const char* const ASSMAN_AUTHOR = "Tom Crowley";
	const char* const ASSMAN_YEAR = "2016";


	// Default groups
	const char G_DEFAULT_ONE[] = "Group.Default.One";
	const char G_DEFAULT_TWO[] = "Group.Default.Two";
	const char G_DEFAULT_THREE[] = "Group.Default.Three";


	const char C_GLOBAL[] = "AssMan Context";
	const char C_EDITORMANAGER[] = "EditorManager";

	// Menubar
	const char MENU_BAR[] = "MenuBar";

	// Menus
	const char M_FILE[] = "Menu.File";
	const char M_FILE_NEW[] = "Menu.File.New";
	const char M_FILE_OPEN[] = "Menu.File.Open";
	const char M_FILE_RECENTFILES[] = "Menu.File.RecentFiles";
	const char M_FILE_RECENTPROJECTS[] = "Menu.File.RecentProjects";
	const char M_EDIT[] = "Menu.Edit";
	const char M_EDIT_FIND[] = "Menu.Edit.Find";
	const char M_VIEW[] = "Menu.View";
	const char M_VIEW_TOOLBAR[] = "Menu.View.ToolBar";
	const char M_DEBUG[] = "Menu.Debug";
	const char M_TOOLS[] = "Menu.Tools";
	const char M_WINDOW[] = "Menu.Window";
	const char M_WINDOW_PANES[] = "Menu.Window.Panes";
	const char M_WINDOW_WINDOWS[] = "Menu.Window.Windows";
	const char M_HELP[] = "Menu.Help";


	// Main menu bar groups
	const char G_FILE[] = "Group.File";
	const char G_EDIT[] = "Group.Edit";
	const char G_VIEW[] = "Group.View";
	const char G_DEBUG[] = "Group.Debug";
	const char G_TOOLS[] = "Group.Tools";
	const char G_WINDOW[] = "Group.Window";
	const char G_HELP[] = "Group.Help";


	// File menu groups
	const char G_FILE_NEW[] = "Group.File.New";
	const char G_FILE_OPEN[] = "Group.File.Open";
	const char G_FILE_SAVE[] = "Group.File.Save";
	const char G_FILE_RECENT[] = "Group.File.Recent";
	const char G_FILE_CLOSE[] = "Group.File.Close";

	// Edit menu groups
	const char G_EDIT_UNDOREDO[] = "Group.Edit.UndoRedo";
	const char G_EDIT_COPYPASTE[] = "Group.Edit.CopyPaste";
	const char G_EDIT_SELECTALL[] = "Group.Edit.SelectAll";
	const char G_EDIT_FIND[] = "Group.Edit.Find";

	// View menu groups
	const char G_VIEW_CODE[] = "Group.View.Code";
	const char G_VIEW_DOCKED[] = "Group.View.Docked";
	const char G_VIEW_TOOLBARS[] = "Group.View.ToolBars";

	// Window menu groups
	const char G_WINDOW_LAYOUT[] = "Group.Window.Layout";
	const char G_WINDOW_SPLIT[] = "Group.Window.Split";
	const char G_WINDOW_DOCUMENT[] = "Group.Window.Documents";
	const char G_WINDOW_WINDOWS[] = "Group.Window.Windows";
	const char G_WINDOW_PANES[] = "Group.Window.Panes";

	// Help menu groups
	const char G_HELP_HELP[] = "Group.Help.Help";
	const char G_HELP_ABOUT[] = "Group.Help.About";


	//  ----------- Actions -----------
	
	// File Actions
	const char NEW_ASSET[] = "NewAsset";
	const char SAVE[] = "Save";
	const char SAVEAS[] = "SaveAs";
	const char SAVEALL[] = "SaveAll";
	const char EXIT[] = "Exit";


	// View Actions
	const char VIEW_CODE[] = "View.Code";
	const char VIEW_ASSETDBEXPLORER[] = "View.AssetDBExplorere";


	// Window Actions
	const char CLOSE_ALLDOCS[] = "Window.CloseAlldocs";
	const char RESET_LAYTOUT[] = "Window.ResetLayout";

	const char SPLIT[] = "Window.Split";
	const char SPLIT_SIDE_BY_SIDE[] = "Window.SplitSideBySide";
	const char SPLIT_NEW_WINDOW[] = "Window.SplitNewWindow";
	const char REMOVE_CURRENT_SPLIT[] = "Window.RemoveCurrentSplit";
	const char REMOVE_ALL_SPLITS[] = "Window.RemoveAllSplits";

	// Help Actions
	const char SHOW_ABOUT[] = "Help.About";



	//  ----------- Actions -----------


	// Icons
	const char ICON_LOGO_32[] = ":/misc/img/icon.png";
	const char ICON_LOGO_64[] = ":/misc/img/logo_64.png";
	const char ICON_LOGO_128[] = ":/misc/img/logo_128.png";
	const char ICON_LOGO_CRY_64[] = ":/misc/img/logo_cry_128.png";
	const char ICON_LOGO_CRY_128[] = ":/misc/img/logo_cry_128.png";

	// Editors
	const char ASSETPROP_EDITOR_ID[] = "AssetProp.Editor";
	const char C_ASSETPROP_EDITOR[] = "AssetPropEditor";
	const char C_ASSETPROP_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "AssetProp Editor");




} // namespace 


X_NAMESPACE_END