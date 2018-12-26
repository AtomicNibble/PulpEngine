#include "stdafx.h"
#include "Localisation.h"


X_NAMESPACE_BEGIN(locale)


Localisation::Localisation(core::MemoryArenaBase* arena) :
    ht_(arena, core::bitUtil::NextPowerOfTwo(MAX_STRINGS))
{

}

Localisation::~Localisation()
{

}


Localisation::string_view Localisation::getString(Key k) const
{
    auto it = ht_.find(k);
    if (it != ht_.end()) {
        return string_view(it->second.begin(), it->second.length());
    }

    return {};
}



X_NAMESPACE_END
