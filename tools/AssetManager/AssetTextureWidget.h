#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class AssetTextureWidget : public QWidget
{
	Q_OBJECT

public:
	AssetTextureWidget(QWidget *parent);
	~AssetTextureWidget();

private slots:
	void browseClicked(void);

private:


};

X_NAMESPACE_END