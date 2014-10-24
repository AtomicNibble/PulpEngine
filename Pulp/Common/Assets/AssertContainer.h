#pragma once


#ifndef X_BASE_ASSET_H_
#define X_BASE_ASSET_H_

#include <Threading\AtomicInt.h>
#include <Containers\HashMap.h>
#include <Containers\Array.h>

#include <String\Path.h>

X_NAMESPACE_BEGIN(core)

class XResourceContainer;

class XBaseAsset
{
	friend class XResourceContainer;

public:
	XBaseAsset() : RefCount_(1), pContainer_(nullptr) {}
	XBaseAsset(const XBaseAsset& Src);
	XBaseAsset& operator=(const XBaseAsset& Src);

	virtual ~XBaseAsset() {};

	X_INLINE int getID() const { return ID_; }
	X_INLINE void setID(int nID) { ID_ = nID; }
	X_INLINE const char* resourceName(void) const { return resourceName_.c_str(); }

	virtual bool isValid() const {
		return false;
	}

	virtual const int addRef() {
		++RefCount_;
		return RefCount_;
	}
	virtual const int release();
	virtual const int getRefCounter() const { return RefCount_; }


protected:
	uint32_t ID_;
	core::AtomicInt RefCount_; // thread safe.
	core::string resourceName_;
	XResourceContainer* pContainer_;
};



class XResourceContainer
{
public:
	friend class XBaseAsset;

public:
	XResourceContainer(core::MemoryArenaBase* arena, size_t size) :
		list(arena),
		hash(arena)
	{
		if (arena) {
			list.reserve(size);
			hash.reserve(size);
		}
	}

	void setArena(core::MemoryArenaBase* arena, size_t size) {
		X_ASSERT_NOT_NULL(arena);

		list.setArena(arena);
		hash.setArena(arena);

		list.reserve(size);
		hash.reserve(size);
	}

	void free(void) {
		list.free();
		hash.free();
	}

	XBaseAsset* findAsset(const char* name) const {
		X_ASSERT_NOT_NULL(name);
		X_ASSERT(core::strUtil::Find(name, name + strlen(name), '\\') == nullptr,
			"asset name must have forward slash")(name);

		ResourceConstItor it = hash.find(X_CONST_STRING(name));
		if (it != hash.end())
			return it->second;
		return nullptr; // O'Deer
	}

	// fast id maps to index.
	XBaseAsset* findAsset(uint32_t id) {
		if (id < 0 || id > list.size()) // needed for release?
			return nullptr; // O'Deer
		return list[id];
	}

	void AddAsset(const char* name, XBaseAsset* pAsset) {
		X_ASSERT_NOT_NULL(name);
		X_ASSERT_NOT_NULL(pAsset);
		X_ASSERT(core::strUtil::Find(name, name + strlen(name), '\\') == nullptr,
			"asset name must have forward slash")(name);

		hash.insert(ResourceItor::value_type(name, pAsset));

		uint32_t id = safe_static_cast<uint32_t, size_t>(list.size());
		pAsset->setID(id);
		pAsset->resourceName_ = name;
		pAsset->pContainer_ = this;

		// the id is the index of the asset 
		// allowing fast retrival of the asset by id.
		list.append(pAsset);
	}

protected:

	virtual bool removeAsset(XBaseAsset* pAsset);

private:
	typedef core::Array<XBaseAsset*> ResourceList;
	typedef core::HashMap<core::string, XBaseAsset*> ResourceMap;

	ResourceList list;
	ResourceMap hash;

public:
	typedef ResourceMap::iterator ResourceItor;
	typedef ResourceMap::const_iterator ResourceConstItor;

	ResourceItor begin(void) {
		return hash.begin();
	}

	ResourceItor end(void) {
		return hash.end();
	}

	const size_t size(void) const {
		return list.size();
	}

};




X_NAMESPACE_END

#endif // !X_BASE_ASSET_H_