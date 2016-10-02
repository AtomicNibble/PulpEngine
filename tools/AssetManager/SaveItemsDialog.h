#pragma once

#include <QDialog>

X_NAMESPACE_BEGIN(assman)

class IAssetEntry;
class EditorManager;


class SaveItemsDialog : public QDialog
{
	Q_OBJECT

public:
	SaveItemsDialog(QWidget *parent, QList<IAssetEntry*> items);
	~SaveItemsDialog();

	void setMessage(const QString &msg);
	void setAlwaysSaveMessage(const QString &msg);
	bool alwaysSaveChecked(void);
	QList<IAssetEntry*> itemsToSave(void) const;

private slots:
	void collectItemsToSave(void);
	void discardAll(void);
	void updateSaveButton(void);

private:
	void adjustButtonWidths(void);

private:
	QList<IAssetEntry*> itemsToSave_;
	QList<IAssetEntry*> allItems_;
private:
	QTreeWidget* pTreeWidget_;
	QCheckBox* pAlwaysSaveCheckbox_;
	QLabel* pMsgLabel_;
	QDialogButtonBox* pButtonBox_;
};


X_NAMESPACE_END

