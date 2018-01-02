#pragma once

#include <QObject>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);


X_NAMESPACE_BEGIN(assman)

class IAssetEntry;
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

	void appendGui(assetDb::AssetDB& db, IAssetEntry* pAssEntry, QWidget* pParent, 
		QGridLayout* pLayout, int32_t& row, int32_t depth);

	void clear(void);
	void clearUI(void);
	void show(bool visible);

	void collapseAll(void);
	void expandAll(void);


	void AddChild(AssetProperty* pChild);
	bool HasChild(const AssetProperty& prop) const;
	bool RemoveChild(const AssetProperty& prop);
	ConstIterator begin(void) const;
	ConstIterator end(void) const;

private slots:
	void buttonClicked(bool);

private:
	ChildrenVec children_;
};


X_NAMESPACE_END