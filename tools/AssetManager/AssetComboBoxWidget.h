#pragma once

#include <QObject>
#include <QComboBox>


X_NAMESPACE_BEGIN(assman)


class AssetComboBoxWidget : public QComboBox
{
	Q_OBJECT

public:
	AssetComboBoxWidget(QWidget *parent, const std::string& value, bool editable = false);
	~AssetComboBoxWidget();

private slots:
	void currentIndexChanged(int32_t index);
	void returnPressed(void);

};


X_NAMESPACE_END