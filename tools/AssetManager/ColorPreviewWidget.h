#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)

class ColorPreview : public QWidget
{
	Q_OBJECT
		
public:
	enum class DisplayMode
	{
		NoAlpha,    ///< Show current color with no transparency
		AllAlpha,   ///< show current color with transparency
		SplitAlpha, ///< Show both solid and transparent side by side
		SplitColor  ///< Show current and comparison colors side by side
	};

public:
	explicit ColorPreview(QWidget *parent = nullptr);
	~ColorPreview();

	QBrush background(void) const;
	void setBackground(const QBrush &bk);

	DisplayMode displayMode(void) const;
	void setDisplayMode(DisplayMode dm);

	QColor color(void) const;
	QColor comparisonColor(void) const;
	QSize sizeHint(void) const;

private:
	void paint(QPainter& painter, QRect rect) const;

public slots:
	void setColor(const QColor& c);
	void setComparisonColor(const QColor& c);

signals:
	void clicked(void);
	void colorChanged(QColor);

protected:
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *);
	void mouseReleaseEvent(QMouseEvent *ev);
	void mouseMoveEvent(QMouseEvent *ev);

private:
	class Private;
	Private* const p;
};

X_NAMESPACE_END