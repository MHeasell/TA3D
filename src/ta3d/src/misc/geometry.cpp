#include "geometry.h"

namespace TA3D
{
	Ray3D::Ray3D(Vector3D position, Vector3D direction) : origin(position), direction(direction) {}

	Vector3D Ray3D::pointAt(const float t) const
	{
		return origin + (t * direction);
	}

	Plane3D Plane3D::fromPoints(Vector3D a, Vector3D b, Vector3D c)
	{
		auto position = a;

		auto v1 = b - a;
		auto v2 = c - a;

		auto normal = v1.cross(v2);

		return Plane3D(position, normal);
	}

	float Plane3D::intersect(const Ray3D& ray) const
	{
		float a = (point - ray.origin).dot(normal);
		float b = ray.direction.dot(normal);
		return a / b;
	}

	bool Plane3D::isInFront(const Vector3D& p) const
	{
		return (p - point).dot(normal) > 0;
	}
}
