#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class AssetProperty;

class AssetGroupWidget : public QToolButton
{
	Q_OBJECT

	typedef QList<AssetProperty*> ChildrenVec;

	typedef ChildrenVec::ConstIterator ConstIterator;
	typedef ChildrenVec::Iterator Iterator;

public:
	AssetGroupWidget(QWidget *parent = nullptr);
	~AssetGroupWidget();

	void appendGui(QGridLayout* pLayout, int32_t& row, int32_t depth);

	void clear(void);
	void show(bool visible);

	void collapseAll(void);
	void expandAll(void);


	void AddChild(AssetProperty* pChild);
	ConstIterator begin(void) const;
	ConstIterator end(void) const;

private slots:
	void buttonClicked(bool);

private:
	ChildrenVec children_;
};


X_NAMESPACE_END