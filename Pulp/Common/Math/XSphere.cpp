#include <EngineCommon.h>
#include "XSphere.h"
#include "XAabb.h"

Sphere::Sphere(const AABB& box) :
    center_(box.center()),
    radius_(box.radius())
{
}
