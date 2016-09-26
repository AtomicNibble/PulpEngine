#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class ColorSelector;

class AssetColorWidget : public QWidget
{
	Q_OBJECT

public:
	AssetColorWidget(QWidget *parent, const std::string& value);
	~AssetColorWidget();


private:
	void setColorInternal(QColor col);

private slots:
	void colPickerClicked(void);
	void editingFinished(void);

protected:
	void mouseReleaseEvent(QMouseEvent *event) X_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) X_OVERRIDE;

private:
	static QColor getScreenColor(const QPoint& globalPos);

private:
	ColorSelector* pColPreview_;
	QLineEdit* pRGBAValueWidgets_[4];

	bool picking_;
};

X_NAMESPACE_END