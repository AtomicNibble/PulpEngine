#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)


class AssetDoubleSpinBoxWidget : public QWidget
{
	Q_OBJECT

public:
	AssetDoubleSpinBoxWidget(QWidget *parent, const std::string& val, double min, double max, double step);
	~AssetDoubleSpinBoxWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void valueChanged(double d);

private:
	QDoubleSpinBox* pSpinBox_;
};


class AssetSpinBoxWidget : public QWidget
{
	Q_OBJECT

public:
	AssetSpinBoxWidget(QWidget *parent, const std::string& val, double min, double max, double step);
	~AssetSpinBoxWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void valueChanged(int d);

private:
	QSpinBox* pSpinBox_;
};

X_NAMESPACE_END
