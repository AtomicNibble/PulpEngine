#pragma once

#include <QObject>

#include "DirectionConstants.h"

X_NAMESPACE_BEGIN(editor)


class BaseWindow;
class CustomTabWidget;
class SplitterOrView;
class IEditor;
class IAssetEntry;




class DiamondBut : public QLabel
{
	Q_OBJECT
public:
	DiamondBut(QWidget* parent = 0);

	void setImage(QString filename);
	void setActive(bool b);

protected:
	QPixmap img_;
	QPixmap hover_;
	bool    active_;
};


class Overlay : public QWidget
{
public:
	enum class CurButton {
		Top,
		Left,
		Right,
		Bottom,
		Center,
		Invalid = 0xff
	};

public:
	Overlay(QWidget * parent = 0);

	void updatePos(const QRect& rect);

	CurButton getActiveButton(void);

protected:
	void paintEvent(QPaintEvent *);

private:
	QPixmap img_;

	CurButton   curButton_;
	CurButton   lastButton_;
	DiamondBut* buttons_[5];
};



class EditorView : public QWidget
{
	Q_OBJECT

private:
	friend class SplitterOrView; // for setParentSplitterOrView
	friend class Codetabview;

public:
	explicit EditorView(SplitterOrView* pParentSplitterOrView, QWidget *parent = 0);
	virtual ~EditorView();

	SplitterOrView* parentSplitterOrView(void) const;
	EditorView* findNextView(void);

	int32_t editorCount(void) const;
	void addEditor(IEditor* pEditor);
	void removeEditor(IEditor* pEditor);
	IEditor* currentEditor(void) const;
	void setCurrentEditor(IEditor* pEditor);

	bool hasEditor(IEditor* pEditor) const;

	QList<IEditor*> editors(void) const;
	IEditor* editorForAssetEntry(const IAssetEntry* pAssetEntry) const;

	IEditor* tabWidgetToEditor(QWidget* pWidget);

protected:
	void mousePressEvent(QMouseEvent* e);
	void focusInEvent(QFocusEvent* e);

private slots:
	void closeView(void);
	void splitHorizontally(void);
	void splitVertically(void);
	void splitNewWindow(void);
	void closeSplit(void);

	void showContextMenu(const QPoint& point);
	void tabCloseRequested(int index);

	void titleChanged(QString title);

private:
	void setParentSplitterOrView(SplitterOrView* pSplitterOrView);

private:
	SplitterOrView* pParentSplitterOrView_;

	CustomTabWidget*  pContainer_;
	QList<IEditor*> editors_;
	QMap<QWidget*, IEditor*> widgetEditorMap_;
};



class SplitterOrView : public QWidget
{
	Q_OBJECT


public:
	explicit SplitterOrView(BaseWindow *window);
	explicit SplitterOrView(IEditor* pEditor = nullptr);
	explicit SplitterOrView(EditorView* pView);
	~SplitterOrView();

	void split(Qt::Orientation orientation);
	void unsplit(void);
	void unsplit(SplitterOrView* pViewToRemove);

	void moveEditor(Qt::Orientation orientation, IEditor* pEditor, Direction side = Direction::Default);
	void addSplitter(SplitterOrView* pSplit, Qt::Orientation orientation, Direction side = Direction::Default);

	X_INLINE bool isView(void) const;
	X_INLINE bool isSplitter(void) const;
	X_INLINE bool isFloated(void) const;
	X_INLINE bool isDrag(void) const;

	X_INLINE IEditor* editor(void) const;
	X_INLINE QList<IEditor*> editors(void) const;
	X_INLINE bool hasEditor(IEditor *editor) const;
	X_INLINE bool hasEditors(void) const;
	X_INLINE EditorView* view(void) const;
	X_INLINE QSplitter* splitter(void) const;
	QSplitter* takeSplitter(void);
	EditorView* takeView(void);

	EditorView* findFirstView(void);
	SplitterOrView* findParentSplitter(void) const;

	X_INLINE QSize sizeHint(void) const;
	QSize minimumSizeHint(void) const;

	void unsplitAll(void);

	void SetDragMode(QPoint& pos);
	void dragMakeNewWindow(void);

protected:
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

private:
	void unsplitAllHelper(void);

private:
	QStackedLayout* pLayout_;
	EditorView* pView_;
	QSplitter* pSplitter_;

	QPoint tabOffset_;

	bool dragging_;
	bool floated_;
};


X_NAMESPACE_END

#include "EditorView.inl"