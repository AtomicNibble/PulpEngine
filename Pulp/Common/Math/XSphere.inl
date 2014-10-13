


X_INLINE bool Sphere::intersects(const Ray &ray)
{
	float 		t;
	Vec3f		temp = ray.getOrigin() - Center_;
	float 		a = ray.getDirection().dot(ray.getDirection());
	float 		b = 2.0f * temp.dot(ray.getDirection());
	float 		c = temp.dot(temp) - Radius_ * Radius_;
	float 		disc = b * b - 4.0f * a * c;

	if (disc < 0.0f) {
		return false;
	}
	else { // this probably can be optimized
		float e = math<float>::sqrt(disc);
		float denom = 2.0f * a;
		t = (-b - e) / denom;    // smaller root

		if (t > EPSILON_VALUE) {
			return true;
		}

		t = (-b + e) / denom;    // larger root
		if (t > EPSILON_VALUE) {
			return true;
		}
	}

	return false;
}

X_INLINE bool Sphere::intersect(const Ray &ray, float *intersection)
{
	float 		t;
	Vec3f		temp = ray.getOrigin() - Center_;
	float 		a = ray.getDirection().dot(ray.getDirection());
	float 		b = 2.0f * temp.dot(ray.getDirection());
	float 		c = temp.dot(temp) - Radius_ * Radius_;
	float 		disc = b * b - 4.0f * a * c;

	if (disc < 0.0f) {
		return false;
	}
	else {
		float e = math<float>::sqrt(disc);
		float denom = 2.0f * a;
		t = (-b - e) / denom;    // smaller root

		if (t > EPSILON_VALUE) {
			*intersection = t;
			return true;
		}

		t = (-b + e) / denom;    // larger root
		if (t > EPSILON_VALUE) {
			*intersection = t;
			return true;
		}
	}

	return false;
}