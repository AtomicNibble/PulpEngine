#pragma once

#include <QObject>

#include "RawFileLoader.h"

X_NAMESPACE_BEGIN(assman)

class IAssetEntry;

class AssetFontWidget : public QWidget
{
	Q_OBJECT

public:
	AssetFontWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value);
	~AssetFontWidget();


	void setPromptDialogTitle(const QString& title);
	QString promptDialogTitle(void) const;

	void setPromptDialogFilter(const QString& filter);
	QString promptDialogFilter(void) const;

private:
	void loadFile(const QString& filePath);
	QString makeDialogTitle(const QString& title);
	static bool fileExtensionValid(const QString& paht);
	bool loadFont(const QString& path);
	void showError(const QString& msg);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent) X_OVERRIDE;
	void dragEnterEvent(QDragEnterEvent* pEvent) X_OVERRIDE;
	void dropEvent(QDropEvent* pEvent) X_OVERRIDE;

signals:
	void valueChanged(const std::string& value);

private slots:
	void browseClicked(void);

	void setProgress(int32_t pro);
	void setProgressLabel(const QString& label, int32_t pro);
	void rawFileLoaded(void);

private:
	IAssetEntry* pAssEntry_;

private:
	RawFileLoader loader_;
	QProgressDialog* pProgress_;

private:
	QLabel* pDropZone_;

	QString dialogFilter_;
	QString dialogTitleOverride_;
	QString initialBrowsePathOverride_;
};

X_NAMESPACE_END