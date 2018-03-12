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

// -----------------------------------


GraphEditorView::GraphEditorView(QWidget *parent) :
	QChartView(parent),
	mouseActive_(false),
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
		pRedoAction_->setShortcutVisibleInContextMenu(true);
		pRedoAction_->setShortcuts(QKeySequence::Redo);

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
			if (i == activeSeries_)
			{
				pen.setColor(colors_[i]);
			}
			else
			{
				pen.setColor(colors_[i].darker(DarkenFactor));
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

	pen.setStyle(Qt::SolidLine);

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
		}
		else
		{
			pen.setColor(colors_[i].darker(DarkenFactor));
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


void GradientWidget::paintEvent(QPaintEvent*)
{
	QLinearGradient grad(0,0, width(), height());
	grad.setColorAt(0, Qt::red);
	grad.setColorAt(0.5, Qt::white);
	grad.setColorAt(1, Qt::green);

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

	pGradient_ = new GradientWidget();
	pGradient_->setMinimumHeight(10);

	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pLayout->addWidget(pGraph_);
		pLayout->addWidget(pGradient_);
	}

	setLayout(pLayout);
}


// -----------------------------------


GraphWithScale::GraphWithScale(const QString& label, QWidget* parent) :
	QGroupBox(label, parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pSizeGraph_ = new GraphEditor(2, 1);
		pScale_ = new QSpinBox();
		pScale_->setMaximumWidth(50);

		pLayout->addWidget(pSizeGraph_);

		QFormLayout* pFormLayout = new QFormLayout();
		pFormLayout->addRow(tr("Scale"), pScale_);

		pLayout->addLayout(pFormLayout);
	}

	setLayout(pLayout);
}

// -----------------------------------


SegmentList::SegmentList(QWidget* parent) :
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

		connect(pTable_, &QTableWidget::itemSelectionChanged, this, &SegmentList::itemSelectionChanged);

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

		connect(pAdd, &QPushButton::clicked, this, &SegmentList::addStageClicked);
		connect(pDelete_, &QPushButton::clicked, this, &SegmentList::deleteSelectedStageClicked);

		pButtonLayout->addWidget(pAdd);
		pButtonLayout->addWidget(pDelete_);
		pButtonLayout->addStretch();
	}

	pTableLayout->addLayout(pButtonLayout);

	setLayout(pTableLayout);
}

void SegmentList::itemSelectionChanged(void)
{
	const int num = pTable_->selectedItems().size();

	pDelete_->setEnabled(num != 0);
}

void SegmentList::addStageClicked(void)
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

void SegmentList::deleteSelectedStageClicked(void)
{
	// TODO.
}

// -----------------------------------

SpawnInfo::SpawnInfo(QWidget* parent) :
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

// -----------------------------------

OriginInfo::OriginInfo(QWidget* parent) :
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

// -----------------------------------


SequenceInfo::SequenceInfo(QWidget* parent) :
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

// -----------------------------------


VelocityGraph::VelocityGraph(QWidget* parent) :
	QGroupBox(parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	{
		pVelGraph_ = new GraphEditor(6, 1);
	
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


// -----------------------------------


VelocityInfo::VelocityInfo(QWidget* parent) :
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

		VelocityGraph* pGraph = new VelocityGraph();

		pLayout->addWidget(pMoveGroupBox);
		pLayout->addWidget(pGraph);
	}

	setLayout(pLayout);
}


// -----------------------------------

RotationGraph::RotationGraph(QWidget *parent) :
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



// -----------------------------------


VisualsInfo::VisualsInfo(QWidget* parent) :
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


// -----------------------------------



AssetFxWidget::AssetFxWidget(QWidget *parent, IAssetEntry* pAssEntry, const std::string& value) :
	QWidget(parent),
	pAssEntry_(pAssEntry)
{
//	QHBoxLayout* pLayout = new QHBoxLayout();
//	pLayout->setContentsMargins(0, 0, 0, 0);

	// gonna get crazy up in here!
	// need something to manage stages.
	{
		QVBoxLayout* pTableLayout = new QVBoxLayout();

		SpawnInfo* pSapwn = new SpawnInfo();
		OriginInfo* pOrigin = new OriginInfo();
		SegmentList* pSegments = new SegmentList();
		SequenceInfo* pSeq = new SequenceInfo();
		GraphWithScale* pSize = new GraphWithScale("Size");
		GraphWithScale* pScale = new GraphWithScale("Scale");
		VisualsInfo* pVisual = new VisualsInfo();
		VelocityInfo* pVelocity = new VelocityInfo();
		RotationGraph* pRot = new RotationGraph();
		ColorGraph* pCol = new ColorGraph();
		AlphaGraph* pAlpha = new AlphaGraph();

		const int minHeight = 300;
		const int minWidth = 300;
		const int maxWidth = 400;

		pSize->setMinimumWidth(300);
		pScale->setMinimumWidth(300);
		pVelocity->setMinimumWidth(300);
		pRot->setMinimumWidth(300);
		pCol->setMinimumWidth(300);
		pAlpha->setMinimumWidth(300);

		pSize->setMinimumHeight(minHeight);
		pScale->setMinimumHeight(minHeight);
		pVelocity->setMinimumHeight(minHeight);
		pRot->setMinimumHeight(minHeight);
		pCol->setMinimumHeight(minHeight);
		pAlpha->setMinimumHeight(minHeight);

		pSize->setMaximumWidth(maxWidth);
		pScale->setMaximumWidth(maxWidth);
		pVelocity->setMaximumWidth(maxWidth);
		pRot->setMaximumWidth(maxWidth);
		pVisual->setMaximumWidth(maxWidth);
		pSapwn->setMaximumWidth(maxWidth);
		pOrigin->setMaximumWidth(maxWidth);
		pSeq->setMaximumWidth(maxWidth);
		pCol->setMaximumWidth(maxWidth);
		pAlpha->setMaximumWidth(maxWidth);

		QHBoxLayout* pSizeLayout = new QHBoxLayout();
		pSizeLayout->setContentsMargins(0,0,0,0);
		pSizeLayout->addWidget(pSize);
		pSizeLayout->addWidget(pScale);
		pSizeLayout->addStretch(0);

		pTableLayout->addWidget(pVisual);
		pTableLayout->addWidget(pSapwn);
		pTableLayout->addWidget(pOrigin);
		pTableLayout->addWidget(pSeq);
		pTableLayout->addWidget(pRot);
		pTableLayout->addLayout(pSizeLayout);
		pTableLayout->addWidget(pCol);
		pTableLayout->addWidget(pAlpha);
		pTableLayout->addWidget(pVelocity);
		pTableLayout->addWidget(pSegments);

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


X_NAMESPACE_END