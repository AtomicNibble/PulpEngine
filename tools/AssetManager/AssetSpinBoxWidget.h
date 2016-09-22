#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)


class AssetDoubleSpinBoxWidget : public QWidget
{
	Q_OBJECT

public:
	AssetDoubleSpinBoxWidget(QWidget *parent, double val, double min, double max, double step);
	~AssetDoubleSpinBoxWidget();

private slots:
	void valueChanged(double d);

};


class AssetSpinBoxWidget : public QWidget
{
	Q_OBJECT

public:
	AssetSpinBoxWidget(QWidget *parent, int32_t val, double min, double max, double step);
	~AssetSpinBoxWidget();

private slots:
	void valueChanged(int32_t d);

};

X_NAMESPACE_END
