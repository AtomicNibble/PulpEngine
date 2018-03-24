#include "AssetFxWidget.h"

#include <String\Json.h>

#include <IAnimation.h>
#include <IEffect.h>

#include "IAssetEntry.h"

#include "ActionManager.h"
#include "ActionContainer.h"
#include "Command.h"

#include "Context.h"
#include "session.h"

#include <QtCharts\QChartView.h>
#include <QtCharts\Qchart.h>
#include <QtCharts\QLineSeries.h>
#include <QtCharts\qvalueaxis.h>

// #include <QLineSerie>
#include <QTableWidget>


using namespace QtCharts;



X_NAMESPACE_BEGIN(assman)


SpinBoxRange::SpinBoxRange(QWidget* parent) :
	QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pStart_ = new QSpinBox();
	pRange_ = new QSpinBox();

	pStart_->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
	pRange_->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

	QLabel* pLabel = new QLabel("+");

	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(pStart_, 1);
	pLayout->addWidget(pLabel, 0);
	pLayout->addWidget(pRange_, 1);
	setLayout(pLayout);
}

void SpinBoxRange::setValue(const Range& r)
{
	pStart_->setValue(r.start);
	pRange_->setValue(r.range);
}

void SpinBoxRange::getValue(Range& r)
{
	r.start = pStart_->value();
	r.range = pRange_->value();
}

// -----------------------------------


SpinBoxRangeDouble::SpinBoxRangeDouble(QWidget* parent) :
	QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pStart_ = new QDoubleSpinBox();
	pRange_ = new QDoubleSpinBox();
	pStart_->setSingleStep(0.05);
	pRange_->setSingleStep(0.05);

	pStart_->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
	pRange_->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

	QLabel* pLabel = new QLabel("+");

	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(pStart_, 1);
	pLayout->addWidget(pLabel, 0);
	pLayout->addWidget(pRange_, 1);
	setLayout(pLayout);
}

void SpinBoxRangeDouble::setValue(const RangeDouble& r)
{
	pStart_->setValue(r.start);
	pRange_->setValue(r.range);
}

void SpinBoxRangeDouble::getValue(RangeDouble& r)
{
	r.start = pStart_->value();
	r.range = pRange_->value();
}


// -----------------------------------

GraphEditorView::ResetGraph::ResetGraph(GraphEditorView* pView, Graph& graph) :
	pView_(pView),
	graph_(graph)
{
	graphPoints_.resize(graph.series.size());
	for (int32_t i = 0; i<graphPoints_.size(); i++)
	{
		graphPoints_[i] = graph.series[i]->pointsVector();
	}
}

void GraphEditorView::ResetGraph::redo(void)
{
	const auto minVal = pView_->getMinY();
	const auto maxVal = pView_->getMaxY();

	// reset all to default.
	for (int32_t i = 0; i < graphPoints_.size(); i++)
	{
		auto* pSeries = graph_.series[i];
		pSeries->clear();
		// what is default?
		pSeries->append(0, maxVal);
		pSeries->append(1, minVal);
	}

	emit pView_->pointsChanged();
}

void GraphEditorView::ResetGraph::undo(void)
{
	for (int32_t i = 0; i < graphPoints_.size(); i++)
	{
		graph_.series[i]->replace(graphPoints_[i]);
	}

	emit pView_->pointsChanged();
}

// -----------------------------------

GraphEditorView::ClearPoints::ClearPoints(GraphEditorView* pView, QtCharts::QLineSeries* pSeries) :
	pView_(pView),
	pSeries_(pSeries)
{
	points_ = pSeries->pointsVector();
}


void GraphEditorView::ClearPoints::redo(void)
{
	if (points_.size() > 0) {
		pSeries_->clear();
		pSeries_->append(points_.front());
		pSeries_->append(points_.back());
	}

	emit pView_->pointsChanged();
}

void GraphEditorView::ClearPoints::undo(void)
{
	pSeries_->replace(points_);

	emit pView_->pointsChanged();
}


// -----------------------------------

GraphEditorView::AddPoint::AddPoint(GraphEditorView* pView, Graph& graph, int32_t index, QPointF point) :
	pView_(pView),
	graph_(graph),
	index_(index),
	point_(point)
{

}


void GraphEditorView::AddPoint::redo(void)
{
	for (auto* pSeries : graph_.series)
	{
		pSeries->insert(index_, point_);
	}
	
	emit pView_->pointsChanged();
}

void GraphEditorView::AddPoint::undo(void)
{
	for (auto* pSeries : graph_.series)
	{
		pSeries->remove(index_);
	}

	emit pView_->pointsChanged();
}

// -----------------------------------


GraphEditorView::MovePoint::MovePoint(GraphEditorView* pView, Graph& graph, int32_t activeSeries, int32_t index, QPointF delta) :
	pView_(pView),
	graph_(graph),
	delta_(delta),
	activeSeries_(activeSeries),
	index_(index)
{
}

void GraphEditorView::MovePoint::redo(void)
{
	auto& s = graph_.series;
	for (int32_t i = 0; i < s.size(); i++)
	{
		auto pos = s[i]->at(index_);
		if (i == activeSeries_)
		{
			pos += delta_;
		}
		else
		{
			pos.setX(pos.x() + delta_.x());
		}

		s[i]->replace(index_, pos);
	}

	emit pView_->pointsChanged();
}

void GraphEditorView::MovePoint::undo(void)
{
	auto& s = graph_.series;
	for (int32_t i = 0; i < s.size(); i++)
	{
		auto pos = s[i]->at(index_);
		if (i == activeSeries_)
		{
			pos -= delta_;
		}
		else
		{
			pos.setX(pos.x() - delta_.x());
		}

		s[i]->replace(index_, pos);
	}

	emit pView_->pointsChanged();
}

int GraphEditorView::MovePoint::id(void) const
{
	return 1;
}

bool GraphEditorView::MovePoint::mergeWith(const QUndoCommand* pOth)
{
	if (pOth->id() != id()) {
		return false;
	}

	delta_ += static_cast<const MovePoint*>(pOth)->delta_;
	return true;
}



// -----------------------------------



GraphEditorView::GraphEditorView(QWidget *parent) :
	QChartView(parent),
	mouseActive_(false),
	singleActiveSeries_(false),
	activePoint_(-1),
	activeSeries_(-1),
	activeGraph_(-1),
	hoverSeries_(-1)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QChartView::customContextMenuRequested, this, &GraphEditorView::showContextMenu);

	pUndoStack_ = new QUndoStack();

	{
		Context context(Constants::C_GRAPH_EDITOR);
		Context context1(Constants::C_GRAPH_EDITOR);

		auto test = context.at(0);

		pContext_ = new IContext(this);
		pContext_->setContext(context);
		pContext_->setWidget(this);
		ICore::addContextObject(pContext_);
		
		pUndoAction_ = pUndoStack_->createUndoAction(this, tr("&Undo"));
		pUndoAction_->setShortcut(QKeySequence::Undo);
		pUndoAction_->setShortcutVisibleInContextMenu(true);
		pUndoAction_->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

		pRedoAction_ = pUndoStack_->createRedoAction(this, tr("&Redo"));
		pRedoAction_->setShortcuts(QKeySequence::Redo);
		pRedoAction_->setShortcutVisibleInContextMenu(true);
		pUndoAction_->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);


		connect(pUndoStack_, &QUndoStack::canUndoChanged, pUndoAction_, &QAction::setEnabled);
		connect(pUndoStack_, &QUndoStack::canRedoChanged, pRedoAction_, &QAction::setEnabled);

		addAction(pUndoAction_);
		addAction(pRedoAction_);
	}

	pChart_ = new QtCharts::QChart();
	pChart_->legend()->hide();
	pChart_->setMargins(QMargins(1, 1, 1, 1));
	pChart_->layout()->setContentsMargins(0, 0, 0, 0);
	pChart_->setBackgroundRoundness(0);
	pChart_->setBackgroundBrush(QBrush(QRgb(0x2D2D30)));

	pAxisX_ = new QValueAxis();
	pAxisY_ = new QValueAxis();
	
	QBrush axisBrush(Qt::darkGray);
	pAxisX_->setLabelsBrush(axisBrush);
	pAxisY_->setLabelsBrush(axisBrush);
	
	pAxisX_->setTickCount(3);
	pAxisY_->setTickCount(2);
	pAxisX_->setRange(0, 1);
	pAxisY_->setRange(0, 1);
	pAxisX_->setGridLineVisible(false);
	pAxisY_->setGridLineVisible(false);

	pChart_->addAxis(pAxisY_, Qt::AlignLeft);
	pChart_->addAxis(pAxisX_, Qt::AlignBottom);

	setChart(pChart_);
}


void GraphEditorView::setValue(const GraphInfo& g)
{
	// for every graph update series.
	for (size_t i = 0; i < graphs_.size(); i++)
	{
		auto& srcG = g.graphs[i];
		auto& dstG = graphs_[i];

		for (size_t j = 0; j < srcG.series.size(); j++)
		{
			auto* pSeries = dstG.series[j];
			pSeries->clear();

			const auto& srcSeries = srcG.series[j];
			for (size_t p = 0; p < srcSeries.points.size(); p++)
			{
				const GraphPoint& point = srcSeries.points[p];
				pSeries->append(point.pos, point.val);
			}
		}
	}
}

void GraphEditorView::getValue(GraphInfo& g)
{
	g.graphs.resize(graphs_.size());

	for (size_t i = 0; i < graphs_.size(); i++)
	{
		const auto& srcG = graphs_[i];
		auto& dstG = g.graphs[i];

		for (size_t j = 0; j < srcG.series.size(); j++)
		{
			const auto* pSeries = srcG.series[j];
			auto points = pSeries->pointsVector();

			auto& dstSeries = dstG.series[j];
			dstSeries.points.resize(pSeries->count());

			for (int32_t p = 0; p < points.count(); p++)
			{
				GraphPoint& point = dstSeries.points[p];
				point.pos = points[p].rx();
				point.val = points[p].ry();
			}
		}
	}
}


void GraphEditorView::createGraphs(int32_t numGraphs, int32_t numSeries)
{
	if (numGraphs < 1 || numSeries < 1) {
		return;
	}
	
	activeSeries_ = 0;
	activeGraph_ = 0;

	names_.resize(numSeries);
	colors_.resize(numSeries);
	graphs_.resize(numGraphs);

	// set everything to default col disabled.
	QColor defaultCol(QRgb(0x900000));
	for (auto& col : colors_)
	{
		col = defaultCol;
	}

	QPen pen;
	pen.setWidth(LineWidth);
	pen.setStyle(Qt::SolidLine);

	const auto minVal = pAxisY_->min();
	const auto maxVal = pAxisY_->max();

	for (size_t g=0; g<graphs_.size(); g++)
	{
		auto& graph = graphs_[g];
		graph.series.resize(numSeries);
		
		if (g == 1)
		{
			pen.setStyle(DisabledPenStyle);
		}

		for (int32_t i = 0; i < numSeries; i++)
		{
			if (i == 0)
			{
				pen.setColor(colors_[i]);
			}
			else
			{
				pen.setColor(colors_[i].darker(DarkenFactor));
			}

			if (singleActiveSeries_ && i == 1)
			{
				pen.setStyle(DisabledPenStyle);
			}

			QLineSeries* pSeries = new QLineSeries();
			pSeries->append(0, maxVal);
			pSeries->append(1, minVal);
			pSeries->setPointsVisible(true);
			pSeries->setPen(pen);
			pChart_->addSeries(pSeries);

			pSeries->attachAxis(pAxisX_);
			pSeries->attachAxis(pAxisY_);

			connect(pSeries, &QLineSeries::hovered, this, &GraphEditorView::seriesHover);

			// reverse them, so index zero is last added to chart.
			graph.series[(numSeries-1) - i] = pSeries;
		}
	}
}

void GraphEditorView::setGraphName(int32_t i, const QString& name)
{
	graphs_[i].name = name;
}

void GraphEditorView::setSeriesName(int32_t i, const QString& name)
{
	names_[i] = name;
}

void GraphEditorView::setSeriesColor(int32_t i, const QColor& col)
{
	colors_[i] = col;

	QPen pen;
	pen.setWidth(LineWidth);
	pen.setColor(col);

	QPen disablePen(pen);
	disablePen.setColor(col.darker(DarkenFactor));

	// update all colors.
	for (size_t g=0;g<graphs_.size(); g++)
	{
		// anything that's not active series and graph is disabled color.
		const auto& series = graphs_[g].series;

		if (g == activeGraph_)
		{
			pen.setStyle(Qt::SolidLine);
			disablePen.setStyle(Qt::SolidLine);
		}
		else
		{
			pen.setStyle(DisabledPenStyle);
			disablePen.setStyle(DisabledPenStyle);
		}

		if (i == activeSeries_)
		{
			series[i]->setPen(pen);
		}
		else
		{
			series[i]->setPen(disablePen);
		}
	}
}

void GraphEditorView::setSingleActiveSeries(bool value)
{
	singleActiveSeries_ = value;
}

void GraphEditorView::setXAxisRange(float min, float max)
{
	pAxisX_->setRange(min, max);
}

void GraphEditorView::setYAxisRange(float min, float max)
{
	pAxisY_->setRange(min, max);
}

void GraphEditorView::setSeriesActive(int32_t seriesIdx)
{
	if (activeSeries_ == seriesIdx) {
		return;
	}

	const auto& col = colors_[seriesIdx];
	
	QPen pen;
	pen.setWidth(LineWidth);
	pen.setColor(col);

	auto* pTargetSeries = getSeries(activeGraph_, seriesIdx);
	pTargetSeries->setPen(pen);

	auto* pCurrentSeries = activeSeries();
	if (pCurrentSeries)
	{
		pen.setColor(colors_[activeSeries_].darker(DarkenFactor));

		if (singleActiveSeries_)
		{
			pen.setStyle(DisabledPenStyle);
		}

		pCurrentSeries->setPen(pen);
	}

	// re-add series so it's on top.
	pChart_->removeSeries(pTargetSeries);
	addSeriesToChart(pTargetSeries);

	activeSeries_ = seriesIdx;
}

void GraphEditorView::setGraphActive(int32_t graphIdx)
{
	if (activeGraph_ == graphIdx) {
		return;
	}

	QPen pen;
	pen.setWidth(LineWidth);

	// i need to change the current graph to be all dotted and disabled colors.
	if (activeGraph_ != -1)
	{
		auto& g = activeGraph();

		pen.setStyle(DisabledPenStyle);

		for (size_t i = 0; i < g.series.size(); i++)
		{
			pen.setColor(colors_[i].darker(DarkenFactor));
			g.series[i]->setPen(pen);
		}
	}

	// activate all series in target graph.
	auto& g = graphs_[graphIdx];

	for (size_t i = 0; i < g.series.size(); i++)
	{
		pChart_->removeSeries(g.series[i]);
	}

	// re add this graphs series, with the active one on top.
	for (size_t i = 0; i < g.series.size(); i++)
	{
		if (i == activeSeries_) {
			continue;
		}
		addSeriesToChart(g.series[i]);
	}

	addSeriesToChart(getSeries(graphIdx, activeSeries_));

	for (size_t i = 0; i < g.series.size(); i++)
	{
		if (i == activeSeries_)
		{
			pen.setColor(colors_[i]);
			pen.setStyle(Qt::SolidLine);
		}
		else
		{
			pen.setColor(colors_[i].darker(DarkenFactor));

			if (singleActiveSeries_)
			{
				pen.setStyle(DisabledPenStyle);
			}
			else
			{
				pen.setStyle(Qt::SolidLine);
			}
		}

		g.series[i]->setPen(pen);
	}

	activeGraph_ = graphIdx;
}

X_INLINE void GraphEditorView::addSeriesToChart(QtCharts::QLineSeries* pSeries)
{
	pChart_->addSeries(pSeries);
	pSeries->attachAxis(pAxisX_);
	pSeries->attachAxis(pAxisY_);
}

void GraphEditorView::mouseMoveEvent(QMouseEvent *event)
{
	X_UNUSED(event);

	if (isDraggingPoint())
	{
		auto newVal = pChart_->mapToValue(event->localPos());

		auto* pSeries = activeSeries();
		const auto curVal = pSeries->at(activePoint_);

		// can't move begin / end.
		if (activePoint_ == 0 || activePoint_ == pSeries->count() - 1)
		{
			newVal.setX(curVal.x());
		}
		else
		{
			// you can't move past other points.
			auto prevX = pSeries->at(activePoint_ - 1).x();
			auto nextX = pSeries->at(activePoint_ + 1).x();

			const qreal minDist = 0.001;

			newVal.setX(std::clamp(newVal.x(), prevX + minDist, nextX - minDist));
		}

		// clamp in range.
		newVal.setY(std::clamp(newVal.y(), pAxisY_->min(), pAxisY_->max()));
		newVal.setX(std::clamp(newVal.x(), pAxisX_->min(), pAxisX_->max()));

		auto& g = activeGraph();
		QPointF delta = newVal - curVal;
		
		pUndoStack_->push(new MovePoint(this, g, activeSeries_, activePoint_, delta));
		return;
	}

	QChartView::mouseMoveEvent(event);
}

void GraphEditorView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
	{
		QChartView::mousePressEvent(event);
		return;
	}

	if (hoverSeries_ != -1)
	{
		setSeriesActive(hoverSeries_);
		hoverSeries_ = -1;
	}

	if (activePoint_ >= 0)
	{
		pUndoStack_->beginMacro("Move");
		mouseActive_ = true;
		return;
	}

	// click de click
	auto value = pChart_->mapToValue(event->localPos());
	
	auto minValX = pAxisX_->min();
	auto minValY = pAxisY_->min();

	if (value.x() >= minValX && value.y() >= minValY)
	{
		auto* pSeries = activeSeries();
		if (pSeries)
		{
			int32_t i = 0;
			for (; i < pSeries->count(); i++)
			{
				const auto& p = pSeries->at(i);
				if (value.x() < p.x())
				{
					// check if we are really close
					if ((value - p).manhattanLength() < 0.1)
					{
						return;
					}

					break;
				}
			}

			pUndoStack_->push(new AddPoint(this, activeGraph(), i, value));		
			return;
		}
	}

	QChartView::mousePressEvent(event);
}

void GraphEditorView::mouseReleaseEvent(QMouseEvent *event)
{
	if (isDraggingPoint())
	{
		pUndoStack_->endMacro();

		mouseActive_ = false;
		activePoint_ = -1;
	}

	QChartView::mouseReleaseEvent(event);
}

void GraphEditorView::leaveEvent(QEvent *event)
{
	if (!mouseActive_)
	{
		activePoint_ = -1;
	}

	QChartView::leaveEvent(event);
}

void GraphEditorView::seriesHover(const QPointF &point, bool state)
{
	hoverSeries_ = -1;

	if (state) 
	{
		// can select any point from active graph.
		// if you select one of diffrent series you switch to that.
		auto findhoveringPoint = [&point](QtCharts::QLineSeries* pSeries) -> int32_t {
			auto count = pSeries->count();
			for (int32_t i = 0; i < count; i++)
			{
				const auto& p = pSeries->at(i);
				auto rel = point - p;

				if (rel.manhattanLength() < 0.1)
				{
					return i;
				}
			}

			return -1;
		};

		const auto numPoints = activeSeries()->count();

		// look in active series first.
		auto index = findhoveringPoint(activeSeries());
		if (index == -1)
		{
			auto& g = activeGraph();

			for (int32_t s = 0; s < g.series.size(); s++)
			{
				if (s == activeSeries_) {
					continue;
				}

				index = findhoveringPoint(g.series[s]);
				if (index != -1)
				{
					// change series.
					hoverSeries_ = s;
					break;
				}
			}
		}

		// found a point?
		if (index != -1)
		{
			if (index != activePoint_)
			{
				activePoint_ = index;
			}

			if (index == 0 || index == numPoints - 1)
			{
				setCursor(Qt::SizeVerCursor);
			}
			else
			{
				setCursor(Qt::SizeAllCursor);
			}
			return;
		}
	}
	else
	{
		activePoint_ = -1;
	}

	unsetCursor();
}

void GraphEditorView::showContextMenu(const QPoint &pos)
{
	ActionContainer* pActionContainer = ActionManager::createMenu(Constants::M_GRAPH_EDITOR);
	auto* pMenu = pActionContainer->menu();
	pMenu->clear();

	pMenu->addAction(pUndoAction_);
	pMenu->addAction(pRedoAction_);
	pMenu->addSeparator();

	if (names_.size() > 1)
	{
		// show series selection.
		for (size_t i = 0; i<names_.size(); i++)
		{
			QString name = names_[i];
			if (name.isEmpty())
			{
				name = QString("Series %1").arg(i);
			}

			QAction* pAction = pMenu->addAction(name);
			pAction->setCheckable(true);
			pAction->setChecked(i == activeSeries_);
			pAction->setData(qVariantFromValue(i));
			connect(pAction, &QAction::triggered, this, &GraphEditorView::setActiveSeries);
		}

		pMenu->addSeparator();
	}

	// now graphs.
	for (size_t i = 0; i < graphs_.size(); i++)
	{
		QString name = graphs_[i].name;
		if (name.isEmpty())
		{
			name = QString("Graph %1").arg(i);
		}

		QAction* pAction = pMenu->addAction(name);
		pAction->setCheckable(true);
		pAction->setChecked(i == activeGraph_);
		pAction->setData(qVariantFromValue(i));
		connect(pAction, &QAction::triggered, this, &GraphEditorView::setActiveGraph);
	}

	pMenu->addSeparator();

	QAction* pAction = pMenu->addAction("Clear Series");
	pAction->setStatusTip(tr("Clear the knots from current series"));
	connect(pAction, &QAction::triggered, this, &GraphEditorView::clearKnots);

	pAction = pMenu->addAction("Reset Graph");
	connect(pAction, &QAction::triggered, this, &GraphEditorView::resetKnots);

	pMenu->popup(mapToGlobal(pos));
}

void GraphEditorView::setActiveSeries(void)
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	if (pAction)
	{
		int32_t series = pAction->data().value<int32_t>();
		setSeriesActive(series);
	}
}

void GraphEditorView::setActiveGraph(void)
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	if (pAction)
	{
		int32_t graph = pAction->data().value<int32_t>();
		setGraphActive(graph);
	}
}

void GraphEditorView::clearKnots(void)
{
	if (activeSeries_ >= 0)
	{
		// remove all but first and last.
		auto count = activeSeries()->count();
		if (count > 2) {
			pUndoStack_->push(new ClearPoints(this, activeSeries()));
		}
	}
}

void GraphEditorView::resetKnots(void)
{
	if (activeSeries_ >= 0)
	{
		pUndoStack_->push(new ResetGraph(this, activeGraph()));
	}
}


X_INLINE bool GraphEditorView::isDraggingPoint(void) const
{
	return (mouseActive_ && activePoint_ >= 0);
}

X_INLINE QtCharts::QLineSeries* GraphEditorView::getSeries(int32_t graphIdx, int32_t seriesIdx) const
{
	return graphs_[graphIdx].series[seriesIdx];
}

X_INLINE QtCharts::QLineSeries* GraphEditorView::activeSeries(void) const
{
	if (activeGraph_ < 0 || activeSeries_ < 0) {
		return nullptr;
	}

	return graphs_[activeGraph_].series[activeSeries_];
}


X_INLINE const GraphEditorView::Graph& GraphEditorView::activeGraph(void) const
{
	return graphs_[activeGraph_];
}

X_INLINE GraphEditorView::Graph& GraphEditorView::activeGraph(void)
{
	return graphs_[activeGraph_];
}

X_INLINE qreal GraphEditorView::getMinY(void) const
{
	return pAxisY_->min();
}

X_INLINE qreal GraphEditorView::getMaxY(void) const
{
	return pAxisY_->max();
}


// -----------------------------------

GraphEditor::GraphEditor(QWidget* parent) :
	GraphEditorView(parent)
{
}

GraphEditor::GraphEditor(int32_t numGraph, int32_t numSeries, QWidget* parent) :
	GraphEditorView(parent)
{
	createGraphs(numGraph, numSeries);
}

// -----------------------------------

GradientWidget::GradientWidget(QWidget* parent) :
	QWidget(parent)
{
	ColorPoint cp;
	cp.col = QColor(255,255,255);
	cp.pos = 0.0;
	colors_.push_back(cp);

	cp.col = QColor(0, 0, 0);
	cp.pos = 1.0;
	colors_.push_back(cp);
}

void GradientWidget::setColors(const ColorPointArr& colors)
{
	colors_ = colors;

	update();
}

void GradientWidget::paintEvent(QPaintEvent*)
{
	QLinearGradient grad(0,0, width(), height());

	for (const auto& cp : colors_)
	{
		grad.setColorAt(cp.pos, cp.col);
	}

	QPainter painter(this);
	painter.fillRect(rect(), grad);
}

// -----------------------------------

ColorGraphEditor::ColorGraphEditor(int32_t numGraph, QWidget* parent) :
	QWidget(parent)
{
	pGraph_ = new GraphEditorView();
	pGraph_->createGraphs(numGraph, 3);

	pGraph_->setSeriesName(0, "R");
	pGraph_->setSeriesName(1, "G");
	pGraph_->setSeriesName(2, "B");

	pGraph_->setSeriesColor(0, QColor(QRgb(0xc00000)));
	pGraph_->setSeriesColor(1, QColor(QRgb(0x00c000)));
	pGraph_->setSeriesColor(2, QColor(QRgb(0x0000c0)));

	connect(pGraph_, &GraphEditorView::pointsChanged, this, &ColorGraphEditor::updateColor);

	pGradient_ = new GradientWidget();
	pGradient_->setMinimumHeight(10);

	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pLayout->addWidget(pGraph_);
		pLayout->addWidget(pGradient_);
	}

	// i need to know when the graph changes.
	// so i can update colors.

	setLayout(pLayout);
}

void ColorGraphEditor::updateColor(void)
{
	// i only care for active graph.
	const auto& graph = const_cast<const GraphEditorView*>(pGraph_)->activeGraph();

	// now i want all the graphs.
	if (graph.series.size() != 3) {
		return;
	}

	colors_.resize(graph.series.front()->count());

	auto* pSeriesR = graph.series[0];
	auto* pSeriesG = graph.series[1];
	auto* pSeriesB = graph.series[2];

	auto pointsR = pSeriesR->pointsVector();
	auto pointsG = pSeriesG->pointsVector();
	auto pointsB = pSeriesB->pointsVector();

	for (int32_t p = 0; p < pointsR.count(); p++)
	{
		auto pos = pointsR[p].x();

		auto r = pointsR[p].y();
		auto g = pointsG[p].y();
		auto b = pointsB[p].y();

		colors_[p].pos = pos;
		colors_[p].col = QColor::fromRgbF(r,g,b);
	}

	pGradient_->setColors(colors_);
}


void ColorGraphEditor::setValue(const ColorInfo& col)
{
	// need to update graphs.
	pGraph_->setValue(col.col);

	updateColor();
}

void ColorGraphEditor::getValue(ColorInfo& col)
{
	pGraph_->getValue(col.col);
}

// -----------------------------------


GraphWithScale::GraphWithScale(const QString& label, QWidget* parent) :
	QGroupBox(label, parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pGraph_ = new GraphEditor(2, 1);
		pScale_ = new QDoubleSpinBox();
		pScale_->setSingleStep(0.05);

		pRandomGraph_ = new QCheckBox();
		pRandomGraph_->setText("Random Graph");

		pLayout->addWidget(pGraph_);

		QHBoxLayout* pHLayout = new QHBoxLayout();
		pHLayout->setContentsMargins(0, 0, 0, 0);
		pHLayout->addWidget(new QLabel("Scale"));
		pHLayout->addWidget(pScale_);
		pHLayout->addStretch(1);
		pHLayout->addWidget(pRandomGraph_);

		pLayout->addLayout(pHLayout);
	}

	setLayout(pLayout);
}


void GraphWithScale::setValue(const GraphScaleInfo& g)
{
	pGraph_->setValue(g);
	pScale_->setValue(g.scale);
}

void GraphWithScale::getValue(GraphScaleInfo& g)
{
	pGraph_->getValue(g);
	g.scale = pScale_->value();
}

// -----------------------------------


SegmentListWidget::SegmentListWidget(FxSegmentModel* pModel, QWidget* parent) :
	QWidget(parent),
	pSegmentModel_(pModel)
{
	QVBoxLayout* pTableLayout = new QVBoxLayout();
	{
		pTable_ = new QTableView();

		pTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
		pTable_->setSelectionMode(QAbstractItemView::SingleSelection);
		pTable_->setMinimumHeight(100);
		pTable_->setMaximumHeight(200);
		pTable_->horizontalHeader()->setStretchLastSection(true);
		pTable_->setModel(pSegmentModel_);
		
		connect(pTable_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentListWidget::itemSelectionChanged);
		connect(pTable_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentListWidget::selectionChanged);

		pTableLayout->addWidget(pTable_);
	}
	QHBoxLayout* pButtonLayout = new QHBoxLayout();
	{
		QPushButton* pAdd = new QPushButton();
		pAdd->setText(tr("Add"));
		pAdd->setMaximumWidth(80);

		pDelete_ = new QPushButton();
		pDelete_->setText(tr("Delete"));
		pDelete_->setMaximumWidth(80);
		pDelete_->setEnabled(false);

		connect(pAdd, &QPushButton::clicked, this, &SegmentListWidget::addStageClicked);
		connect(pDelete_, &QPushButton::clicked, this, &SegmentListWidget::deleteSelectedStageClicked);

		pButtonLayout->addWidget(pAdd);
		pButtonLayout->addWidget(pDelete_);
		pButtonLayout->addStretch();
	}

	pTableLayout->addLayout(pButtonLayout);

	setLayout(pTableLayout);
}

void SegmentListWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	X_UNUSED(deselected);

	pDelete_->setEnabled(!selected.isEmpty());
}

void SegmentListWidget::addStageClicked(void)
{
	pSegmentModel_->addSegment();
}

void SegmentListWidget::deleteSelectedStageClicked(void)
{
	auto* pSelectModel = pTable_->selectionModel();

	if (!pSelectModel->hasSelection()) {
		return;
	}

	QModelIndexList indexes = pSelectModel->selectedRows();
	for (int32_t i = 0; i < indexes.count(); ++i)
	{
		pSelectModel->model()->removeRow(indexes[i].row(), indexes[i].parent());
	}
}

// -----------------------------------

SpawnInfoWidget::SpawnInfoWidget(QWidget* parent) :
	QGroupBox("Spawn", parent)
{
	QFormLayout* pLayout = new QFormLayout();
	pLayout->setLabelAlignment(Qt::AlignLeft);
	{
		// list of shit.
		pCount_ = new SpinBoxRange();
		pInterval_ = new QSpinBox();
		pLoopCount_ = new QSpinBox();
		pLife_ = new SpinBoxRange();
		pDelay_ = new SpinBoxRange();

		QHBoxLayout* pRadioLayout = new QHBoxLayout();
		{
			QButtonGroup* pGroup = new QButtonGroup();
			pOneShot_ = new QRadioButton();
			pLooping_ = new QRadioButton();

			pOneShot_->setText("One-shot");
			pOneShot_->setChecked(true);
			pLooping_->setText("Looping");

			pGroup->addButton(pOneShot_);
			pGroup->addButton(pLooping_);
			pGroup->setExclusive(true);

			pRadioLayout->addWidget(pOneShot_);
			pRadioLayout->addWidget(pLooping_);
			pRadioLayout->addStretch(1);
		}

		pLayout->addRow(pRadioLayout);
		pLayout->addRow(tr("Count"), pCount_);
		pLayout->addRow(tr("Interval"), pInterval_);
		pLayout->addRow(tr("Loop Count"), pLoopCount_);
		pLayout->addRow(tr("Life"), pLife_);
		pLayout->addRow(tr("Delay"), pDelay_);
	}

	setLayout(pLayout);
}

void SpawnInfoWidget::setValue(const SpawnInfo& spawn)
{
	pLooping_->setChecked(spawn.looping);
	pInterval_->setValue(spawn.interval);
	pLoopCount_->setValue(spawn.loopCount);

	pCount_->setValue(spawn.count);
	pLife_->setValue(spawn.life);
	pDelay_->setValue(spawn.delay);
}

void SpawnInfoWidget::getValue(SpawnInfo& spawn)
{
	spawn.looping = pLooping_->isChecked();
	spawn.interval = pInterval_->value();
	spawn.loopCount = pLoopCount_->value();

	pCount_->getValue(spawn.count);
	pLife_->getValue(spawn.life);
	pDelay_->getValue(spawn.delay);
}

// -----------------------------------

OriginInfoWidget::OriginInfoWidget(QWidget* parent) :
	QGroupBox("Origin", parent)
{
	QFormLayout* pLayout = new QFormLayout();
	{
		pForward_ = new SpinBoxRange();
		pRight_ = new SpinBoxRange();
		pUp_ = new SpinBoxRange();

		pLayout->addRow(tr("Forward"), pForward_);
		pLayout->addRow(tr("Right"), pRight_);
		pLayout->addRow(tr("Up"), pUp_);
	}

	setLayout(pLayout);
}

void OriginInfoWidget::setValue(const OriginInfo& org)
{
	pForward_->setValue(org.spawnOrgX);
	pRight_->setValue(org.spawnOrgY);
	pUp_->setValue(org.spawnOrgZ);
}

void OriginInfoWidget::getValue(OriginInfo& org)
{
	pForward_->getValue(org.spawnOrgX);
	pRight_->getValue(org.spawnOrgY);
	pUp_->getValue(org.spawnOrgZ);
}

// -----------------------------------


SequenceInfoWidget::SequenceInfoWidget(QWidget* parent) :
	QGroupBox("Sequence Control", parent)
{
	QFormLayout* pLayout = new QFormLayout();
	{
		pStart_ = new QSpinBox();
		pPlayRate_ = new QSpinBox();
		pLoopCount_ = new QSpinBox();

		pLayout->addRow(tr("Start"), pStart_);
		pLayout->addRow(tr("PlayRate"), pPlayRate_);
		pLayout->addRow(tr("Loop"), pLoopCount_);
	}

	setLayout(pLayout);
}


void SequenceInfoWidget::setValue(const SequenceInfo& sq)
{
	pStart_->setValue(sq.startFrame);
	pPlayRate_->setValue(sq.fps);
	pLoopCount_->setValue(sq.loop);
}

void SequenceInfoWidget::getValue(SequenceInfo& sq)
{
	sq.startFrame = pStart_->value();
	sq.fps = pPlayRate_->value();
	sq.loop = pLoopCount_->value();
}


// -----------------------------------


VelocityGraph::VelocityGraph(QWidget* parent) :
	QGroupBox(parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pVelGraph_ = new GraphEditorView();
		pVelGraph_->setYAxisRange(-0.5f, 0.5f);
		pVelGraph_->createGraphs(6, 1);
		pVelGraph_->setGraphName(0, "Graph 0: Forward");
		pVelGraph_->setGraphName(1, "Graph 0: Right");
		pVelGraph_->setGraphName(2, "Graph 0: Up");
		pVelGraph_->setGraphName(3, "Graph 1: Forward");
		pVelGraph_->setGraphName(4, "Graph 1: Right");
		pVelGraph_->setGraphName(5, "Graph 1: Up");

		pForwardScale_ = new QDoubleSpinBox();
		pForwardScale_->setMinimumWidth(50);
		pForwardScale_->setMaximumWidth(50);
		pForwardScale_->setSingleStep(0.05);
		pRightScale_ = new QDoubleSpinBox();
		pRightScale_->setMinimumWidth(50);
		pRightScale_->setMaximumWidth(50);
		pRightScale_->setSingleStep(0.05);
		pUpScale_ = new QDoubleSpinBox();
		pUpScale_->setMinimumWidth(50);
		pUpScale_->setMaximumWidth(50);
		pUpScale_->setSingleStep(0.05);

		pRandomGraph_ = new QCheckBox();
		pRandomGraph_->setText("Random Graph");

//		QHBoxLayout* pHLayout = new QHBoxLayout();
	//	pHLayout ->addStretch(1);
	//	pHLayout ->addWidget(pRandomGraph_);

		QHBoxLayout* pFormLayout = new QHBoxLayout();
		pFormLayout->addWidget(new QLabel("Forward"));
		pFormLayout->addWidget(pForwardScale_);
		pFormLayout->addWidget(new QLabel("Right"));
		pFormLayout->addWidget(pRightScale_);
		pFormLayout->addWidget(new QLabel("Up"));
		pFormLayout->addWidget(pUpScale_);
		pFormLayout->addWidget(pRandomGraph_);
		pFormLayout->addStretch(1);

		pLayout->addWidget(pVelGraph_);
	//	pLayout->addLayout(pHLayout);
		pLayout->addLayout(pFormLayout);

	}

	setLayout(pLayout);
}


void VelocityGraph::setValue(const VelocityInfo& vel)
{
	pVelGraph_->setValue(vel.graph);
}

void VelocityGraph::getValue(VelocityInfo& vel)
{
	pVelGraph_->getValue(vel.graph);
}


// -----------------------------------


VelocityInfoWidget::VelocityInfoWidget(QWidget* parent) :
	QGroupBox("Velocity", parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		QGroupBox* pMoveGroupBox = new QGroupBox("Relative to");
		{
			QHBoxLayout* pMoveLayout = new QHBoxLayout();
			QButtonGroup* pGroup = new QButtonGroup();

			pSpawn_ = new QRadioButton();
			pNow_ = new QRadioButton();

			pSpawn_->setText("Spawn");
			pSpawn_->setChecked(true);
			pNow_->setText("Now");

			pGroup->addButton(pSpawn_);
			pGroup->addButton(pNow_);
			pGroup->setExclusive(true);

			pMoveLayout->addWidget(pSpawn_);
			pMoveLayout->addWidget(pNow_);
			
			pMoveGroupBox->setMaximumWidth(140);
			pMoveGroupBox->setLayout(pMoveLayout);
		}

		pGraph_ = new VelocityGraph();

		pLayout->addWidget(pMoveGroupBox);
		pLayout->addWidget(pGraph_);
	}

	setLayout(pLayout);
}

void VelocityInfoWidget::setValue(const VelocityInfo& vel)
{
	static_assert(engine::fx::RelativeTo::ENUM_COUNT == 2, "Enum count changed? This might need updating");

	if (vel.postionType == engine::fx::RelativeTo::Spawn) {
		pSpawn_->setChecked(true);
	}

	pGraph_->setValue(vel);
}

void VelocityInfoWidget::getValue(VelocityInfo& vel)
{
	static_assert(engine::fx::RelativeTo::ENUM_COUNT == 2, "Enum count changed? This might need updating");

	vel.postionType = engine::fx::RelativeTo::Now;
	if (pSpawn_->isChecked()) {
		vel.postionType = engine::fx::RelativeTo::Spawn;
	} 

	pGraph_->getValue(vel);
}

// -----------------------------------

RotationGraphWidget::RotationGraphWidget(QWidget *parent) :
	QGroupBox("Rotation", parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pRotationGraph_ = new GraphEditor();
		pRotationGraph_->setYAxisRange(-0.5f,0.5f);
		pRotationGraph_->createGraphs(2, 1);

		pInitialRotation_ = new SpinBoxRangeDouble();
		pInitialRotation_->setMinimumWidth(80);

		pScale_ = new QDoubleSpinBox();
		pScale_->setSingleStep(0.05);

		pRandomGraph_ = new QCheckBox();
		pRandomGraph_->setText("Random Graph");


		QHBoxLayout* pFormLayout = new QHBoxLayout();
		pFormLayout->addWidget(new QLabel("Initial Rotation"));
		pFormLayout->addWidget(pInitialRotation_);
		pFormLayout->addStretch(1);

		QHBoxLayout* pHLayout = new QHBoxLayout();
		pHLayout->setContentsMargins(0, 0, 0, 0);
		pHLayout->addWidget(new QLabel("Scale"));
		pHLayout->addWidget(pScale_);
		pHLayout->addStretch(1);
		pHLayout->addWidget(pRandomGraph_);


		pLayout->addLayout(pFormLayout);
		pLayout->addWidget(pRotationGraph_);
		pLayout->addLayout(pHLayout);
	}

	setLayout(pLayout);
}


void RotationGraphWidget::setValue(const RotationInfo& rot)
{
	pInitialRotation_->setValue(rot.initial);
	pRotationGraph_->setValue(rot.rot);
}

void RotationGraphWidget::getValue(RotationInfo& rot)
{
	pInitialRotation_->getValue(rot.initial);
	pRotationGraph_->getValue(rot.rot);
}

// -----------------------------------
	
ColorGraph::ColorGraph(QWidget *parent) :
	QGroupBox("RGB", parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pColorGraph_ = new ColorGraphEditor(2);

		pLayout->addWidget(pColorGraph_);
	}

	setLayout(pLayout);
}


void ColorGraph::setValue(const ColorInfo& col)
{
	pColorGraph_->setValue(col);
}

void ColorGraph::getValue(ColorInfo& col)
{
	pColorGraph_->getValue(col);
}


// -----------------------------------

AlphaGraph::AlphaGraph(QWidget *parent) :
	QGroupBox("Alpha", parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pAlphaGraph_ = new GraphEditor(2, 1);

		pLayout->addWidget(pAlphaGraph_);
	}

	setLayout(pLayout);
}

void AlphaGraph::setValue(const ColorInfo& col)
{
	pAlphaGraph_->setValue(col.alpha);
}

void AlphaGraph::getValue(ColorInfo& col)
{
	pAlphaGraph_->getValue(col.alpha);
}


// -----------------------------------


VisualsInfoWidget::VisualsInfoWidget(QWidget* parent) :
	QGroupBox("Visuals", parent)
{
	QFormLayout* pLayout = new QFormLayout();
	{
		pType_ = new QComboBox();

		for (uint32_t i = 0; i < engine::fx::StageType::ENUM_COUNT; i++)
		{
			QString name = engine::fx::StageType::ToString(i);
			pType_->addItem(name);
		}

		connect(pType_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VisualsInfoWidget::currentIndexChanged);

		pMaterial_ = new QLineEdit();

		pLayout->addRow(tr("Type"), pType_);
		pLayout->addRow(tr("Material"), pMaterial_);
	}

	setLayout(pLayout);
}

void VisualsInfoWidget::setValue(const VisualsInfo& vis)
{
	blockSignals(true);

	pMaterial_->setText(vis.material);
	pType_->setCurrentIndex(static_cast<int32_t>(vis.type));

	blockSignals(false);
}

void VisualsInfoWidget::getValue(VisualsInfo& vis)
{
	vis.material = pMaterial_->text();
	
	auto idx = pType_->currentIndex();
	X_ASSERT(idx >= 0 && idx < engine::fx::StageType::ENUM_COUNT, "Invalid index for type combo")(idx, engine::fx::StageType::ENUM_COUNT);

	vis.type = static_cast<engine::fx::StageType::Enum>(idx);
}

void VisualsInfoWidget::currentIndexChanged(int32_t idx)
{
	auto type = static_cast<engine::fx::StageType::Enum>(idx);

	emit typeChanged(type);
}

// -----------------------------------

FxSegmentModel::FxSegmentModel(QObject *parent) :
	QAbstractTableModel(parent)
{
	addSegment();

	core::string test;
	getJson(test);
}

void FxSegmentModel::getJson(core::string& jsonStrOut)
{
	// build me some json.
	core::json::StringBuffer s;
	core::json::Writer<core::json::StringBuffer> writer(s);

	writer.SetMaxDecimalPlaces(5);

	writer.StartObject();
	writer.Key("stages");
	writer.StartArray();

	auto writeRange = [&](const char* pPrefix, const Range& range) {

		core::StackString<64> startStr(pPrefix);
		core::StackString<64> rangeStr(pPrefix);

		startStr.append("Start");
		rangeStr.append("Range");

		writer.Key(startStr.c_str());
		writer.Int(range.start);
		writer.Key(rangeStr.c_str());
		writer.Int(range.range);
	};


	auto writeGraph = [&](const char* pName, const GraphInfo& g, float scale) {

		writer.Key(pName);
		writer.StartObject();
		writer.Key("scale");
		writer.Double(scale);

		writer.Key("graphs");
		writer.StartArray();

		for (size_t i = 0; i < g.graphs.size(); i++)
		{
			const auto& graph = g.graphs[i];

			writer.StartArray();

			X_ASSERT(graph.series.size() == 1, "Unexpected series size")(graph.series.size());

			auto& series = graph.series.front();

			for (size_t p = 0; p < series.points.size(); p++)
			{
				auto& point = series.points[p];

				writer.StartObject();
				writer.Key("time");
				writer.Double(point.pos);
				writer.Key("val");
				writer.Double(point.val);
				writer.EndObject();
			}

			writer.EndArray();
		}

		writer.EndArray();
		writer.EndObject();
	};

	auto writeSubGraph = [&](const char* pName, const GraphInfo& g, float scale, std::initializer_list<size_t> indexes) {

		writer.Key(pName);
		writer.StartObject();
		writer.Key("scale");
		writer.Double(scale);

		writer.Key("graphs");
		writer.StartArray();

		for (size_t i = 0; i < g.graphs.size(); i++)
		{
			if (std::find(indexes.begin(), indexes.end(), i) == indexes.end()) {
				continue;
			}

			const auto& graph = g.graphs[i];

			writer.StartArray();

			X_ASSERT(graph.series.size() == 1, "Unexpected series size")(graph.series.size());

			auto& series = graph.series.front();

			for (size_t p = 0; p < series.points.size(); p++)
			{
				auto& point = series.points[p];

				writer.StartObject();
				writer.Key("time");
				writer.Double(point.pos);
				writer.Key("val");
				writer.Double(point.val);
				writer.EndObject();
			}

			writer.EndArray();
		}

		writer.EndArray();
		writer.EndObject();
	};


	auto writeColGraph = [&](const char* pName, const GraphInfo& g) {

		writer.Key(pName);
		writer.StartObject();
		writer.Key("scale");
		writer.Double(1.0);

		writer.Key("graphs");
		writer.StartArray();

		for (size_t i = 0; i < g.graphs.size(); i++)
		{
			const auto& graph = g.graphs[i];

			writer.StartArray();

			X_ASSERT(graph.series.size() == 3, "Unexpected series size")(graph.series.size());

			// all series should be same size.
			auto& seriesR = graph.series[0];
			auto& seriesG = graph.series[1];
			auto& seriesB = graph.series[2];

			bool sizeMatch = (std::adjacent_find(graph.series.begin(), graph.series.end(),
				[](const SeriesData& a, const SeriesData& b) -> bool {
					return a.points.size() != b.points.size();
				}
			) == graph.series.end());

			X_ASSERT(sizeMatch, "Series size don't math")(seriesR.points.size(), seriesG.points.size(), seriesB.points.size());

			for (size_t p = 0; p < seriesR.points.size(); p++)
			{
				// all points sshuld have same pos.
				auto& point = seriesR.points[p];

				auto r = seriesR.points[p].val;
				auto g = seriesG.points[p].val;
				auto b = seriesB.points[p].val;

				writer.StartObject();
				writer.Key("time");
				writer.Double(point.pos);
				writer.Key("rgb");
				writer.StartArray();
					writer.Double(r);
					writer.Double(g);
					writer.Double(b);
				writer.EndArray();
				writer.EndObject();
			}

			writer.EndArray();
		}

		writer.EndArray();
		writer.EndObject();
	};

	for (auto& segment : segments_)
	{
		writer.StartObject();

		auto name = segment->name.toStdString();

		if (segment->spawn.looping)
		{
			segment->flags.Set(engine::fx::StageFlag::Looping);
		}
		else
		{
			segment->flags.Remove(engine::fx::StageFlag::Looping);
		}

		// manually build flag string instead of using Flag::ToString, as the format of the flags is important.
		core::StackString<512> flagsStr;
		for (int32_t i = 0; i < engine::fx::StageFlags::FLAGS_COUNT; i++)
		{
			auto flag = static_cast<engine::fx::StageFlag::Enum>(1 << i);
			if (segment->flags.IsSet(flag))
			{
				if (flagsStr.isNotEmpty()) {
					flagsStr.append(" ");
				}
				flagsStr.append(engine::fx::StageFlag::ToString(flag));
			}
		}


		writer.Key("name");
		writer.String(name.c_str());
		
		writer.Key("enabled");
		writer.Bool(segment->enabled);

		writer.Key("type");
		writer.String(engine::fx::StageType::ToString(segment->vis.type));

		writer.Key("relativeTo");
		writer.String(engine::fx::RelativeTo::ToString(segment->vel.postionType));

		writer.Key("flags");
		writer.String(flagsStr.c_str());

		writer.Key("materials");
		writer.StartArray();
			writer.String("Goat");
		writer.EndArray();

		writer.Key("interval");
		writer.Int(segment->spawn.interval);
		writer.Key("loopCount");
		writer.Int(segment->spawn.loopCount);
		
		writeRange("count", segment->spawn.count);
		writeRange("life", segment->spawn.life);
		writeRange("delay", segment->spawn.delay);

		writeRange("spawnOrgX", segment->origin.spawnOrgX);
		writeRange("spawnOrgY", segment->origin.spawnOrgY);
		writeRange("spawnOrgZ", segment->origin.spawnOrgZ);

		writer.Key("sequence");
		writer.StartObject();

			writer.Key("startFrame");
			writer.Int(segment->seq.startFrame);
			writer.Key("fps");
			writer.Int(segment->seq.fps);
			writer.Key("loop");
			writer.Int(segment->seq.loop);

		writer.EndObject();


		// GRAPH ME BABY!
		writeColGraph("colorGraph", segment->col.col);
		writeGraph("alphaGraph", segment->col.alpha, 1.f);
		writeGraph("sizeGraph", segment->size.size, segment->size.size.scale);
		writeGraph("scaleGraph", segment->size.scale, segment->size.scale.scale);
		writeGraph("rotGraph", segment->rot.rot, segment->rot.rot.scale);

		// need to wrtie a verlocity graph.
		// it's a little special since we allow seperate scales.
		// vel0XGraph, vel0XGraph, vel0XGraph is the first graph.
		writeSubGraph("vel0XGraph", segment->vel.graph, segment->vel.forwardScale, { 0,3 } );
		writeSubGraph("vel0YGraph", segment->vel.graph, segment->vel.rightScale, { 1,4 } );
		writeSubGraph("vel0ZGraph", segment->vel.graph, segment->vel.upScale, { 2,5 } );


		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	jsonStrOut = core::string(s.GetString(), s.GetSize());
}

void FxSegmentModel::addSegment()
{
	int currentRows = static_cast<int32_t>(segments_.size());

	beginInsertRows(QModelIndex(), currentRows, currentRows);

	auto seg = std::make_unique<Segment>();

	seg->name = QString("Goat");
	seg->enabled = true;

	// so for size i want both graphs to have some default points.
	GraphInfo linDescend;
	GraphInfo zero;

	GraphData linGraph;
	GraphData zeroGraph;

	SeriesData linDescendSeries;
	SeriesData zeroSeries;

	{
		linDescendSeries.points.push_back(GraphPoint(0.f, 1.f));
		linDescendSeries.points.push_back(GraphPoint(1.f, 0.f));

		linGraph.series.push_back(linDescendSeries);

		// each graph have one series.
		linDescend.graphs.push_back(linGraph);
		linDescend.graphs.push_back(linGraph);
	}

	{
		zeroSeries.points.push_back(GraphPoint(0.f, 0.f));
		zeroSeries.points.push_back(GraphPoint(1.f, 0.f));

		zeroGraph.series.push_back(zeroSeries);

		zero.graphs.push_back(zeroGraph);
		zero.graphs.push_back(zeroGraph);
	}

	// verlocity has 6 graphs with one seriex.
	seg->vel.graph.graphs.reserve(6);
	for (int32_t i = 0; i < 6; i++)
	{
		seg->vel.graph.graphs.push_back(zeroGraph);
	}

	seg->size.size.graphs = linDescend.graphs;
	seg->size.scale.graphs = linDescend.graphs;

	// so i want 2 graphs with 3 series each.
	{
		GraphData colGraph;
		colGraph.series.push_back(linDescendSeries);
		colGraph.series.push_back(linDescendSeries);
		colGraph.series.push_back(linDescendSeries);

		seg->col.col.graphs.push_back(colGraph);
		seg->col.col.graphs.push_back(std::move(colGraph));
	}

	seg->col.alpha.graphs = linDescend.graphs;

	// rotation is 0.5 - -0.5
	seg->rot.rot.graphs = zero.graphs;

	segments_.push_back(std::move(seg));

	endInsertRows();
}

void FxSegmentModel::setSegmentType(int32_t idx, engine::fx::StageType::Enum type)
{
	segments_[idx]->vis.type = type;
	
	auto modelIndex = index(idx, 1);

	emit dataChanged(modelIndex, modelIndex);
}

int FxSegmentModel::rowCount(const QModelIndex & /*parent*/) const
{
	return static_cast<int32_t>(segments_.size());
}

int FxSegmentModel::columnCount(const QModelIndex & /*parent*/) const
{
	return 4;
}

QVariant FxSegmentModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	int col = index.column();

	auto& segment = segments_[row];

	switch (role)
	{
		case Qt::DisplayRole:
			if (col == 0)
			{
				return segment->name;
			}
			if (col == 1)
			{
				return engine::fx::StageType::ToString(segment->vis.type);
			}
			if (col == 2)
			{
				return 0;
			}
			if (col == 3)
			{
				return 0;
			}
			break;
		case Qt::CheckStateRole:
			if (col == 0)
			{
				if (segment->enabled) {
					return Qt::Checked;
				}

				return Qt::Unchecked;
			}
	}
	return QVariant();
}

bool FxSegmentModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	auto& segment = segments_[index.row()];

	if (role == Qt::CheckStateRole && index.column() == 0)
	{
		bool enabled = (value == Qt::Checked);
		
		if(enabled != segment->enabled)
		{
			segment->enabled = enabled;
			emit dataChanged(index, index);
		}
	}
	else if (role == Qt::EditRole && index.column() == 0)
	{
		if (segment->name != value.toString())
		{
			segment->name = value.toString();
			emit dataChanged(index, index);
		}
	}

	return true;
}

Qt::ItemFlags FxSegmentModel::flags(const QModelIndex &index) const
{
	auto flags = QAbstractTableModel::flags(index);

	if (index.column() == 0) {
		flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
	}

	return flags; 
}

QVariant FxSegmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal) 
		{
			switch (section)
			{
				case 0:
					return QString("Name");
				case 1:
					return QString("Type");
				case 2:
					return QString("Delay");
				case 3:
					return QString("Count");
			}
		}
	}
	return QVariant();
}

bool FxSegmentModel::removeRows(int row, int count, const QModelIndex &parent)
{
	if (row > segments_.size())
	{
		return false;
	}

	if (count != 1) 
	{
		return false;
	}

	beginRemoveRows(parent, row, row);
	segments_.erase(segments_.begin() + row);
	endRemoveRows();
	return true;
}
// -----------------------------------



AssetFxWidget::AssetFxWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
	segmentModel_(),
	currentSegment_(-1)
{
//	QHBoxLayout* pLayout = new QHBoxLayout();
//	pLayout->setContentsMargins(0, 0, 0, 0);

	// gonna get crazy up in here!
	// need something to manage stages.
	{
		QVBoxLayout* pTableLayout = new QVBoxLayout();

		pSapwn_ = new SpawnInfoWidget();
		pOrigin_ = new OriginInfoWidget();
		pSegments_ = new SegmentListWidget(&segmentModel_);
		pSequence_ = new SequenceInfoWidget();
		pSize_ = new GraphWithScale("Size");
		pScale_ = new GraphWithScale("Scale");
		pVisualInfo_ = new VisualsInfoWidget();
		pVerlocity_ = new VelocityInfoWidget();
		pRotation_ = new RotationGraphWidget();
		pCol_ = new ColorGraph();
		pAlpha_ = new AlphaGraph();

		const int minHeight = 300;
		const int minWidth = 300;
		const int maxWidth = 400;

		pSize_->setMinimumWidth(300);
		pScale_->setMinimumWidth(300);
		pVerlocity_->setMinimumWidth(300);
		pRotation_->setMinimumWidth(300);
		pCol_->setMinimumWidth(300);
		pAlpha_->setMinimumWidth(300);

		pSize_->setMinimumHeight(minHeight);
		pScale_->setMinimumHeight(minHeight);
		pVerlocity_->setMinimumHeight(minHeight + 50);
		pRotation_->setMinimumHeight(minHeight);
		pCol_->setMinimumHeight(minHeight);
		pAlpha_->setMinimumHeight(minHeight);

		pSize_->setMaximumWidth(maxWidth);
		pScale_->setMaximumWidth(maxWidth);
		pVerlocity_->setMaximumWidth(maxWidth);
		pRotation_->setMaximumWidth(maxWidth);
		pVisualInfo_->setMaximumWidth(maxWidth);
		pSapwn_->setMaximumWidth(maxWidth);
		pOrigin_->setMaximumWidth(maxWidth);
		pSequence_->setMaximumWidth(maxWidth);
		pCol_->setMaximumWidth(maxWidth);
		pAlpha_->setMaximumWidth(maxWidth);

		QHBoxLayout* pShizLayout = new QHBoxLayout();
		pShizLayout->setContentsMargins(0, 0, 0, 0);
		pShizLayout->addWidget(pSize_);
		pShizLayout->addWidget(pScale_);
		pShizLayout->addStretch(0);

		QHBoxLayout* pSizeLayout = new QHBoxLayout();
		pSizeLayout->setContentsMargins(0,0,0,0);
		pSizeLayout->addWidget(pSize_);
		pSizeLayout->addWidget(pScale_);
		pSizeLayout->addStretch(0);

		QHBoxLayout* pColLayout = new QHBoxLayout();
		pColLayout->setContentsMargins(0, 0, 0, 0);
		pColLayout->addWidget(pCol_);
		pColLayout->addWidget(pAlpha_);
		pColLayout->addStretch(0);

		QHBoxLayout* pVelLayout = new QHBoxLayout();
		pVelLayout->setContentsMargins(0, 0, 0, 0);
		pVelLayout->addWidget(pVerlocity_);
		pVelLayout->addWidget(pRotation_);
		pVelLayout->addStretch(0);


		pTableLayout->addWidget(pVisualInfo_);
		pTableLayout->addWidget(pSapwn_);
		pTableLayout->addWidget(pOrigin_);
		pTableLayout->addWidget(pSequence_);
		pTableLayout->addLayout(pSizeLayout);
		pTableLayout->addLayout(pVelLayout);
		pTableLayout->addLayout(pColLayout);
		pTableLayout->addWidget(pSegments_);

		enableWidgets(false);

		// HEllloo JERRRYY!!!
		connect(pSegments_, &SegmentListWidget::itemSelectionChanged, this, &AssetFxWidget::segmentSelectionChanged);
		connect(pVisualInfo_, &VisualsInfoWidget::typeChanged, this, &AssetFxWidget::typeChanged);


		setLayout(pTableLayout);
	}

	// setLayout(pLayout);

	setValue(value);
}


AssetFxWidget::~AssetFxWidget()
{

}

void AssetFxWidget::enableWidgets(bool enable)
{
	pSapwn_->setEnabled(enable);
	pOrigin_->setEnabled(enable);
	pSequence_->setEnabled(enable);
	pVisualInfo_->setEnabled(enable);
	pRotation_->setEnabled(enable);
	pVerlocity_->setEnabled(enable);
	pCol_->setEnabled(enable);
	pAlpha_->setEnabled(enable);
	pSize_->setEnabled(enable);
	pScale_->setEnabled(enable);
}


void AssetFxWidget::setValue(const std::string& value)
{
	X_UNUSED(value);

	blockSignals(true);
	

	blockSignals(false);
}

void AssetFxWidget::segmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	X_UNUSED(selected, deselected);

	if (selected.count() != 1) {
		enableWidgets(false);
		return;
	}

	auto indexes = selected.first().indexes();

	int32_t curRow = indexes.first().row(); 

	if (currentSegment_ >= 0)
	{
		auto& segment = segmentModel_.getSegment(currentSegment_);

		pSapwn_->getValue(segment.spawn);
		pOrigin_->getValue(segment.origin);
		pSequence_->getValue(segment.seq);
		pVisualInfo_->getValue(segment.vis);
		pRotation_->getValue(segment.rot);
		pVerlocity_->getValue(segment.vel);
		pCol_->getValue(segment.col);
		pAlpha_->getValue(segment.col);
		pSize_->getValue(segment.size.size);
		pScale_->getValue(segment.size.scale);
	}
	
	{
		auto& segment = segmentModel_.getSegment(curRow);

		pSapwn_->setValue(segment.spawn);
		pOrigin_->setValue(segment.origin);
		pSequence_->setValue(segment.seq);
		pVisualInfo_->setValue(segment.vis);
		pRotation_->setValue(segment.rot);
		pVerlocity_->setValue(segment.vel);
		pCol_->setValue(segment.col);
		pAlpha_->setValue(segment.col);
		pSize_->setValue(segment.size.size);
		pScale_->setValue(segment.size.scale);
	}

	if (currentSegment_ < 0) {
		enableWidgets(true);
	}

	currentSegment_ = curRow;
}

void AssetFxWidget::typeChanged(engine::fx::StageType::Enum type)
{

	segmentModel_.setSegmentType(currentSegment_, type);

	// TODO: enable / disable widgets based on type.
}


X_NAMESPACE_END