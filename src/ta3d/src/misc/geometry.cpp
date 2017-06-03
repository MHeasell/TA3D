#include "geometry.h"
#include <cmath>

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

	BoundingBox3D::IntersectResult BoundingBox3D::intersect(const Ray3D& ray) const
	{
		Plane3D bottomPlane(center + Vector3D(0.0f, 0.0f, -extents.z), Vector3D(0.0f, 0.0f, 1.0f));
		Plane3D topPlane(center + Vector3D(0.0f, 0.0f, extents.z), Vector3D(0.0f, 0.0f, 1.0f));

		float bottomIntersect = bottomPlane.intersect(ray);
		float topIntersect = topPlane.intersect(ray);

		float zEnter;
		float zExit;
		if (topIntersect > bottomIntersect)
		{
			zEnter = bottomIntersect;
			zExit = topIntersect;
		}
		else
		{
			zEnter = topIntersect;
			zExit = bottomIntersect;
		}

		Plane3D leftPlane(center + Vector3D(-extents.x, 0.0f, 0.0f), Vector3D(1.0f, 0.0f, 0.0f));
		Plane3D rightPlane(center + Vector3D(extents.x, 0.0f, 0.0f), Vector3D(1.0f, 0.0f, 0.0f));

		float leftIntersect = leftPlane.intersect(ray);
		float rightIntersect = rightPlane.intersect(ray);

		float xEnter;
		float xExit;
		if (rightIntersect > leftIntersect)
		{
			xEnter = leftIntersect;
			xExit = rightIntersect;
		}
		else
		{
			xEnter = rightIntersect;
			xExit = leftIntersect;
		}

		Plane3D nearPlane(center + Vector3D(0.0f, -extents.y, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));
		Plane3D farPlane(center + Vector3D(0.0f, extents.y, 0.0f), Vector3D(0.0f, 1.0f, 0.0f));

		float nearIntersect = nearPlane.intersect(ray);
		float farIntersect = farPlane.intersect(ray);

		float yEnter;
		float yExit;
		if (farIntersect > nearIntersect)
		{
			yEnter = nearIntersect;
			yExit = farIntersect;
		}
		else
		{
			yEnter = farIntersect;
			yExit = nearIntersect;
		}

		float enter = fmaxf(fmaxf(xEnter, yEnter), zEnter);
		float exit = fminf(fminf(xExit, yExit), zExit);

		if (std::isfinite(enter) && std::isfinite(exit) && enter <= exit)
		{
			return IntersectResult(enter, exit);
		}

		// return a miss
		return IntersectResult();
	}

	float Triangle3D::intersect(const Ray3D& ray) const
	{
		float d = toPlane().intersect(ray);
		if (std::isinf(d))
		{
			return std::numeric_limits<float>::infinity();
		}

		Vector3D p = toBarycentric(ray.pointAt(d));
		if (p.x < 0 || p.y < 0 || p.z < 0)
		{
			return std::numeric_limits<float>::infinity();
		}

		return d;
	}

	Plane3D Triangle3D::toPlane() const
	{
		return Plane3D::fromPoints(a, b, c);
	}

	Vector3D Triangle3D::toBarycentric(const Vector3D& p) const
	{
		auto v0 = b - a;
		auto v1 = c - a;
		auto v2 = p - a;

		float v = ((v1.dot(v1) * v2.dot(v0)) - (v1.dot(v0) * v2.dot(v1)))
				/ ((v0.dot(v0) * v1.dot(v1)) - (v0.dot(v1) * v1.dot(v0)));

		float w = ((v0.dot(v0) * v2.dot(v1)) - (v0.dot(v1) * v2.dot(v0)))
				/ ((v0.dot(v0) * v1.dot(v1)) - (v0.dot(v1) * v1.dot(v0)));

		float u = 1.0f - v - w;

		return Vector3D(u, v, w);
	}

	Vector3D Triangle3D::toCartesian(const Vector3D& p) const
	{
		return (p.x * a) + (p.y * b) + (p.z * c);
	}

	const Triangle3D::LineIntersectResult& Triangle3D::LineIntersectResult::closestTo(
		const Vector3D& v,
	    const Triangle3D::LineIntersectResult& b)
	{
		if (!hit)
		{
			return b;
		}
		if (!b.hit)
		{
			return *this;
		}

		if ((point - v).lengthSquared() < (b.point - v).lengthSquared())
		{
			return *this;
		}
		else
		{
			return b;
		}
	}
}
