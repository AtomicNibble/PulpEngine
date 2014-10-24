#include <EngineCommon.h>
#include "XSphere.h"
#include "XAabb.h"


Sphere::Sphere(const AABB& box) : 
	Center_(box.center()), 
	Radius_(box.radius())
{

}
