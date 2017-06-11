#pragma once

#include <QObject>

#include "RawFileLoader.h"

X_NAMESPACE_BEGIN(assman)

class IAssetEntry;

class AssetTextureWidget : public QWidget
{
	Q_OBJECT

public:
	AssetTextureWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value);
	~AssetTextureWidget();


	void setPromptDialogTitle(const QString& title);
	QString promptDialogTitle(void) const;

	void setPromptDialogFilter(const QString& filter);
	QString promptDialogFilter(void) const;

private:
	void showPreviewWidgets(bool vis);
	void loadFile(const QString& filePath);
	QString makeDialogTitle(const QString& title);
	static bool fileExtensionValid(const QString& paht);
	bool loadCurrentToPix(QPixmap& pixOut);
	void setPreviewPix(QPixmap& pix);
	bool loadImage(const QString& path);
	void showError(const QString& msg);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent) X_OVERRIDE;
	void dragEnterEvent(QDragEnterEvent* pEvent) X_OVERRIDE;
	void dropEvent(QDropEvent* pEvent) X_OVERRIDE;

signals:
	void valueChanged(const std::string& value);

private slots:
	void loadPreview(void);
	void showTextureDialog(void);
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
	QLabel* pPreview_;
	QToolButton* pZoom_;
	QLabel* pDropZone_; 
	QLabel* pEditDimensions_;

	QString dialogFilter_;
	QString dialogTitleOverride_;
	QString initialBrowsePathOverride_;
};

X_NAMESPACE_END