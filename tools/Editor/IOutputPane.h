#pragma once


#include <QObject>
#include <QList>
#include <QString>

class QWidget;

X_NAMESPACE_BEGIN(editor)

class IOutputPane : public QObject
{
	Q_OBJECT

public:
	enum Flag { 
		NoModeSwitch = 0, 
		ModeSwitch = 1,
		WithFocus = 2,
		EnsureSizeHint = 4
	};

	Q_DECLARE_FLAGS(Flags, Flag)

public:
	IOutputPane(QObject *parent = 0);

	virtual QWidget *outputWidget(QWidget *parent) X_ABSTRACT;
	virtual QList<QWidget *> toolBarWidgets(void) const X_ABSTRACT;
	virtual QString displayName(void) const X_ABSTRACT;

	virtual void clearContents(void) X_ABSTRACT;
	virtual void visibilityChanged(bool visible) X_ABSTRACT;

	virtual void setFocus(void) X_ABSTRACT;
	virtual bool hasFocus(void) const X_ABSTRACT;
	virtual bool canFocus(void) const X_ABSTRACT;

	X_INLINE virtual bool canNext(void) const;
	X_INLINE virtual bool canPrevious(void) const;
	X_INLINE virtual void goToNext(void);
	X_INLINE virtual void goToPrev(void);

public slots:
	X_INLINE void popup(int32_t flags);
	X_INLINE void hide(void);
	X_INLINE void toggle(int32_t flags);
	X_INLINE void navigateStateChanged(void);
	X_INLINE void flash(void);

signals:
	void showPage(int32_t flags);
	void hidePage(void);
	void togglePage(int32_t flags);
	void navigateStateUpdate(void);
	void flashButton(void);
};


X_INLINE IOutputPane::IOutputPane(QObject *parent) :
	QObject(parent) 
{

}


X_INLINE bool IOutputPane::canNext(void) const
{
	return false;
}

X_INLINE bool IOutputPane::canPrevious(void) const
{
	return false;
}

X_INLINE void IOutputPane::goToNext(void)
{

}

X_INLINE void IOutputPane::goToPrev(void)
{

}


void IOutputPane::popup(int32_t flags)
{ 
	emit showPage(flags);
}

void IOutputPane::hide(void) 
{ 
	emit hidePage(); 
}

void IOutputPane::toggle(int32_t flags)
{ 
	emit togglePage(flags); 
}

void IOutputPane::navigateStateChanged(void) 
{ 
	emit navigateStateUpdate(); 
}

void IOutputPane::flash(void) 
{ 
	emit flashButton(); 
}


X_NAMESPACE_END