#include "AssetFxWidget.h"

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

GraphEditorView::ResetGraph::ResetGraph(Graph& graph) :
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
	// reset all to default.
	for (int32_t i = 0; i < graphPoints_.size(); i++)
	{
		auto* pSeries = graph_.series[i];
		pSeries->clear();
		// what is default?
		pSeries->append(0, 0.5);
		pSeries->append(1, 0.5);
	}
}

void GraphEditorView::ResetGraph::undo(void)
{
	for (int32_t i = 0; i < graphPoints_.size(); i++)
	{
		graph_.series[i]->replace(graphPoints_[i]);
	}
}

// -----------------------------------

GraphEditorView::ClearPoints::ClearPoints(QtCharts::QLineSeries* pSeries) :
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
}

void GraphEditorView::ClearPoints::undo(void)
{
	pSeries_->replace(points_);
}


// -----------------------------------

GraphEditorView::AddPoint::AddPoint(Graph& graph, int32_t index, QPointF point) :
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
}

void GraphEditorView::AddPoint::undo(void)
{
	for (auto* pSeries : graph_.series)
	{
		pSeries->remove(index_);
	}
}

// -----------------------------------


GraphEditorView::MovePoint::MovePoint(Graph& graph, int32_t activeSeries, int32_t index, QPointF delta) :
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

	pChart_->addAxis(pAxisX_, Qt::AlignLeft);
	pChart_->addAxis(pAxisY_, Qt::AlignBottom);

	setChart(pChart_);
}


void GraphEditorView::setSeriesValue(int32_t idx, const GrapInfo& g)
{
	// take the graphs and apply them.
	X_ASSERT(idx < names_.size(), "Series index out of range")(idx);

	for (size_t i = 0; i < g.series.size(); i++)
	{
		auto& series = g.series[i];

		if (i >= graphs_.size()) {
			break;
		}

		auto& graphSeries = graphs_[i].series;
		auto* pSeries = graphSeries.front();

		pSeries->clear();

		for (size_t p=0; p<series.points.size(); p++)
		{
			const GraphPoint& point = series.points[p];
			pSeries->append(point.pos, point.val);
		}
	}
}

void GraphEditorView::getSeriesValue(int32_t idx, GrapInfo& g)
{
	X_ASSERT(idx < names_.size(), "Series index out of range")(idx);

	g.series.resize(graphs_.size());

	for (size_t i = 0; i < graphs_.size(); i++)
	{
		const auto& src = graphs_[i];
		const auto* pSeries = src.series[idx];

		auto& series = g.series[i];
		series.points.resize(pSeries->count());

		auto points = pSeries->pointsVector();

		for (int32_t p = 0; p < points.count(); p++)
		{
			GraphPoint& point = series.points[p];
			point.pos = points[p].rx();
			point.val = points[p].ry();
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
			pSeries->append(0, 0.5);
			pSeries->append(1, 0.5);
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

			newVal.setX(std::clamp(newVal.x(), prevX, nextX));
		}

		// clamp in range.
		newVal.setY(std::clamp(newVal.y(), pAxisY_->min(), pAxisY_->max()));

		auto& g = activeGraph();
		QPointF delta = newVal - curVal;
		
		pUndoStack_->push(new MovePoint(g, activeSeries_, activePoint_, delta));

		emit pointsChanged();
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
	
	if (value.x() > 0 && value.y() > 0)
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

			pUndoStack_->push(new AddPoint(activeGraph(), i, value));		

			emit pointsChanged();
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
		QAction* pAction = pMenu->addAction(QString("Graph %1").arg(i));
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
			pUndoStack_->push(new ClearPoints(activeSeries()));
		}
	}
}

void GraphEditorView::resetKnots(void)
{
	if (activeSeries_ >= 0)
	{
		pUndoStack_->push(new ResetGraph(activeGraph()));
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

// -----------------------------------

GraphEditor::GraphEditor(int32_t numGraph, int32_t numSeries, QWidget* parent) :
	GraphEditorView(parent)
{
	createGraphs(numGraph, numSeries);
}

// -----------------------------------

GradientWidget::GradientWidget(QWidget* parent) :
	QWidget(parent)
{


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
	pGraph_->setSeriesValue(0, col.r);
	pGraph_->setSeriesValue(1, col.g);
	pGraph_->setSeriesValue(2, col.b);

	updateColor();
}

void ColorGraphEditor::getValue(ColorInfo& col)
{
	pGraph_->getSeriesValue(0, col.r);
	pGraph_->getSeriesValue(1, col.g);
	pGraph_->getSeriesValue(2, col.b);
}

// -----------------------------------


GraphWithScale::GraphWithScale(const QString& label, QWidget* parent) :
	QGroupBox(label, parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pGraph_ = new GraphEditor(2, 1);
		pScale_ = new QDoubleSpinBox();
		pScale_->setMaximumWidth(50);
		pScale_->setSingleStep(0.05);

		pLayout->addWidget(pGraph_);

		QFormLayout* pFormLayout = new QFormLayout();
		pFormLayout->addRow(tr("Scale"), pScale_);

		pLayout->addLayout(pFormLayout);
	}

	setLayout(pLayout);
}


void GraphWithScale::setValue(const GrapScaleInfo& g)
{
	pGraph_->setSeriesValue(0, g);
	pScale_->setValue(g.scale);
}

void GraphWithScale::getValue(GrapScaleInfo& g)
{
	pGraph_->getSeriesValue(0, g);
	g.scale = pScale_->value();
}

// -----------------------------------


SegmentListWidget::SegmentListWidget(QWidget* parent) :
	QWidget(parent)
{
	QVBoxLayout* pTableLayout = new QVBoxLayout();
	{
		pTable_ = new QTableWidget();

		QStringList labels;
		labels << "Name" << "Type" << "Delay" << "Count";

		pTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
		pTable_->setSelectionMode(QAbstractItemView::SingleSelection);
		pTable_->setColumnCount(labels.size());
		pTable_->setMinimumHeight(100);
		pTable_->setMaximumHeight(150);
		pTable_->setHorizontalHeaderLabels(labels);
		pTable_->horizontalHeader()->setStretchLastSection(true);

		connect(pTable_, &QTableWidget::itemSelectionChanged, this, &SegmentListWidget::itemSelectionChanged);
		connect(pTable_, &QTableWidget::itemSelectionChanged, this, &SegmentListWidget::selectionChanged);

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


int SegmentListWidget::count(void) const
{
	return pTable_->rowCount();
}

int SegmentListWidget::currentRow(void) const
{
	return pTable_->currentRow();
}

void SegmentListWidget::itemSelectionChanged(void)
{
	const int num = pTable_->selectedItems().size();

	pDelete_->setEnabled(num != 0);
}

void SegmentListWidget::addStageClicked(void)
{
	int32_t row = pTable_->rowCount();
	pTable_->insertRow(row);

	QTableWidgetItem* pItem0 = new QTableWidgetItem(tr("segment"));
	QTableWidgetItem* pItem1 = new QTableWidgetItem("BillboardSprite");
	QTableWidgetItem* pItem2 = new QTableWidgetItem("0");
	QTableWidgetItem* pItem3 = new QTableWidgetItem("0");
	pItem0->setCheckState(Qt::Checked);

	pTable_->setRowHeight(10, row);
	pTable_->setItem(row, 0, pItem0);
	pTable_->setItem(row, 1, pItem1);
	pTable_->setItem(row, 2, pItem2);
	pTable_->setItem(row, 3, pItem3);
}

void SegmentListWidget::deleteSelectedStageClicked(void)
{
	// TODO.
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
		pVelGraph_->setSingleActiveSeries(true);
		pVelGraph_->createGraphs(2, 3);
		pVelGraph_->setSeriesName(0, "Forward");
		pVelGraph_->setSeriesName(1, "Right");
		pVelGraph_->setSeriesName(2, "Up");

		pForwardScale_ = new QSpinBox();
		pForwardScale_->setMinimumWidth(50);
		pForwardScale_->setMaximumWidth(50);
		pRightScale_ = new QSpinBox();
		pRightScale_->setMinimumWidth(50);
		pRightScale_->setMaximumWidth(50);
		pUpScale_ = new QSpinBox();
		pUpScale_->setMinimumWidth(50);
		pUpScale_->setMaximumWidth(50);

		QHBoxLayout* pFormLayout = new QHBoxLayout();
		pFormLayout->addWidget(new QLabel("Forward Scale"));
		pFormLayout->addWidget(pForwardScale_);
		pFormLayout->addWidget(new QLabel("Right Scale"));
		pFormLayout->addWidget(pRightScale_);
		pFormLayout->addWidget(new QLabel("Up Scale"));
		pFormLayout->addWidget(pUpScale_);
		pFormLayout->addStretch(1);

		pLayout->addWidget(pVelGraph_);
		pLayout->addLayout(pFormLayout);
	}

	setLayout(pLayout);
}


void VelocityGraph::setValue(const VelocityInfo& vel)
{
	pVelGraph_->setSeriesValue(0, vel.forward);
	pVelGraph_->setSeriesValue(1, vel.right);
	pVelGraph_->setSeriesValue(2, vel.up);

	pUpScale_->setValue(vel.up.scale);
	pForwardScale_->setValue(vel.forward.scale);
	pRightScale_->setValue(vel.right.scale);
}

void VelocityGraph::getValue(VelocityInfo& vel)
{
	pVelGraph_->getSeriesValue(0, vel.up);
	pVelGraph_->getSeriesValue(1, vel.forward);
	pVelGraph_->getSeriesValue(2, vel.right);

	vel.up.scale = pUpScale_->value();
	vel.forward.scale = pForwardScale_->value();
	vel.right.scale = pRightScale_->value();
}


// -----------------------------------


VelocityInfoWidget::VelocityInfoWidget(QWidget* parent) :
	QGroupBox("Velocity", parent)
{
	QFormLayout* pLayout = new QFormLayout();
	{
		QGroupBox* pMoveGroupBox = new QGroupBox("Relative to");
		{
			QFormLayout* pMoveLayout = new QFormLayout();

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
			

			pMoveGroupBox->setMaximumWidth(100);
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
		pRotationGraph_ = new GraphEditor(2, 1);

		pInitialRotation_ = new SpinBoxRange();
		pInitialRotation_->setMinimumWidth(80);


		QHBoxLayout* pFormLayout = new QHBoxLayout();
		pFormLayout->addWidget(new QLabel("Initial Rotation"));
		pFormLayout->addWidget(pInitialRotation_);
		pFormLayout->addStretch(1);

		pLayout->addLayout(pFormLayout);
		pLayout->addWidget(pRotationGraph_);
	}

	setLayout(pLayout);
}


void RotationGraphWidget::setValue(const RotationInfo& rot)
{
	pInitialRotation_->setValue(rot.initial);

}

void RotationGraphWidget::getValue(RotationInfo& rot)
{
	pInitialRotation_->getValue(rot.initial);


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
	pAlphaGraph_->setSeriesValue(0, col.alpha);
}

void AlphaGraph::getValue(ColorInfo& col)
{
	pAlphaGraph_->getSeriesValue(0, col.alpha);
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


		pMaterial_ = new QLineEdit();

		pLayout->addRow(tr("Type"), pType_);
		pLayout->addRow(tr("Material"), pMaterial_);
	}

	setLayout(pLayout);
}

void VisualsInfoWidget::setValue(const VisualsInfo& vis)
{
	pMaterial_->setText(vis.material);
	pType_->setCurrentIndex(static_cast<int32_t>(vis.type));
}

void VisualsInfoWidget::getValue(VisualsInfo& vis)
{
	vis.material = pMaterial_->text();
	
	auto idx = pType_->currentIndex();
	X_ASSERT(idx >= 0 && idx < engine::fx::StageType::ENUM_COUNT, "Invalid index for type combo")(idx, engine::fx::StageType::ENUM_COUNT);

	vis.type = static_cast<engine::fx::StageType::Enum>(idx);
}

// -----------------------------------



AssetFxWidget::AssetFxWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry),
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
		pSegments_ = new SegmentListWidget();
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
		pVerlocity_->setMinimumHeight(minHeight);
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

		// HEllloo JERRRYY!!!
		connect(pSegments_, &SegmentListWidget::selectionChanged, this, &AssetFxWidget::segmentSelectionChanged);


		setLayout(pTableLayout);
	}

	// setLayout(pLayout);

	setValue(value);
}


AssetFxWidget::~AssetFxWidget()
{

}


void AssetFxWidget::setValue(const std::string& value)
{
	X_UNUSED(value);

	blockSignals(true);
	

	blockSignals(false);
}

void AssetFxWidget::segmentSelectionChanged(void)
{
	// WHEN i sing, bitches come running.
	// just like when the segment selection changes we need to update all the values :(
	// i need to keep info for each segment then somehow set it :/

	int32_t curRow = pSegments_->currentRow();

	while(curRow >= segments_.size()) {
		segments_.push_back(std::make_unique<Segment>());
	}

	// now I want to update all the widgets!
	// potentially I could make all the fx data a model.
	// each row would be a segment.
	// then id use something like QDataWidgetMapper to map the data to the various widgets.
	//
	// but I think that may ed up a bit fiddly? (well more fiddly dunno)

	if (currentSegment_ >= 0)
	{
		auto& segment = segments_[currentSegment_];
		auto* pSeg = segment.get();

		pSapwn_->getValue(pSeg->spawn);
		pOrigin_->getValue(pSeg->origin);
		pSequence_->getValue(pSeg->seq);
		pVisualInfo_->getValue(pSeg->vis);
		pRotation_->getValue(pSeg->rot);
		pVerlocity_->getValue(pSeg->vel);
		pCol_->getValue(pSeg->col);
		pAlpha_->getValue(pSeg->col);
		pSize_->getValue(pSeg->size.size);
		pScale_->getValue(pSeg->size.scale);
	}

	{
		auto& segment = segments_[curRow];
		const auto* pSeg = segment.get();

		pSapwn_->setValue(pSeg->spawn);
		pOrigin_->setValue(pSeg->origin);
		pSequence_->setValue(pSeg->seq);
		pVisualInfo_->setValue(pSeg->vis);
		pRotation_->setValue(pSeg->rot);
		pVerlocity_->setValue(pSeg->vel);
		pCol_->setValue(pSeg->col);
		pAlpha_->setValue(pSeg->col);
		pSize_->setValue(pSeg->size.size);
		pScale_->setValue(pSeg->size.scale);
	}


	currentSegment_ = curRow;
}


X_NAMESPACE_END