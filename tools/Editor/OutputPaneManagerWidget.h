#pragma once

#include <QObject>



X_NAMESPACE_BEGIN(assman)

class IOutputPane;

class OutputPanePlaceHolder : public QWidget
{
	Q_OBJECT
public:
	explicit OutputPanePlaceHolder(QSplitter *parent = 0);
	~OutputPanePlaceHolder();

	static OutputPanePlaceHolder* getCurrent(void);
	static bool isCurrentVisible(void);

	void unmaximize(void);
	bool isMaximized(void) const;
	void setDefaultHeight(int32_t height);
	void ensureSizeHintAsMinimum(void);

private slots:

private:
	bool canMaximizeOrMinimize(void) const;
	void maximizeOrMinimize(bool maximize);

private:
	QSplitter* pSplitter_;
	int32_t lastNonMaxSize_;
	static OutputPanePlaceHolder* pCurrent_;
};

class OutputPaneManager : public QWidget
{
	Q_OBJECT

public:
	explicit OutputPaneManager(QWidget *parent = nullptr);
	~OutputPaneManager();

	static void create(void);
	static void destroy(void);

public:
	void init(void);
	void addPane(IOutputPane* pPane);

	static OutputPaneManager* instance(void);

	void updateStatusButtons(bool visible);
	bool isMaximized(void) const;

private:
	void showPage(int32_t idx, int32_t flags);
	void ensurePageVisible(int32_t idx);
	int32_t findIndexForPage(IOutputPane *out);
	int32_t currentIndex(void) const;


public slots:
	void slotHide(void);
	void slotNext(void);
	void slotPrev(void);
	void shortcutTriggered(void);
	void slotMinMax(void);

protected:
	void focusInEvent(QFocusEvent *e);
	void resizeEvent(QResizeEvent *e);

private slots:
	void showPage(int32_t flags);
	void togglePage(int32_t flags);
	void clearPage(void);
	void updateNavigateState(void);
	void popupMenu(void);

	void setCurrentIndex(int idx);


private:
	QWidget* pToolBar_;
	QList<IOutputPane *> panes_;
	QVector<QAction *> actions_;
	QVector<Id> ids_;

	QStackedWidget* pOutputWidgetPane_;
	QComboBox* pOutputSlector_;

	int32_t outputPaneHeight_;
	bool maximised_;
};



X_NAMESPACE_END