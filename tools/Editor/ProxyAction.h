#pragma once

#include <QObject>
#include <QPointer>
#include <QAction>

X_NAMESPACE_BEGIN(editor)


class ProxyAction : public QAction
{
	Q_OBJECT
public:
	enum Attribute {
		Hide = 0x01,
		UpdateText = 0x02,
		UpdateIcon = 0x04
	};
	Q_DECLARE_FLAGS(Attributes, Attribute)

public:
	explicit ProxyAction(QObject *parent = nullptr);


	void initialize(QAction* pAction);

	void setAction(QAction* pAction);
	QAction* action(void) const;

	bool shortcutVisibleInToolTip(void) const;
	void setShortcutVisibleInToolTip(bool visible);

	void setAttribute(Attribute attribute);
	void removeAttribute(Attribute attribute);
	bool hasAttribute(Attribute attribute);

	static QString stringWithAppendedShortcut(const QString& str, const QKeySequence& shortcut);

private slots:
	void actionChanged(void);
	void updateState(void);
	void updateToolTipWithKeySequence(void);

private:
	void disconnectAction(void);
	void connectAction(void);
	void update(QAction* pAction, bool initialize);

private:
	QPointer<QAction> action_;
	Attributes attributes_;
	bool showShortcut_;
	QString toolTip_;
	bool block_;
};


X_NAMESPACE_END