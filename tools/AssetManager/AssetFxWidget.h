#pragma once

#include <QObject>
#include <QtCharts/QChartView.h>

#include <IEffect.h>

#include <vector>

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


struct Range
{
	Range() :
		start(0),
		range(0)
	{
	}

	int32_t start;
	int32_t range;
};


#if 0
struct ColorPoint
{
	qreal pos;
	QColor col;
};

typedef std::vector<ColorPoint> ColorPointArr;
#endif

// GraphCollection:
//	Graph0:
//		Series0:
//		Series1:
//	Graph1:
//		Series0:
//		Series1:

struct GraphPoint
{
	GraphPoint() :
		GraphPoint(0.f, 0.f)
	{
	}

	GraphPoint(float32_t pos, float32_t val) :
		pos(pos),
		val(val)
	{
	}

	float32_t pos;
	float32_t val;
};

typedef std::vector<GraphPoint> GraphPointArr;

struct SeriesData
{
	GraphPointArr points;
};

typedef std::vector<SeriesData> SeriesDataArr;
#if 0

struct GraphData
{
	SeriesDataArr series;
};

typedef std::vector<GraphData> GraphDataArr;
#endif

struct GraphInfo
{
	SeriesDataArr graphs; // the series for each graph.
};

struct GrapScaleInfo : public GraphInfo
{
	GrapScaleInfo() : 
		GraphInfo(),
		scale(0)
	{

	}

	float scale;
};

struct RotationInfo
{
	GraphInfo rot;
	Range initial;
};

struct SizeInfo
{
	GrapScaleInfo size;
	GrapScaleInfo scale;
};

struct ColorInfo
{
	GraphInfo alpha;
	GraphInfo r;
	GraphInfo g;
	GraphInfo b;
};

struct VelocityInfo
{
	VelocityInfo() :
		postionType(engine::fx::RelativeTo::Spawn)
	{
	}

	engine::fx::RelativeTo::Enum postionType;

	GrapScaleInfo forward;
	GrapScaleInfo right;
	GrapScaleInfo up;
};


struct SpawnInfo
{
	SpawnInfo() :
		looping(false),
		interval(200),
		loopCount(0)
	{
		life.start = 1000;
	}

	bool looping;

	int32_t interval;	// how often we run, if loopcount > 1 we wait this time before next.
	int32_t loopCount;	// how many times we spawn before the end.

	Range count;		// 
	Range life;			// life for each elem.
	Range delay;		// delay is when we start this stage.
};

struct OriginInfo
{
	Range spawnOrgX;
	Range spawnOrgY;
	Range spawnOrgZ;
};

struct SequenceInfo
{
	SequenceInfo() :
		startFrame(0),
		fps(0),
		loop(0)
	{
	}

	int32_t startFrame; 
	int32_t fps;		
	int32_t loop;
};

struct VisualsInfo
{
	VisualsInfo() :
		type(engine::fx::StageType::BillboardSprite)
	{
	}

	QString material;
	engine::fx::StageType::Enum type;
};

struct Segment
{
	SpawnInfo spawn;
	OriginInfo origin;
	SequenceInfo seq;
	VisualsInfo vis;
	VelocityInfo vel;

	RotationInfo rot;
	SizeInfo size;
	ColorInfo col;
};


class SpinBoxRange : public QWidget
{
	Q_OBJECT
public:
	SpinBoxRange(QWidget* parent = nullptr);

	void setValue(const Range& r);
	void getValue(Range& r);

private:
	QSpinBox* pStart_;
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
		AddPoint(GraphEditorView* pView, Graph& graph, int32_t index, QPointF point);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;

	private:
		GraphEditorView* pView_;
		Graph& graph_;
		QPointF point_;
		int32_t index_;
	};

	class MovePoint : public QUndoCommand
	{
	public:
		MovePoint(GraphEditorView* pView, Graph& graph, int32_t activeSeries, int32_t index, QPointF delta);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;
		int id(void) const X_FINAL;
		bool mergeWith(const QUndoCommand* pOth) X_FINAL;

	private:
		GraphEditorView* pView_;
		Graph& graph_;
		QPointF delta_;
		int32_t index_;
		int32_t activeSeries_;
	};


public:
	GraphEditorView(QWidget *parent = nullptr);

	void setSeriesValue(int32_t idx, const GraphInfo& g);
	void getSeriesValue(int32_t idx, GraphInfo& g);

	void createGraphs(int32_t numGraphs, int32_t numSeries);
	void setSeriesName(int32_t i, const QString& name);
	void setSeriesColor(int32_t i, const QColor& col);
	void setSingleActiveSeries(bool value);
	void setXAxisRange(float min, float max);
	void setYAxisRange(float min, float max);

	void setSeriesActive(int32_t seriesIdx);
	void setGraphActive(int32_t graphIdx);


	const Graph& activeGraph(void) const;

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

signals:
	void pointsChanged(void);

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
	bool singleActiveSeries_;


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
	GraphEditor(QWidget *parent = nullptr);
	GraphEditor(int32_t numGraph, int32_t numSeries, QWidget *parent = nullptr);

};

class GradientWidget : public QWidget
{
	Q_OBJECT

public:
	struct ColorPoint
	{
		qreal pos;
		QColor col;
	};

	typedef std::vector<ColorPoint> ColorPointArr;

public:
	GradientWidget(QWidget* parent = nullptr);

public slots:
	void setColors(const ColorPointArr& colors);

protected:
	void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

private:
	ColorPointArr colors_;
};

class ColorGraphEditor : public QWidget
{
	Q_OBJECT

public:
	ColorGraphEditor(int32_t numGraph, QWidget *parent = nullptr);

	void setValue(const ColorInfo& col);
	void getValue(ColorInfo& col);

private slots:
	void updateColor(void);

private:
	GraphEditorView* pGraph_;
	GradientWidget* pGradient_;

	GradientWidget::ColorPointArr colors_;
};

class GraphWithScale : public QGroupBox
{
	Q_OBJECT

public:
	GraphWithScale(const QString& label, QWidget *parent = nullptr);

	void setValue(const GrapScaleInfo& g);
	void getValue(GrapScaleInfo& g);

private:
	GraphEditor* pGraph_;
	QDoubleSpinBox* pScale_;
};


class SegmentListWidget : public QWidget
{
	Q_OBJECT

public:
	SegmentListWidget(QWidget *parent = nullptr);
	
	int count(void) const;
	int currentRow(void) const;

signals:
	void selectionChanged(void);


private slots:
	void itemSelectionChanged(void);

	void addStageClicked(void);
	void deleteSelectedStageClicked(void);

private:
	QTableWidget* pTable_;
	QPushButton* pDelete_;
};


class SpawnInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	SpawnInfoWidget(QWidget *parent = nullptr);

	void setValue(const SpawnInfo& spawn);
	void getValue(SpawnInfo& spawn);


private:
	QRadioButton* pOneShot_;
	QRadioButton* pLooping_;
	SpinBoxRange* pCount_;
	QSpinBox* pInterval_;
	QSpinBox* pLoopCount_;
	SpinBoxRange* pLife_;
	SpinBoxRange* pDelay_;
};

class OriginInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	OriginInfoWidget(QWidget *parent = nullptr);

	void setValue(const OriginInfo& org);
	void getValue(OriginInfo& org);

private:
	SpinBoxRange* pForward_;
	SpinBoxRange* pRight_;
	SpinBoxRange* pUp_;
};


class SequenceInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	SequenceInfoWidget(QWidget *parent = nullptr);

	void setValue(const SequenceInfo& sq);
	void getValue(SequenceInfo& sq);

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

	void setValue(const VelocityInfo& vel);
	void getValue(VelocityInfo& vel);

private:
	GraphEditorView* pVelGraph_;
	QDoubleSpinBox* pForwardScale_;
	QDoubleSpinBox* pRightScale_;
	QDoubleSpinBox* pUpScale_;
};

class VelocityInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	VelocityInfoWidget(QWidget *parent = nullptr);

	void setValue(const VelocityInfo& vel);
	void getValue(VelocityInfo& vel);

private:
	QRadioButton* pSpawn_;
	QRadioButton* pNow_;
	VelocityGraph* pGraph_;
};

class RotationGraphWidget : public QGroupBox
{
	Q_OBJECT

public:
	RotationGraphWidget(QWidget *parent = nullptr);
	
	void setValue(const RotationInfo& rot);
	void getValue(RotationInfo& rot);

private:
	GraphEditor* pRotationGraph_;
	SpinBoxRange* pInitialRotation_;
};

class ColorGraph : public QGroupBox
{
	Q_OBJECT

public:
	ColorGraph(QWidget *parent = nullptr);

	void setValue(const ColorInfo& col);
	void getValue(ColorInfo& col);

private:
	ColorGraphEditor* pColorGraph_;
};

class AlphaGraph : public QGroupBox
{
	Q_OBJECT

public:
	AlphaGraph(QWidget *parent = nullptr);

	void setValue(const ColorInfo& col);
	void getValue(ColorInfo& col);

private:
	GraphEditor* pAlphaGraph_;
};


class VisualsInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	VisualsInfoWidget(QWidget *parent = nullptr);

	void setValue(const VisualsInfo& vis);
	void getValue(VisualsInfo& vis);

private:
	QComboBox* pType_;
	QLineEdit* pMaterial_;
};


class AssetFxWidget : public QWidget
{
	Q_OBJECT

	typedef std::unique_ptr<Segment> SegmentPtr;
	typedef std::vector<SegmentPtr> SegmentArr;

public:
	AssetFxWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value);
	~AssetFxWidget();

private:

	void enableWidgets(bool enable);

private slots :
	void setValue(const std::string& value);

	void segmentSelectionChanged(void);

signals:
	void valueChanged(const std::string& value);

private:
	IAssetEntry* pAssEntry_;
	
	SegmentListWidget* pSegments_;
	SpawnInfoWidget* pSapwn_;
	OriginInfoWidget* pOrigin_;
	SequenceInfoWidget* pSequence_;
	VisualsInfoWidget* pVisualInfo_;
	RotationGraphWidget* pRotation_;
	VelocityInfoWidget* pVerlocity_;
	ColorGraph* pCol_;
	AlphaGraph* pAlpha_;
	GraphWithScale* pSize_;
	GraphWithScale* pScale_;

	int32_t currentSegment_;
	SegmentArr segments_;
};


X_NAMESPACE_END