#pragma once

#include <QAbstractTableModel>

#include <IEffect.h>


X_NAMESPACE_BEGIN(assman)

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


struct RangeDouble
{
	RangeDouble() :
		start(0),
		range(0)
	{
	}

	float64_t start;
	float64_t range;
};

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

struct GraphData
{
	SeriesDataArr series;
};

typedef std::vector<GraphData> GraphDataArr;

// Graph info contains N graphs each with M series.
struct GraphInfo
{
	GraphInfo() :
		scale(0),
		random(false)
	{
	}

	float scale;
	bool random;
	bool _pad[3];
	GraphDataArr graphs;
};

struct RotationInfo
{
	GraphInfo rot;
	RangeDouble initial;
	RangeDouble pitch;
	RangeDouble yaw;
	RangeDouble roll;
};

struct SizeInfo
{
	SizeInfo() :
		size2Enabled(false)
	{
	}

	GraphInfo size0;
	GraphInfo size1;
	GraphInfo scale;

	bool size2Enabled;
};

struct ColorInfo
{
	GraphInfo alpha;
	GraphInfo col;
};

struct VelocityGraphInfo
{
	VelocityGraphInfo() :
		relative(false)
	{
	}

	GraphInfo graph;

	float forwardScale;
	float rightScale;
	float upScale;

	bool relative;
};


struct VelocityInfo
{
	VelocityInfo() :
		postionType(engine::fx::RelativeTo::Spawn)
	{
	}

	engine::fx::RelativeTo::Enum postionType;

	VelocityGraphInfo vel0;
	VelocityGraphInfo vel1;
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
	enum class OffsetType : uint8_t
	{
		None,
		Spherical,
		Cylindrical
	};

	OriginInfo() :
		relative(false),
		offsetType(OffsetType::None)
	{
	}

	RangeDouble spawnOrgX;
	RangeDouble spawnOrgY;
	RangeDouble spawnOrgZ;

	RangeDouble spawnRadius;
	RangeDouble spawnHeight;

	bool relative;
	OffsetType offsetType;
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
	typedef std::vector<QString> QStringArr;

	VisualsInfo() :
		type(engine::fx::StageType::BillboardSprite)
	{
	}

	QStringArr materials;
	engine::fx::StageType::Enum type;
};

struct Segment
{
	QString name;
	bool enabled;

	SpawnInfo spawn;
	OriginInfo origin;
	SequenceInfo seq;
	VisualsInfo vis;
	VelocityInfo vel;

	RotationInfo rot;
	SizeInfo size;
	ColorInfo col;
};


class FxSegmentModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	typedef std::unique_ptr<Segment> SegmentPtr;
	typedef std::vector<SegmentPtr> SegmentArr;

public:
	FxSegmentModel(QObject *parent = nullptr);

	void getJson(core::string& json) const;
	bool fromJson(const core::string & json);

	void addSegment(void);
	void duplicateSegment(size_t idx);

	size_t numSegments(void) const;
	Segment& getSegment(size_t idx);
	void setSegmentType(size_t idx, engine::fx::StageType::Enum type);

public:
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
	SegmentArr segments_;
};


X_NAMESPACE_END