#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)


class AssetCheckBoxWidget : public QCheckBox
{
	Q_OBJECT

public:
	AssetCheckBoxWidget(QWidget *parent, const std::string& val);
	~AssetCheckBoxWidget();

private slots:
	void toggled(bool checked);
	

};


X_NAMESPACE_END