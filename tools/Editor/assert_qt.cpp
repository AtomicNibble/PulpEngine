#include "assert_qt.h"

#include <QDebug>

namespace Internal {

void writeAssertLocation(const char *msg)
{
    qDebug("SOFT ASSERT: %s", msg);
}

} // namespace Internal
