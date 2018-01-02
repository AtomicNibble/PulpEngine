#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(editor)

class ColorSelector;

class AssetColorWidget : public QWidget
{
	Q_OBJECT

public:
	AssetColorWidget(QWidget *parent, const std::string& value);
	~AssetColorWidget();


private:
	void setColorInternal(QColor col, bool force = false);

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);

private slots:
	void colorSelected(const QColor& col);
	void colPickerClicked(void);
	void editingFinished(void);
	void validateText(const QString &);

protected:
	void mouseReleaseEvent(QMouseEvent *event) X_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) X_OVERRIDE;

private:
	static QColor getScreenColor(const QPoint& globalPos);

private:
	ColorSelector* pColPreview_;
	QLineEdit* pRGBAValueWidgets_[4];

	QColor curCol_;
	bool picking_;
};

X_NAMESPACE_END