#include "assert.h"

#include <QDebug>

namespace Utils {

void writeAssertLocation(const char *msg)
{
    qDebug("SOFT ASSERT: %s", msg);
}

} // namespace Utils
