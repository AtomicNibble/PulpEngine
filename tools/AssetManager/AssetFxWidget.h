#pragma once

#include <QObject>
#include <QtCharts/QChartView.h>

QT_BEGIN_NAMESPACE

namespace QtCharts
{
	class QLineSeries;
	class QChart;
	class QValueAxis;

} // namespace QtCharts

QT_END_NAMESPACE

X_NAMESPACE_BEGIN(assman)

class IContext;
class IAssetEntry;

class AssetFxWidget : public QWidget
{
	Q_OBJECT

public:
	AssetFxWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value);
	~AssetFxWidget();

private:

private slots:
	void setValue(const std::string& value);
	
signals:
	void valueChanged(const std::string& value);

private:
	IAssetEntry* pAssEntry_;

private:

};



class SpinBoxRange : public QWidget
{
	Q_OBJECT
public:
	SpinBoxRange(QWidget* parent = nullptr);


private:
	QSpinBox * pStart_;
	QSpinBox* pRange_;
};



class GraphEditorView : public QtCharts::QChartView
{
	Q_OBJECT


private:
	static constexpr Qt::PenStyle DisabledPenStyle = Qt::PenStyle::DotLine;
	static constexpr int DarkenFactor = 250;
	static constexpr int LineWidth = 2;

	typedef std::vector<QString> StringArr;
	typedef std::vector<QColor> ColorArr;
	typedef std::vector<QtCharts::QLineSeries*> QLineSeriesArr;

	// A graph can have multiple series 
	// but it's treated as a single graph.
	struct Graph
	{
		QLineSeriesArr series;
	};

	typedef std::vector<Graph> GraphArr;

private:
	class ResetGraph : public QUndoCommand
	{
		typedef QVector<QPointF> PointArr;
		typedef std::vector<PointArr> PointArrArr;


	public:
		ResetGraph(Graph& graph);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;

	private:
		Graph& graph_;
		PointArrArr graphPoints_;
	};

	class ClearPoints : public QUndoCommand
	{
	public:
		ClearPoints(QtCharts::QLineSeries* pSeries);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;

	private:
		QtCharts::QLineSeries* pSeries_;
		QVector<QPointF> points_;
	};

	class AddPoint : public QUndoCommand
	{
	public:
		AddPoint(Graph& graph, int32_t index, QPointF point);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;

	private:
		Graph& graph_;
		QPointF point_;
		int32_t index_;
	};

	class MovePoint : public QUndoCommand
	{
	public:
		MovePoint(Graph& graph, int32_t activeSeries, int32_t index, QPointF delta);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;
		int id(void) const X_FINAL;
		bool mergeWith(const QUndoCommand* pOth) X_FINAL;

	private:
		Graph& graph_;
		QPointF delta_;
		int32_t index_;
		int32_t activeSeries_;
	};


public:
	GraphEditorView(QWidget *parent = nullptr);

	void createGraphs(int32_t numGraphs, int32_t numSeries);
	void setSeriesName(int32_t i, const QString& name);
	void setSeriesColor(int32_t i, const QColor& col);

	void setSeriesActive(int32_t seriesIdx);
	void setGraphActive(int32_t graphIdx);

private:
	void addSeriesToChart(QtCharts::QLineSeries* pSeries);


private:
	void mouseMoveEvent(QMouseEvent *event) X_OVERRIDE;
	void mousePressEvent(QMouseEvent *event) X_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *event) X_OVERRIDE;

	void leaveEvent(QEvent *event) X_OVERRIDE;

private slots:
	void seriesHover(const QPointF &point, bool state);
	void showContextMenu(const QPoint &pos);
	
	void setActiveSeries(void);
	void setActiveGraph(void);

	void clearKnots(void);
	void resetKnots(void);

private:
	bool isDraggingPoint(void) const;

	QtCharts::QLineSeries* getSeries(int32_t graphIdx, int32_t seriesIdx) const;
	QtCharts::QLineSeries* activeSeries(void) const;
	Graph& activeGraph(void);

private:
	IContext* pContext_;
	QUndoStack* pUndoStack_;
	QAction* pUndoAction_;
	QAction* pRedoAction_;
	QtCharts::QChart* pChart_;
	QtCharts::QValueAxis* pAxisX_;
	QtCharts::QValueAxis* pAxisY_;

	bool mouseActive_;
	bool _pad;

	int32_t activePoint_;
	int32_t activeSeries_; // sticky acroos graphs.
	int32_t activeGraph_;
	int32_t hoverSeries_;

	StringArr names_;
	ColorArr colors_;
	GraphArr graphs_;
};


class GraphEditor : public GraphEditorView
{
	Q_OBJECT

public:
	GraphEditor(int32_t numGraph, int32_t numSeries, QWidget *parent = nullptr);

};

class GradientWidget : public QWidget
{
	Q_OBJECT

public:
	GradientWidget(QWidget* parent = nullptr);

protected:
	void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

};

class ColorGraphEditor : public QWidget
{
	Q_OBJECT

public:
	ColorGraphEditor(int32_t numGraph, QWidget *parent = nullptr);

private:
	GraphEditorView* pGraph_;
	GradientWidget* pGradient_;
};

class GraphWithScale : public QGroupBox
{
	Q_OBJECT

public:
	GraphWithScale(const QString& label, QWidget *parent = nullptr);

	private slots:

private:
	GraphEditor * pSizeGraph_;
	QSpinBox* pScale_;
};


class SegmentList : public QWidget
{
	Q_OBJECT

public:
	SegmentList(QWidget *parent = nullptr);

private slots:
	void itemSelectionChanged(void);

	void addStageClicked(void);
	void deleteSelectedStageClicked(void);

private:
	QTableWidget* pTable_;
	QPushButton* pDelete_;
};


class SpawnInfo : public QGroupBox
{
	Q_OBJECT

public:
	SpawnInfo(QWidget *parent = nullptr);

private slots:

private:
	QRadioButton* pOneShot_;
	QRadioButton* pLooping_;
	SpinBoxRange* pCount_;
	QSpinBox* pInterval_;
	QSpinBox* pLoopCount_;
	SpinBoxRange* pLife_;
	SpinBoxRange* pDelay_;
};

class OriginInfo : public QGroupBox
{
	Q_OBJECT

public:
	OriginInfo(QWidget *parent = nullptr);

	private slots:

private:
	SpinBoxRange* pForward_;
	SpinBoxRange* pRight_;
	SpinBoxRange* pUp_;
};


class SequenceInfo : public QGroupBox
{
	Q_OBJECT

public:
	SequenceInfo(QWidget *parent = nullptr);

	private slots:

private:
	QSpinBox* pStart_;
	QSpinBox* pPlayRate_;
	QSpinBox* pLoopCount_;
};


class VelocityGraph : public QGroupBox
{
	Q_OBJECT

public:
	VelocityGraph(QWidget *parent = nullptr);

	private slots:

private:
	GraphEditor* pVelGraph_;
	QSpinBox* pForwardScale_;
	QSpinBox* pRightScale_;
	QSpinBox* pUpScale_;
};

class VelocityInfo : public QGroupBox
{
	Q_OBJECT

public:
	VelocityInfo(QWidget *parent = nullptr);

	private slots:

private:
	QRadioButton* pSpawn_;
	QRadioButton* pNow_;
	VelocityGraph* pGraph_;
};

class RotationGraph : public QGroupBox
{
	Q_OBJECT

public:
	RotationGraph(QWidget *parent = nullptr);

	private slots:

private:
	GraphEditor* pRotationGraph_;
	SpinBoxRange* pInitialRotation_;
};

class ColorGraph : public QGroupBox
{
	Q_OBJECT

public:
	ColorGraph(QWidget *parent = nullptr);

	private slots:

private:
	ColorGraphEditor* pColorGraph_;
};

class AlphaGraph : public QGroupBox
{
	Q_OBJECT

public:
	AlphaGraph(QWidget *parent = nullptr);

	private slots:

private:
	GraphEditor* pAlphaGraph_;
};


class VisualsInfo : public QGroupBox
{
	Q_OBJECT

public:
	VisualsInfo(QWidget *parent = nullptr);

	private slots:

private:
	QComboBox* pType_;
	QLineEdit* pMaterial_;
};



X_NAMESPACE_END