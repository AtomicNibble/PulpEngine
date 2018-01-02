#include "stdafx.h"
#include "OutputPaneManagerWidget.h"
#include "StyleUtilWidgets.h"

#include "IOutputPane.h"
#include "ICommand.h"
#include "ActionContainer.h"
#include "ActionManager.h"

#include "EditorManager.h"
#include "IEditor.h"

X_NAMESPACE_BEGIN(editor)


OutputPanePlaceHolder* OutputPanePlaceHolder::pCurrent_ = nullptr;

OutputPanePlaceHolder::OutputPanePlaceHolder(QSplitter* parent) :
	QWidget(parent), 
	pSplitter_(parent)
{
	setVisible(false);
	setLayout(new QVBoxLayout);
	QSizePolicy sp;
	sp.setHorizontalPolicy(QSizePolicy::Preferred);
	sp.setVerticalPolicy(QSizePolicy::Preferred);
	sp.setHorizontalStretch(0);
	setSizePolicy(sp);
	layout()->setMargin(0);
}

OutputPanePlaceHolder::~OutputPanePlaceHolder()
{
	if (pCurrent_ == this) {
		if (OutputPaneManager *om = OutputPaneManager::instance()) {
			om->setParent(0);
			om->hide();
		}
	}
}


void OutputPanePlaceHolder::maximizeOrMinimize(bool maximize)
{
	if (!pSplitter_) {
		return;
	}
	int32_t idx = pSplitter_->indexOf(this);
	if (idx < 0) {
		return;
	}

	QList<int32_t> sizes = pSplitter_->sizes();

	if (maximize) {
		lastNonMaxSize_ = sizes[idx];
		int32_t sum = 0;
		foreach(int s, sizes) {
			sum += s;
		}
		for (int32_t i = 0; i < sizes.count(); ++i) {
			sizes[i] = 32;
		}
		sizes[idx] = sum - (sizes.count() - 1) * 32;
	}
	else {
		int32_t target = lastNonMaxSize_ > 0 ? lastNonMaxSize_ : sizeHint().height();
		int32_t space = sizes[idx] - target;
		if (space > 0) {
			for (int32_t i = 0; i < sizes.count(); ++i) {
				sizes[i] += space / (sizes.count() - 1);
			}
			sizes[idx] = target;
		}
	}

	pSplitter_->setSizes(sizes);
}

bool OutputPanePlaceHolder::isMaximized(void) const
{
	return OutputPaneManager::instance()->isMaximized();
}

void OutputPanePlaceHolder::setDefaultHeight(int32_t height)
{
	if (height == 0) {
		return;
	}
	if (!pSplitter_) {
		return;
	}
	int32_t idx = pSplitter_->indexOf(this);
	if (idx < 0) {
		return;
	}

	pSplitter_->refresh();
	QList<int32_t> sizes = pSplitter_->sizes();
	int32_t difference = height - sizes.at(idx);
	if (difference <= 0) { // is already larger
		return;
	}
	for (int32_t i = 0; i < sizes.count(); ++i) {
		sizes[i] += difference / (sizes.count() - 1);
	}

	sizes[idx] = height;
	pSplitter_->setSizes(sizes);
}

void OutputPanePlaceHolder::ensureSizeHintAsMinimum(void)
{
	OutputPaneManager *om = OutputPaneManager::instance();
	int minimum = (pSplitter_->orientation() == Qt::Vertical
		? om->sizeHint().height() : om->sizeHint().width());
	setDefaultHeight(minimum);
}

void OutputPanePlaceHolder::unmaximize(void)
{
	if (OutputPaneManager::instance()->isMaximized()) {
		OutputPaneManager::instance()->slotMinMax();
	}
}

OutputPanePlaceHolder* OutputPanePlaceHolder::getCurrent(void)
{
	return pCurrent_;
}

bool OutputPanePlaceHolder::canMaximizeOrMinimize(void) const
{
	return pSplitter_ != nullptr;
}

bool OutputPanePlaceHolder::isCurrentVisible(void)
{
	return pCurrent_ && pCurrent_->isVisible();
}




// ------------------------------------------------------------

static OutputPaneManager* pInstance_ = nullptr;


OutputPaneManager::OutputPaneManager(QWidget *parent) :
	QWidget(parent),
	pOutputWidgetPane_(new QStackedWidget),
	maximised_(false),
	outputPaneHeight_(0),
	pToolBar_(nullptr)
{
	setWindowTitle(tr("Output"));

//	connect(ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));

	QVBoxLayout* mainlayout = new QVBoxLayout;
	mainlayout->setSpacing(0);
	mainlayout->setMargin(0);


	pOutputSlector_ = new QComboBox();
	pOutputSlector_->setProperty("hideicon", true);
	pOutputSlector_->setSizeAdjustPolicy(QComboBox::AdjustToContents);

	QLabel* text = new QLabel("Show output from:");
	text->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	text->setContentsMargins(4, 0, 5, 0);

	pToolBar_ = new StyledBar();
	QHBoxLayout *toolLayout = new QHBoxLayout(pToolBar_);
	toolLayout->setMargin(3);
	toolLayout->setSpacing(0);
	toolLayout->addSpacerItem(new QSpacerItem(1, 1));
	toolLayout->addWidget(text, 0, Qt::AlignVCenter);
	toolLayout->addWidget(pOutputSlector_, 1, Qt::AlignVCenter);
	toolLayout->addWidget(new StyledSeparator());


	mainlayout->addWidget(pToolBar_);
	mainlayout->addWidget(pOutputWidgetPane_, 10);
	setLayout(mainlayout);

	connect(pOutputSlector_, SIGNAL(currentIndexChanged(int)),
		this, SLOT(setCurrentIndex(int)));
}

OutputPaneManager::~OutputPaneManager()
{
}


void OutputPaneManager::create(void)
{
	pInstance_ = new OutputPaneManager;
}

void OutputPaneManager::destroy(void)
{
	delete pInstance_;
	pInstance_ = nullptr;
}


void OutputPaneManager::init(void)
{
	ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);
	const Context globalContext(Constants::C_GLOBAL);

	const char* ACTIONS_GROUP = "OutputPane.ActionsGroup";
	const char* PANES_GROUP = "OutputPane.PanesGroup";

	// Window->Output Panes
	ActionContainer *mpanes = ActionManager::createMenu(Constants::M_WINDOW_PANES);
	mwindow->addMenu(mpanes, Constants::G_WINDOW_PANES);
	mpanes->menu()->setTitle(tr("Output &Panes"));
	mpanes->appendGroup(ACTIONS_GROUP);
	mpanes->appendGroup(PANES_GROUP);

	const int32_t numPanes = panes_.size();

	int32_t shortcutNumber = 1;

	const Id baseId = "Pane.";
	for (int32_t i = 0; i != numPanes; ++i)
	{
		IOutputPane* pOutPane = panes_.at(i);
		const int32_t idx = pOutputWidgetPane_->addWidget(pOutPane->outputWidget(this));
		BUG_CHECK(idx == i);

		connect(pOutPane, SIGNAL(showPage(int32_t)), this, SLOT(showPage(int32_t)));
		connect(pOutPane, SIGNAL(hidePage()), this, SLOT(slotHide()));
		connect(pOutPane, SIGNAL(togglePage(int32_t)), this, SLOT(togglePage(int32_t)));
		connect(pOutPane, SIGNAL(navigateStateUpdate()), this, SLOT(updateNavigateState()));


		//    minTitleWidth = qMax(minTitleWidth, titleFm.width(outPane->displayName()));
		QString suffix = pOutPane->displayName().simplified();
		suffix.remove(QLatin1Char(' '));

		const Id id = baseId.withSuffix(suffix);
		QAction* pSwitchToAction = new QAction(pOutPane->displayName(), this);
		ICommand* pSwitchToCmd = ActionManager::registerAction(pSwitchToAction, id, globalContext);

		mpanes->addAction(pSwitchToCmd, PANES_GROUP);
		actions_.append(pSwitchToAction);

		ids_.append(id);

		pSwitchToCmd->setDefaultKeySequence(QKeySequence(Qt::ALT | (Qt::Key_0 + shortcutNumber)));
		++shortcutNumber;

		// add item to combo
		pOutputSlector_->addItem(pOutPane->displayName());


		connect(pSwitchToAction, SIGNAL(triggered()), this, SLOT(shortcutTriggered()));
	}

	if (numPanes == 1) {
		pToolBar_->hide();
	}
}

void OutputPaneManager::addPane(IOutputPane* pPane)
{
	if (!panes_.contains(pPane)) {
		panes_.append(pPane);
	}
}


OutputPaneManager* OutputPaneManager::instance(void)
{
	return pInstance_;
}


void OutputPaneManager::updateStatusButtons(bool visible)
{
	const int32_t idx = currentIndex();
	if (idx == -1) {
		return;
	}

	panes_.at(idx)->visibilityChanged(visible);
}


bool OutputPaneManager::isMaximized(void) const
{
	return maximised_;
}


void OutputPaneManager::showPage(int32_t idx, int32_t flags)
{
	BUG_ASSERT(idx >= 0, return);

	OutputPanePlaceHolder* ph = OutputPanePlaceHolder::getCurrent();

	if (!ph && flags & IOutputPane::ModeSwitch) {
		// In this mode we don't have a placeholder
		// switch to the output mode and switch the page
		//    ModeManager::activateMode(Id(Constants::MODE_EDIT));
		ph = OutputPanePlaceHolder::getCurrent();
	}

	bool onlyFlash = !ph || (
			panes_.at(currentIndex())->hasFocus()
			&& !(flags & IOutputPane::WithFocus)
			&& idx != currentIndex()
		);

	if (onlyFlash) {
		//        buttons_.value(idx)->flash();
	}
	else {
		// make the page visible
		ph->setVisible(true);

		ensurePageVisible(idx);
		IOutputPane *out = panes_.at(idx);
		out->visibilityChanged(true);
		if (flags & IOutputPane::WithFocus && out->canFocus()) {
			out->setFocus();
			ICore::raiseWindow(pOutputWidgetPane_);
		}

		ph->setDefaultHeight(outputPaneHeight_);
		if (flags & IOutputPane::EnsureSizeHint) {
			ph->ensureSizeHintAsMinimum();
		}
	}
}


void OutputPaneManager::ensurePageVisible(int32_t idx)
{
	int current = currentIndex();
	if (current != idx) {
		pOutputWidgetPane_->setCurrentIndex(idx);
	}
	setCurrentIndex(idx);
}

int32_t OutputPaneManager::findIndexForPage(IOutputPane *out)
{
	return panes_.indexOf(out);
}

int32_t OutputPaneManager::currentIndex(void) const
{
	return pOutputWidgetPane_->currentIndex();
}


void OutputPaneManager::slotHide(void)
{
	OutputPanePlaceHolder* ph = OutputPanePlaceHolder::getCurrent();
	if (ph) 
	{
		ph->setVisible(false);

		const int32_t idx = currentIndex();

		BUG_ASSERT(idx >= 0, return);

		//       buttons_.at(idx)->setChecked(false);
		panes_.value(idx)->visibilityChanged(false);

		if (IEditor* pEditor = EditorManager::currentEditor()) 
		{
			QWidget* w = pEditor->widget()->focusWidget();
			if (!w) {
				w = pEditor->widget();
			}
			w->setFocus();
		}
	}
}


void OutputPaneManager::slotNext(void)
{
	const int32_t idx = currentIndex();
	ensurePageVisible(idx);
	IOutputPane *out = panes_.at(idx);
	if (out->canNext()) {
		out->goToNext();
	}
}

void OutputPaneManager::slotPrev(void)
{
	const int32_t idx = currentIndex();
	ensurePageVisible(idx);
	IOutputPane *out = panes_.at(idx);
	if (out->canPrevious()) {
		out->goToPrev();
	}
}


void OutputPaneManager::shortcutTriggered(void)
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	BUG_ASSERT(pAction, return);
	int32_t idx = actions_.indexOf(pAction);
	BUG_ASSERT(idx != -1, return);
	IOutputPane *outputPane = panes_.at(idx);
	// Now check the special case, the output window is already visible,
	// we are already on that page but the outputpane doesn't have focus
	// then just give it focus.
	const int32_t current = currentIndex();
	if (OutputPanePlaceHolder::isCurrentVisible() && current == idx) 
	{
		if (!outputPane->hasFocus() && outputPane->canFocus()) {
			outputPane->setFocus();
			ICore::raiseWindow(pOutputWidgetPane_);
		}
		else {
			slotHide();
		}
	}
	else {
		setCurrentIndex(idx);
	}
}

void OutputPaneManager::slotMinMax(void)
{
	OutputPanePlaceHolder *ph = OutputPanePlaceHolder::getCurrent();
	BUG_ASSERT(ph, return);

	if (!ph->isVisible()) {// easier than disabling/enabling the action
		return;
	}

	maximised_ = !maximised_;
}


void OutputPaneManager::focusInEvent(QFocusEvent *e)
{
	if (QWidget *w = pOutputWidgetPane_->currentWidget()) {
		w->setFocus(e->reason());
	}
}

void OutputPaneManager::resizeEvent(QResizeEvent *e)
{
	if (e->size().height() == 0) {
		return;
	}
	outputPaneHeight_ = e->size().height();
}


void OutputPaneManager::showPage(int32_t flags)
{
	int32_t idx = findIndexForPage(qobject_cast<IOutputPane*>(sender()));
	showPage(idx, flags);
}

void OutputPaneManager::togglePage(int32_t flags)
{
	int32_t idx = findIndexForPage(qobject_cast<IOutputPane*>(sender()));
	if (OutputPanePlaceHolder::isCurrentVisible() && currentIndex() == idx) {
		slotHide();
	}
	else {
		showPage(idx, flags);
	}
}

void OutputPaneManager::clearPage(void)
{
	int32_t idx = currentIndex();
	if (idx >= 0) {
		panes_.at(idx)->clearContents();
	}
}


void OutputPaneManager::updateNavigateState(void)
{
	IOutputPane *pane = qobject_cast<IOutputPane*>(sender());
	int32_t idx = findIndexForPage(pane);
	if (currentIndex() == idx) {
		    // nothing for now.
	}
}

void OutputPaneManager::popupMenu(void)
{
	QMenu menu;
	int32_t idx = 0;
	foreach(IOutputPane *pane, panes_)
	{
		QAction *act = menu.addAction(pane->displayName());
		act->setCheckable(true);
		act->setData(idx);
		++idx;
	}

	QAction *result = menu.exec(QCursor::pos());
	if (!result) {
		return;
	}

	idx = result->data().toInt();
}



void OutputPaneManager::setCurrentIndex(int32_t idx)
{
	static int lastIndex = -1;

	if (lastIndex != -1) {
		panes_.at(lastIndex)->visibilityChanged(false);
	}

	if (idx != -1) 
	{
		pOutputWidgetPane_->setCurrentIndex(idx);

		IOutputPane *pane = panes_.at(idx);
		pane->visibilityChanged(true);


		if (pOutputSlector_->currentIndex() != idx) {
			pOutputSlector_->setCurrentIndex(idx);
		}

		//        m_prevAction->setEnabled(canNavigate && pane->canPrevious());
		//        m_nextAction->setEnabled(canNavigate && pane->canNext());
		//        m_buttons.at(idx)->setChecked(OutputPanePlaceHolder::isCurrentVisible());
		//        m_titleLabel->setText(pane->displayName());
	}

	lastIndex = idx;
}
















X_NAMESPACE_END
