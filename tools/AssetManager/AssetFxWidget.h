#pragma once


#include <QObject>

X_NAMESPACE_BEGIN(assman)

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



class SpinBoxRange : public QWidget
{
	Q_OBJECT
public:
	SpinBoxRange(QWidget* parent = nullptr);


private:
	QSpinBox* pStart_;
	QSpinBox* pRange_;
};


class SpawnInfo : public QWidget
{
	Q_OBJECT

public:
	SpawnInfo(QWidget *parent = nullptr);

private slots:

private:
	QSpinBox * pCount_;
	QSpinBox* pInterval_;
	QSpinBox* pLoopCount_;
	SpinBoxRange* pLife_;
	SpinBoxRange* pDelay_;
};

class OriginInfo : public QWidget
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


class SequenceInfo : public QWidget
{
	Q_OBJECT

public:
	SequenceInfo(QWidget *parent = nullptr);

	private slots:

private:
	SpinBoxRange* pStart_;
	SpinBoxRange* pPlayRate_;
	SpinBoxRange* pLoopCount_;
};


class GraphEditor : public QWidget
{
	Q_OBJECT

public:
	GraphEditor(QWidget *parent = nullptr);

	private slots:

private:

};


X_NAMESPACE_END