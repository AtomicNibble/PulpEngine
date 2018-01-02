#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class AssetLineEditWidget : public QLineEdit
{
	Q_OBJECT

public:
	AssetLineEditWidget(QWidget *parent, const std::string& value);
	~AssetLineEditWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void editingFinished(void);

};

X_NAMESPACE_END