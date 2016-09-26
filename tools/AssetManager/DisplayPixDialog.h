#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)


class DisplayPixDialog : public QDialog
{
	Q_OBJECT

public:
	DisplayPixDialog(QWidget *parent, QPixmap& pix);
	~DisplayPixDialog();

private slots:
	void accept(void);
	void reject(void);
	void done(int32_t val);

};


X_NAMESPACE_END