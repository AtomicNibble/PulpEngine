#pragma once

#include <QObject>

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

	QString path(void) const;

private:
	void showPreviewWidgets(bool vis);
	void setPath(const QString& filePath);
	QString makeDialogTitle(const QString& title);
	static bool fileExtensionValid(const QString& paht);
	bool getPixMapCurPath(QPixmap& pixOut);
	bool loadImage(void);
	void setErrorTip(const QString& tip);
	void removeErrorTip(void);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent) X_OVERRIDE;
	void dragEnterEvent(QDragEnterEvent* pEvent) X_OVERRIDE;
	void dropEvent(QDropEvent* pEvent) X_OVERRIDE;

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void editingFinished(void);
	void validatePath(void);
	void showTextureDialog(void);
	void browseClicked(void);

private:
	IAssetEntry* pAssEntry_;

private:
	QLabel* pPreview_;
	QToolButton* pZoom_;
	QLabel* pEditPathPad_; // used for layout currently.
	QLineEdit* pEditPath_;
	QLabel* pEditDimensions_;
	QAction* pWarningAction_;

	QString curPath_;
	QString dialogFilter_;
	QString dialogTitleOverride_;
	QString initialBrowsePathOverride_;
};

X_NAMESPACE_END