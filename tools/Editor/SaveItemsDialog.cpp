#include "stdafx.h"
#include "SaveItemsDialog.h"

#include "IAssetEntry.h"


X_NAMESPACE_BEGIN(editor)



SaveItemsDialog::SaveItemsDialog(QWidget *parent, QList<IAssetEntry *> items) :
	QDialog(parent),
	allItems_(items)
{
	setWindowTitle(tr("Save Items"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	pTreeWidget_ = new QTreeWidget();
//	pTreeWidget_->setHeaderHidden(true);
	pTreeWidget_->setExpandsOnDoubleClick(true);
	pTreeWidget_->setColumnCount(2);
	pTreeWidget_->setUniformRowHeights(true);
	pTreeWidget_->setItemsExpandable(true);
	pTreeWidget_->headerItem()->setText(0, "Name");
	pTreeWidget_->headerItem()->setText(1, "Type");
	pAlwaysSaveCheckbox_ = new QCheckBox();
	pAlwaysSaveCheckbox_->setText(tr("Always Save"));
	pMsgLabel_ = new QLabel();
	pMsgLabel_->setText(tr("The following items have unsaved changes:"));
	pButtonBox_ = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);

	QVBoxLayout* pLayout = new QVBoxLayout();
	pLayout->addWidget(pMsgLabel_);
	pLayout->addWidget(pTreeWidget_);
	pLayout->addWidget(pAlwaysSaveCheckbox_);
	pLayout->addWidget(pButtonBox_);
	setLayout(pLayout);

	const QDialogButtonBox::ButtonRole discardButtonRole = QDialogButtonBox::DestructiveRole;
	QPushButton *discardButton = pButtonBox_->addButton(tr("Do not Save"), discardButtonRole);
	pButtonBox_->button(QDialogButtonBox::Save)->setDefault(true);
	pTreeWidget_->setFocus();


	foreach(IAssetEntry* pAssetEntry, items)
	{
		QString fileName = pAssetEntry->name();
		QString type = assetDb::AssetType::ToString(pAssetEntry->type());

		type = type.toLower();

		QTreeWidgetItem* item = new QTreeWidgetItem(pTreeWidget_, QStringList() << fileName << type);

		item->setData(0, Qt::UserRole, qVariantFromValue(pAssetEntry));
	}

	pTreeWidget_->resizeColumnToContents(0);
	pTreeWidget_->selectAll();

	adjustButtonWidths();
	updateSaveButton();

	connect(pButtonBox_->button(QDialogButtonBox::Save), SIGNAL(clicked()), this, SLOT(collectItemsToSave()));
	connect(pButtonBox_, SIGNAL(rejected()), this, SLOT(reject()));

	connect(discardButton, SIGNAL(clicked()), this, SLOT(discardAll()));
	connect(pTreeWidget_, SIGNAL(itemSelectionChanged()), this, SLOT(updateSaveButton()));
}

SaveItemsDialog::~SaveItemsDialog()
{

}



void SaveItemsDialog::setMessage(const QString &msg)
{
	pMsgLabel_->setText(msg);
}

void SaveItemsDialog::updateSaveButton(void)
{
	const int32_t count = pTreeWidget_->selectedItems().count();
	QPushButton *button = pButtonBox_->button(QDialogButtonBox::Save);
	if (count == pTreeWidget_->topLevelItemCount()) {
		button->setEnabled(true);
		button->setText(tr("Save All"));
	}
	else if (count == 0) {
		button->setEnabled(true);
		button->setText(tr("Save All"));
	}
	else {
		button->setEnabled(true);
		button->setText(tr("Save Selected"));
	}
}

void SaveItemsDialog::adjustButtonWidths(void)
{
	QStringList possibleTexts;
	possibleTexts << tr("Save") << tr("Save All");

	if (pTreeWidget_->topLevelItemCount() > 1) {
		possibleTexts << tr("Save Selected");
	}

	int maxTextWidth = 0;
	QPushButton* saveButton = pButtonBox_->button(QDialogButtonBox::Save);
	foreach(const QString& text, possibleTexts)
	{
		saveButton->setText(text);
		const int32_t hint = saveButton->sizeHint().width();
		if (hint > maxTextWidth) {
			maxTextWidth = hint;
		}
	}

	saveButton->setMinimumWidth(maxTextWidth);
}

void SaveItemsDialog::collectItemsToSave(void)
{
	itemsToSave_.clear();

	const int32_t count = pTreeWidget_->selectedItems().count();
	if (count == 0) // save all
	{
		itemsToSave_ = allItems_;
	}
	else
	{
		foreach(QTreeWidgetItem* pItem, pTreeWidget_->selectedItems()) {
			itemsToSave_.append(pItem->data(0, Qt::UserRole).value<IAssetEntry*>());
		}
	}

	accept();
}

void SaveItemsDialog::discardAll(void)
{
	pTreeWidget_->clearSelection();
	collectItemsToSave();
}

QList<IAssetEntry*> SaveItemsDialog::itemsToSave(void) const
{
	return itemsToSave_;
}

void SaveItemsDialog::setAlwaysSaveMessage(const QString &msg)
{
	pAlwaysSaveCheckbox_->setText(msg);
	pAlwaysSaveCheckbox_->setVisible(true);
}

bool SaveItemsDialog::alwaysSaveChecked(void)
{
	return pAlwaysSaveCheckbox_->isChecked();
}



X_NAMESPACE_END

