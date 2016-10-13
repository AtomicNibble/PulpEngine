#pragma once

#include <QObject>

#include "RawFileLoader.h"

X_NAMESPACE_BEGIN(assman)

class IAssetEntry;

class AssetModelWidget : public QWidget
{
	Q_OBJECT

public:
	AssetModelWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value);
	~AssetModelWidget();

	void setPromptDialogTitle(const QString& title);
	QString promptDialogTitle(void) const;

	void setPromptDialogFilter(const QString& filter);
	QString promptDialogFilter(void) const;

	QString path(void) const;

private:
	void setPath(const QString& filePath);
	void setErrorTip(const QString& tip);
	void removeErrorTip(void);
	bool loadRawModel(void);

private slots:
	void setValue(const std::string& value);
	void editingFinished(void);
	void validatePath(void);
	void browseClicked(void);

	void setProgress(int32_t pro);
	void setProgressLabel(const QString& label, int32_t pro);
	void rawFileLoaded(void);

signals:
	void valueChanged(const std::string& value);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent) X_OVERRIDE;
	void dragEnterEvent(QDragEnterEvent* pEvent) X_OVERRIDE;
	void dropEvent(QDropEvent* pEvent) X_OVERRIDE;

private:
	QString makeDialogTitle(const QString& title);
	static bool fileExtensionValid(const QString& path);


private:
	IAssetEntry* pAssEntry_;

private:
	RawFileLoader loader_;
	QProgressDialog* pProgress_;

private:
	QLineEdit* pEditPath_;
	QAction* pWarningAction_;

	QString curPath_;
	QString dialogFilter_;
	QString dialogTitleOverride_;
	QString initialBrowsePathOverride_;
};


X_NAMESPACE_END