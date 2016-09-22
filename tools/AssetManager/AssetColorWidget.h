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
	ColorSelector* pColPreview_;
	QLineEdit* pRGBAValueWidgets_[4];
};

X_NAMESPACE_END