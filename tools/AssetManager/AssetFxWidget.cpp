#include "AssetFxWidget.h"

#include <IAnimation.h>
#include "IAssetEntry.h"


#include <QtCharts\Qchartview>
#include <QtCharts\Qchart>
#include <QtCharts\QLineSeries>


// #include <QLineSerie>
#include <QTableWidget>

using namespace QtCharts;


X_NAMESPACE_BEGIN(assman)

SegmentList::SegmentList(QWidget* parent) :
	QWidget(parent)
{
	QVBoxLayout* pTableLayout = new QVBoxLayout();
	{
		pTable_ = new QTableWidget();

		QStringList labels;
		labels << "Name" << "Type";

		pTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
		pTable_->setSelectionMode(QAbstractItemView::SingleSelection);
		pTable_->setColumnCount(2);
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
	QTableWidgetItem* pItem1 = new QTableWidgetItem(tr("BillboardSprite"));
	pItem0->setCheckState(Qt::Checked);

	pTable_->setRowHeight(10, row);
	pTable_->setItem(row, 0, pItem0);
	pTable_->setItem(row, 1, pItem1);
}

void SegmentList::deleteSelectedStageClicked(void)
{
	// TODO.
}

// -----------------------------------

SpinBoxRange::SpinBoxRange(QWidget* parent) :
	QWidget(parent)
{
	QHBoxLayout* pLayout = new QHBoxLayout();
	pStart_ = new QSpinBox();
	pRange_ = new QSpinBox();

	QLabel* pLabel = new QLabel("+");

	pLayout->setContentsMargins(0,0,0,0);
	pLayout->addWidget(pStart_, 1);
	pLayout->addWidget(pLabel, 0);
	pLayout->addWidget(pRange_, 1);
	setLayout(pLayout);
}

// -----------------------------------

SpawnInfo::SpawnInfo(QWidget* parent) :
	QWidget(parent)
{
	QFormLayout* pLayout = new QFormLayout();
	pLayout->setLabelAlignment(Qt::AlignLeft);
	{
		// list of shit.
		pCount_ = new QSpinBox();
		pInterval_ = new QSpinBox();
		pLoopCount_ = new QSpinBox();
		pLife_ = new SpinBoxRange();
		pDelay_ = new SpinBoxRange();

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
	QWidget(parent)
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
	QWidget(parent)
{
	QFormLayout* pLayout = new QFormLayout();
	{
		pStart_ = new SpinBoxRange();
		pPlayRate_ = new SpinBoxRange();
		pLoopCount_ = new SpinBoxRange();

		pLayout->addRow(tr("Start"), pStart_);
		pLayout->addRow(tr("PlayRate"), pPlayRate_);
		pLayout->addRow(tr("Loop"), pLoopCount_);
	}

	setLayout(pLayout);
}

// -----------------------------------

GraphEditor::GraphEditor(QWidget* parent) :
	QWidget(parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();

	
	QLineSeries *series = new QLineSeries();
	series->append(0, 6);
	series->append(2, 4);
	series->append(3, 8);
	series->append(7, 4);
	series->append(10, 5);

//	connect(series, &QLineSeries::hovered, this, &View::tooltip);

	QtCharts::QChart* pChart = new QtCharts::QChart();
	pChart->legend()->hide();
	pChart->addSeries(series);
	pChart->createDefaultAxes();
	pChart->setTheme(QChart::ChartThemeDark);

	QtCharts::QChartView* pChartView = new QtCharts::QChartView(pChart);

	pChartView->setMinimumHeight(200);

	pLayout->addWidget(pChartView);

	setLayout(pLayout);

}

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

		pSapwn->setMaximumWidth(300);
		pOrigin->setMaximumWidth(300);
		pSeq->setMaximumWidth(300);

		GraphEditor* pGraph = new GraphEditor();

		pTableLayout->addWidget(pGraph);
		pTableLayout->addWidget(pSapwn);
		pTableLayout->addWidget(pOrigin);
		pTableLayout->addWidget(pSeq);
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