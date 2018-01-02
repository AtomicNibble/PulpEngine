#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(editor)

class AssetCheckBoxWidget : public QCheckBox
{
	Q_OBJECT

public:
	AssetCheckBoxWidget(QWidget *parent, const std::string& val);
	~AssetCheckBoxWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void toggled(bool checked);

private:
};


X_NAMESPACE_END