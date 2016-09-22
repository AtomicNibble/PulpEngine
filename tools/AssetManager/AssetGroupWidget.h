#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class AssetGroupWidget : public QToolButton
{
	Q_OBJECT

public:
	AssetGroupWidget(QWidget *parent = nullptr);
	~AssetGroupWidget();

private slots:
	void buttonClicked(bool);

};


X_NAMESPACE_END