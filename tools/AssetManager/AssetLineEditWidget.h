#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class AssetLineEditWidget : public QLineEdit
{
	Q_OBJECT

public:
	AssetLineEditWidget(QWidget *parent = nullptr);
	~AssetLineEditWidget();
};

X_NAMESPACE_END