
X_NAMESPACE_BEGIN(lvl)

namespace mapFile
{


	X_INLINE size_t XMapFile::getNumEntities(void) const 
	{ 
		return entities_.size();
	}

	X_INLINE XMapEntity* XMapFile::getEntity(size_t i) const 
	{
		return entities_[i]; 
	}

	X_INLINE const XMapFile::PrimTypeNumArr& XMapFile::getPrimCounts(void) const 
	{ 
		return primCounts_; 
	}


} // namespae mapFile

X_NAMESPACE_END
