#pragma once


X_NAMESPACE_BEGIN(render)

namespace texture
{


	class TextureManager
	{
	public:
		TextureManager(core::MemoryArenaBase* arena);
		~TextureManager();

		bool init(void);
		bool shutDown(void);



	private:
		core::MemoryArenaBase* arena_;
	};


} // namespace


X_NAMESPACE_END