#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)


class AssetPathWidget : public QWidget
{
	Q_OBJECT

public:
	AssetPathWidget(QWidget *parent, const std::string& value);
	~AssetPathWidget();

private slots:
	void browseClicked(void);

private:

};

X_NAMESPACE_END