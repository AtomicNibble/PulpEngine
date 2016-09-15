#ifndef PCH_H
#define PCH_H




#include <Pulp/Common/EngineCommon.h>
#include <Pulp/Common/IAssetDb.h>

#include <Containers\Array.h>

#pragma comment(lib, "Dwrite")


#define QT_BEGIN_MOC_NAMESPACE X_NAMESPACE_BEGIN(assman)
#define QT_END_MOC_NAMESPACE X_NAMESPACE_END


extern core::MemoryArenaBase* g_arena;



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

#include "Constants.h"
#include "logging.h"
#include "assert_qt.h"
#include "id.h"
#include "IAssManCore.h"

#endif // PCH_H
