#include "stdafx.h"
#include "AssetGroupWidget.h"

#include "AssetScriptTypes.h"

X_NAMESPACE_BEGIN(editor)

AssetGroupWidget::AssetGroupWidget(QWidget *parent) :
	QToolButton(parent)
{

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setAutoRaise(true);
	setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
	setCheckable(true);

	auto f = font();
	f.setBold(true);
	setFont(f);

	{
		QPixmap collapsePix(":/misc/img/collapse.png");
		QPixmap expandPix(":/misc/img/expand.png");

		QIcon icon;
		icon.addPixmap(collapsePix, QIcon::Normal, QIcon::Off);
		icon.addPixmap(expandPix, QIcon::Normal, QIcon::On);

		setIcon(icon);
		setIconSize(QSize(12, 12));
	}


	connect(this, SIGNAL(clicked(bool)), this, SLOT(buttonClicked(bool)));

}

AssetGroupWidget::~AssetGroupWidget()
{
}

void AssetGroupWidget::appendGui(assetDb::AssetDB& db, IAssetEntry* pAssEntry, QWidget* pParent,
	QGridLayout* pLayout, int32_t& row, int32_t depth)
{
	for (auto& pChild : children_)
	{
		pChild->appendGui(db, pAssEntry, pParent, pLayout, row, depth + 1);
		row += 1;
	}
}

void AssetGroupWidget::clear(void)
{
	for (auto& pChild : children_)
	{
		pChild->clear();
	}

	children_.clear();
}

void AssetGroupWidget::clearUI(void)
{
	for (auto& pChild : children_)
	{
		pChild->clearUI();
	}

	children_.clear();
}

void AssetGroupWidget::show(bool visible)
{
	QToolButton::setVisible(visible);
	QToolButton::setChecked(false);

	for (auto& pChild : children_)
	{
		pChild->show(visible);
	}
}

void AssetGroupWidget::collapseAll(void)
{
	QToolButton::setChecked(true);

	for (auto& pChild : children_)
	{
		pChild->show(false);
	}
}

void AssetGroupWidget::expandAll(void)
{
	QToolButton::setChecked(false);

	for (auto& pChild : children_)
	{
		pChild->show(true);
	}
}


void AssetGroupWidget::AddChild(AssetProperty* pChild)
{
	return children_.append(pChild);
}

bool AssetGroupWidget::HasChild(const AssetProperty& prop) const
{
	for (const auto& p : children_)
	{
		if (p == &prop)
		{
			return true;
		}
	}

	return false;
}

bool AssetGroupWidget::RemoveChild(const AssetProperty& prop)
{
	for (int32_t i=0; i<children_.size(); i++)
	{
		if (children_[i] == &prop)
		{
			children_.removeAt(i);
			return true;
		}
	}

	return false;
}


AssetGroupWidget::ConstIterator AssetGroupWidget::begin(void) const
{
	return children_.begin();
}

AssetGroupWidget::ConstIterator AssetGroupWidget::end(void) const
{
	return children_.end();
}



void AssetGroupWidget::buttonClicked(bool checked)
{
	for (auto& pChild : children_)
	{
		pChild->show(!checked);
	}
}

X_NAMESPACE_END