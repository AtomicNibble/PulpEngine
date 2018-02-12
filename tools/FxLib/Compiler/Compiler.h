#pragma once


#include <Containers\ByteStream.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(engine)

namespace fx
{
	template<typename T>
	struct GraphBuilder
	{
		struct DataPoint {
			DataPoint(float t, T d) : 
				time(t), data(d) 
			{}

			float time;
			T data;
		};

		typedef core::Array<DataPoint> DataPointArr;

		GraphBuilder(core::MemoryArenaBase* arena) :
			points(arena)
		{}

		DataPointArr points;
	};

	template<typename T>
	struct GraphSet
	{
		typedef T Type;
		typedef GraphBuilder<T> GraphT;
		typedef core::Array<GraphT> GraphArr;

		GraphSet(core::MemoryArenaBase* arena) :
			graphs(arena)
		{}

		float scale;
		GraphArr graphs;
	};

	typedef GraphSet<Vec3f> ColGraphSet;
	typedef GraphSet<float> FloatGraphSet;

	struct StageBuilder : public Stage
	{
		typedef core::Array<core::string> StrArr;

		StageBuilder(core::MemoryArenaBase* arena) :
			materials(arena),
			color(arena),
			alpha(arena),
			size(arena),
			scale(arena),
			rot(arena),
			vel0X(arena),
			vel0Y(arena),
			vel0Z(arena)
		{}

		StrArr materials;

		ColGraphSet color;
		FloatGraphSet alpha;
		FloatGraphSet size;
		FloatGraphSet scale;
		FloatGraphSet rot;

		FloatGraphSet vel0X;
		FloatGraphSet vel0Y;
		FloatGraphSet vel0Z;
	};

	class EffectCompiler
	{
		typedef core::Array<uint8_t> DataVec;
		typedef core::Array<StageBuilder> StageBuilderArr;


	public:
		EffectCompiler(core::MemoryArenaBase* arena);
		~EffectCompiler();

		bool loadFromJson(core::string& str);
		bool writeToFile(core::XFile* pFile) const;

	private:
		template<typename GraphSetT>
		bool parseGraphFloat(core::json::Document::ValueType& p, const char* pName, GraphSetT& graphSet);

		template<typename GraphSetT, typename FunT>
		bool parseGraph(core::json::Document::ValueType& p, const char* pName, GraphSetT& graphSet, FunT objectParseFunc);
		
	private:
		core::MemoryArenaBase* arena_;
		StageBuilderArr stages_;
	};

} // namespace fx


X_NAMESPACE_END