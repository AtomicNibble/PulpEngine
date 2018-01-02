#ifndef PCH_H
#define PCH_H




#include <Pulp/Common/EngineCommon.h>
#include <Pulp/Common/IAssetDb.h>

#include <Containers\Array.h>



#define QT_BEGIN_MOC_NAMESPACE X_NAMESPACE_BEGIN(editor)
#define QT_END_MOC_NAMESPACE X_NAMESPACE_END


extern core::MemoryArenaBase* g_arena;

// ----------------------------

#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#define AS_PROCESS_METADATA 0
#include <angelscript\include\angelscript.h>

#if X_DEBUG
X_LINK_LIB("angelscriptd");
#else
X_LINK_LIB("angelscript");
#endif // !X_DEBUG

// ----------------------------

// qt wants this..
#pragma comment(lib, "Dwrite")

// ----------------------------

#if X_DEBUG
// #define QT_NO_DEBUG_OUTPUT
// #define QT_NO_INFO_OUTPUT
// #define QT_NO_WARNING_OUTPUT
#elif X_RELEASE
#define QT_NO_DEBUG_OUTPUT
#define QT_NO_INFO_OUTPUT
#define QT_NO_WARNING_OUTPUT
#elif X_SUPER
#define QT_NO_DEBUG_OUTPUT
#define QT_NO_INFO_OUTPUT
#define QT_NO_WARNING_OUTPUT
#elif 
#error "unknown config."
#endif

#define CONTEXT_DEBUGGING 0


#include <QIcon>
#include <QObject>
#include <QStringList>
#include <QDebug>
#include <QMainWindow>

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QSet>
#include <QHash>
#include <QWidget>
#include <QTreeView>
#include <QDir>
#include <QMap>
#include <QHash>
#include <QMultiHash>


#include <QtWidgets>
#include <QColor.h>
#include <QGridLayout>



#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QPointer>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QClipboard>

#include <array>
#include <algorithm>

#include "StrUtil.h"
#include "Constants.h"
#include "logging.h"
#include "assert_qt.h"
#include "id.h"
#include "IAssManCore.h"



#endif // PCH_H
