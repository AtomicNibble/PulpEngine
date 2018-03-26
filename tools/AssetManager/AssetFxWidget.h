#pragma once

#include <QObject>
#include <QtCharts/QChartView.h>

#include <IEffect.h>
#include <vector>

#include "FxSegmentModel.h"

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


class SpinBoxRange : public QWidget
{
	Q_OBJECT
public:
	SpinBoxRange(QWidget* parent = nullptr);

	void setValue(const Range& r);
	void getValue(Range& r);

signals:
	void valueChanged(void);

private:
	QSpinBox* pStart_;
	QSpinBox* pRange_;
};

class SpinBoxRangeDouble : public QWidget
{
	Q_OBJECT
public:
	SpinBoxRangeDouble(QWidget* parent = nullptr);

	void setValue(const RangeDouble& r);
	void getValue(RangeDouble& r);

signals:
	void valueChanged(void);

private:
	QDoubleSpinBox* pStart_;
	QDoubleSpinBox* pRange_;
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
		QString name;
		QLineSeriesArr series;
	};

	typedef std::vector<Graph> GraphArr;

private:
	class ResetGraph : public QUndoCommand
	{
		typedef QVector<QPointF> PointArr;
		typedef std::vector<PointArr> PointArrArr;


	public:
		ResetGraph(GraphEditorView* pView, Graph& graph);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;

	private:
		GraphEditorView* pView_;
		Graph& graph_;
		PointArrArr graphPoints_;
	};

	class ClearPoints : public QUndoCommand
	{
	public:
		ClearPoints(GraphEditorView* pView, QtCharts::QLineSeries* pSeries);

		void redo(void) X_FINAL;
		void undo(void) X_FINAL;

	private:
		GraphEditorView* pView_;
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

	void setValue(const GraphInfo& g);
	void getValue(GraphInfo& g);

	void createGraphs(int32_t numGraphs, int32_t numSeries);
	void setGraphName(int32_t i, const QString& name);
	void setSeriesName(int32_t i, const QString& name);
	void setSeriesColor(int32_t i, const QColor& col);
	void setSingleActiveSeries(bool value);
	void setXAxisRange(float min, float max);
	void setYAxisRange(float min, float max);

	void setSeriesActive(int32_t seriesIdx);
	void setGraphActive(int32_t graphIdx);

	const Graph& activeGraph(void) const;

	qreal getMinY(void) const;
	qreal getMaxY(void) const;

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

signals:
	void valueChanged(void);

private slots:
	void updateColor(void);

private:
	GraphEditorView* pGraph_;
	GradientWidget* pGradient_;
	QCheckBox* pRandomGraph_;

	GradientWidget::ColorPointArr colors_;
};

class GraphWithScale : public QGroupBox
{
	Q_OBJECT

public:
	GraphWithScale(const QString& label, QWidget *parent = nullptr);

	void setValue(const GraphInfo& g);
	void getValue(GraphInfo& g);

signals:
	void valueChanged(void);

private:
	GraphEditor* pGraph_;
	QDoubleSpinBox* pScale_;
	QCheckBox* pRandomGraph_;
};


class SegmentListWidget : public QWidget
{
	Q_OBJECT

public:
	SegmentListWidget(FxSegmentModel* pModel, QWidget *parent = nullptr);
	
	void setActiveIndex(int32_t idx);

signals:
	void itemSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private slots:
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

	void addStageClicked(void);
	void deleteSelectedStageClicked(void);

private:
	FxSegmentModel* pSegmentModel_;
	QTableView* pTable_;
	QPushButton* pDelete_;
};


class SpawnInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	SpawnInfoWidget(QWidget *parent = nullptr);

	void setValue(const SpawnInfo& spawn);
	void getValue(SpawnInfo& spawn);

signals:
	void valueChanged(void);

private slots:
	void buttonToggled(int id, bool checked);

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

signals:
	void valueChanged(void);

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

signals:
	void valueChanged(void);

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

signals:
	void valueChanged(void);

private:
	GraphEditorView* pVelGraph_;
	QDoubleSpinBox* pForwardScale_;
	QDoubleSpinBox* pRightScale_;
	QDoubleSpinBox* pUpScale_;
	QCheckBox* pRandomGraph_;
};

class VelocityInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	VelocityInfoWidget(QWidget *parent = nullptr);

	void setValue(const VelocityInfo& vel);
	void getValue(VelocityInfo& vel);

signals:
	void valueChanged(void);

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

signals:
	void valueChanged(void);

private:
	GraphEditor* pRotationGraph_;
	SpinBoxRangeDouble* pInitialRotation_;
	QDoubleSpinBox* pScale_;
	QCheckBox* pRandomGraph_;
};

class ColorGraph : public QGroupBox
{
	Q_OBJECT

public:
	ColorGraph(QWidget *parent = nullptr);

	void setValue(const ColorInfo& col);
	void getValue(ColorInfo& col);

signals:
	void valueChanged(void);

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

signals:
	void valueChanged(void);

private:
	GraphEditor* pAlphaGraph_;
	QCheckBox* pRandomGraph_;
};


class VisualsInfoWidget : public QGroupBox
{
	Q_OBJECT

public:
	VisualsInfoWidget(QWidget *parent = nullptr);

	void setValue(const VisualsInfo& vis);
	void getValue(VisualsInfo& vis);

signals:
	void valueChanged(void);
	void typeChanged(engine::fx::StageType::Enum type);

private slots:
	void currentIndexChanged(int32_t idx);

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

	void segmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void typeChanged(engine::fx::StageType::Enum type);

	void onValueChanged(void);

signals:
	void valueChanged(const std::string& value);

private:
	IAssetEntry* pAssEntry_;
	
	FxSegmentModel segmentModel_;

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
};


X_NAMESPACE_END