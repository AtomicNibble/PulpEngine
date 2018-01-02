#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(editor)


class AssetTextWidget : public QWidget
{
	Q_OBJECT

public:
	AssetTextWidget(QWidget *parent, const std::string& value);
	~AssetTextWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void textChanged(void);

private:
	QPlainTextEdit* pTextEdit_;
};


X_NAMESPACE_END