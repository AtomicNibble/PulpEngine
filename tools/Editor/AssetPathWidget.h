#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class AssetPathWidget : public QWidget
{
	Q_OBJECT

	enum class Kind {
		ExistingDirectory,
		Directory,			// A directory, doesn't need to exist
		File,
		SaveFile,
		Any
	};


public:
	AssetPathWidget(QWidget *parent, const std::string& value);
	~AssetPathWidget();

	void setExpectedKind(Kind expected);
	Kind expectedKind(void) const;

	void setPromptDialogTitle(const QString& title);
	QString promptDialogTitle(void) const;

	void setPromptDialogFilter(const QString& filter);
	QString promptDialogFilter(void) const;

	//  backup apth for when current value is missing from OS.
	void setInitialBrowsePathBackup(const QString& path);

	QString baseDirectory(void) const;
	void setBaseDirectory(const QString& directory);
	QString baseFileName(void) const;
	void setBaseFileName(const QString& base);

	QString path(void) const;
	QString rawPath(void) const; // The raw unexpanded input.
	QString fileName(void) const;

signals:
	void valueChanged(const std::string& value);

private slots:
	void editingFinished(void);
	void setValue(const std::string& value);

private:
	void setPath(const QString& filePath);
	void setFileName(const QString& fileName);

	QString expandedPath(const QString &path) const;

	// Returns overridden title or the one from <title>
	QString makeDialogTitle(const QString& title);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent) X_OVERRIDE;
	void dragEnterEvent(QDragEnterEvent *event) X_OVERRIDE;
	void dropEvent(QDropEvent* event) X_OVERRIDE;

private slots:
	void browseClicked(void);

private:
	QLineEdit* pLineEdit_;

	Kind acceptingKind_;
	QString dialogTitleOverride_;
	QString dialogFilter_;
	QString initialBrowsePathOverride_;
	QString baseDirectory_;
};

X_NAMESPACE_END