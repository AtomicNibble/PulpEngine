#pragma once

#include <QWidget>

X_NAMESPACE_BEGIN(assman)


class StyledBar : public QWidget
{
	Q_OBJECT
public:
	StyledBar(QWidget *parent = 0);
	void setSingleRow(bool singleRow);
	bool isSingleRow(void) const;

	void setLightColored(bool lightColored);
	bool isLightColored(void) const;

protected:
	void paintEvent(QPaintEvent *event);
};

class StyledSeparator : public QWidget
{
	Q_OBJECT
public:
	StyledSeparator(QWidget *parent = 0);

protected:
	void paintEvent(QPaintEvent *event);
};


X_NAMESPACE_END