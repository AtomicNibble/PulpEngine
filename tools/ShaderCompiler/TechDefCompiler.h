#pragma once


X_NAMESPACE_BEGIN(engine)

namespace compiler
{

	class TechDefCompiler
	{
	public:
		TechDefCompiler(core::MemoryArenaBase* arena);
		~TechDefCompiler();

		void PrintBanner(void);
		bool Init(void);

		bool ConvertAll(void);
		bool CleanAll(void);


	private:

	};

} // namespace compiler

X_NAMESPACE_END