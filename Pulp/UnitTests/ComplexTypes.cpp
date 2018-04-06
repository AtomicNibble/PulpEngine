#include "stdafx.h"
#include "ComplexTypes.h"

namespace testTypes
{
    int CONSRUCTION_COUNT = 0;
    int DECONSRUCTION_COUNT = 5000;
    int MOVE_COUNT = 10000;

    void resetConConters(void)
    {
        CONSRUCTION_COUNT = 0;
        DECONSRUCTION_COUNT = 0;
        MOVE_COUNT = 0;
    }

} // namespace testTypes