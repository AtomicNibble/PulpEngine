#pragma once

#include "LvlFmts\LvlSource.h"

X_NAMESPACE_BEGIN(level)

struct LvlEntity;

namespace mapFile
{
    class MapFileSource : public LvlSource
    {
    public:
        using LvlSource::LvlSource;
        virtual ~MapFileSource() X_FINAL = default;

        bool load(core::Path<char>& path);

    private:
        bool processMapEntity(LvlEntity& ent, XMapEntity* mapEnt);
        bool processBrush(LvlEntity& ent, XMapBrush& brush, size_t entIdx);
        bool processPatch(LvlEntity& ent, XMapPatch& patch, size_t entIdx);

    private:
    };

} // namespace mapFile

X_NAMESPACE_END