#include "geometry.h"
#include <cmath>

namespace TA3D
{
	Ray3D::Ray3D(const Vector3D& position, const Vector3D& direction) : origin(position), direction(direction) {}

	Vector3D Ray3D::pointAt(const float t) const
	{
		return origin + (t * direction);
	}

	bool Ray3D::isLessFar(const Vector3D& a, const Vector3D& b) const
	{
		return (a - origin).dot(direction) < (b - origin).dot(direction);
	}

	Plane3D::Plane3D(const Vector3D& point, const Vector3D& normal) : point(point), normal(normal) {}

	Plane3D Plane3D::fromPoints(const Vector3D& a, const Vector3D& b, const Vector3D& c)
	{
		auto position = a;

		auto v1 = b - a;
		auto v2 = c - a;

		auto normal = v1.cross(v2);

		return Plane3D(position, normal);
	}

	boost::optional<float> Plane3D::intersect(const Ray3D& ray) const
	{
		float a = (point - ray.origin).dot(normal);
		float b = ray.direction.dot(normal);
		if (b == 0.0f)
		{
			return boost::none;
		}

		return a / b;
	}

	bool Plane3D::isInFront(const Vector3D& p) const
	{
		return (p - point).dot(normal) > 0;
	}

	float Plane3D::intersectOrInfinity(const Ray3D& ray) const
	{
		auto result = intersect(ray);
		if (result)
		{
			return *result;
		}

		return isInFront(ray.origin)
			? std::numeric_limits<float>::infinity()
			: -std::numeric_limits<float>::infinity();
	}

	boost::optional<Vector3D> Plane3D::intersectLine(const Vector3D& startPoint, const Vector3D& endPoint)
	{
		Ray3D ray(startPoint, endPoint - startPoint);
		auto result = intersect(ray);
		if (result && *result >= 0.0f && *result <= 1.0f)
		{
			return ray.pointAt(*result);
		}

		return boost::none;
	}

	BoundingBox3D::BoundingBox3D(const Vector3D& center, const Vector3D& extents) : center(center), extents(extents) {}

	boost::optional<BoundingBox3D::RayIntersect> BoundingBox3D::intersect(const Ray3D& ray) const
	{
		Plane3D bottomPlane(center + Vector3D(0.0f, 0.0f, -extents.z), Vector3D(0.0f, 0.0f, -1.0f));
		Plane3D topPlane(center + Vector3D(0.0f, 0.0f, extents.z), Vector3D(0.0f, 0.0f, -1.0f));

		float bottomIntersect = bottomPlane.intersectOrInfinity(ray);
		float topIntersect = topPlane.intersectOrInfinity(ray);

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

		Plane3D leftPlane(center + Vector3D(-extents.x, 0.0f, 0.0f), Vector3D(-1.0f, 0.0f, 0.0f));
		Plane3D rightPlane(center + Vector3D(extents.x, 0.0f, 0.0f), Vector3D(-1.0f, 0.0f, 0.0f));

		float leftIntersect = leftPlane.intersectOrInfinity(ray);
		float rightIntersect = rightPlane.intersectOrInfinity(ray);

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

		Plane3D nearPlane(center + Vector3D(0.0f, -extents.y, 0.0f), Vector3D(0.0f, -1.0f, 0.0f));
		Plane3D farPlane(center + Vector3D(0.0f, extents.y, 0.0f), Vector3D(0.0f, -1.0f, 0.0f));

		float nearIntersect = nearPlane.intersectOrInfinity(ray);
		float farIntersect = farPlane.intersectOrInfinity(ray);

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
			return RayIntersect(enter, exit);
		}

		// return a miss
		return boost::none;
	}

	Triangle3D::Triangle3D(const Vector3D& a, const Vector3D& b, const Vector3D& c) : a(a), b(b), c(c) {}

	boost::optional<float> Triangle3D::intersect(const Ray3D& ray) const
	{
		auto intersect = toPlane().intersect(ray);
		if (!intersect)
		{
			return boost::none;
		}

		Vector3D p = toBarycentric(ray.pointAt(*intersect));
		if (p.x < 0 || p.y < 0 || p.z < 0)
		{
			return boost::none;
		}

		return *intersect;
	}

	boost::optional<Vector3D> Triangle3D::intersectLine(const Vector3D& p, const Vector3D& q) const
	{
		auto pq = q - p;
		auto pa = a - p;
		auto pb = b - p;
		auto pc = c - p;

		float u = scalarTriple(pq, pc, pb);
		float v = scalarTriple(pq, pa, pc);
		if (!Math::sameSign(u, v))
		{
			return boost::none;
		}

		float w = scalarTriple(pq, pb, pa);
		if (!Math::sameSign(v, w))
		{
			return boost::none;
		}

		float denominator = u + v + w;
		u /= denominator;
		v /= denominator;
		w /= denominator;

		return toCartesian(Vector3D(u, v, w));
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


	const boost::optional<Vector3D>& closestTo(
		const Vector3D& v,
		const boost::optional<Vector3D>& a,
		const boost::optional<Vector3D>& b)
	{
		if (!a)
		{
			return b;
		}

		if (!b)
		{
			return a;
		}

		if ((*a - v).lengthSquared() < (*b - v).lengthSquared())
		{
			return a;
		}

		return b;
	}
}
