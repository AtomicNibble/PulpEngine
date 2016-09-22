#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class AssetTextWidget : public QWidget
{
	Q_OBJECT

public:
	AssetTextWidget(QWidget *parent, const std::string& value);
	~AssetTextWidget();


private slots:
	void textChanged(void);

private:


};


X_NAMESPACE_END