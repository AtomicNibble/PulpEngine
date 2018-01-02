#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)


// had to place this here since a class with Q_OBJECT in cpp makes qt plugin fuck shit up.
class StringValidator : public QValidator
{
	Q_OBJECT
public:
	explicit StringValidator(QObject *parent = nullptr);
	virtual State validate(QString& input, int& pos) const X_OVERRIDE;
};


class AssetStringWidget : public QLineEdit
{
	Q_OBJECT

public:
	AssetStringWidget(QWidget *parent, const std::string& value);
	~AssetStringWidget();

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void editingFinished(void);

};


X_NAMESPACE_END