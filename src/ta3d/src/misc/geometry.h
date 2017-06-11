#ifndef __TA3D_GEOMETRY_H__
#define __TA3D_GEOMETRY_H__

#include <boost/optional.hpp>

#include "vector.h"

namespace TA3D
{
	/** An infinite ray in 3D space. */
	struct Ray3D
	{
		/** The starting point at the ray, at t = 0. */
		Vector3D origin;

		/** The direction the ray is travelling. */
		Vector3D direction;

		Ray3D(Vector3D position, Vector3D direction);

		/** Returns the position along the ray at time t. */
		Vector3D pointAt(const float t) const;

		/**
		 * Returns true if point a is less far along the ray than point b.
		 * a and b are both assumed to lie on the ray.
		 */
		bool isLessFar(const Vector3D& a, const Vector3D& b) const;
	};

	/** An infinite plane in 3D space. */
	struct Plane3D
	{
		/**
		 * Creates a plane from 3 points that lie on it.
		 * The plane is constructed such that the points
		 * are in anticlockwise order when the normal
		 * is pointing at the viewer.
		 */
		static Plane3D fromPoints(Vector3D a, Vector3D b, Vector3D c);

		/** A point on the plane. */
		Vector3D point;

		/** The normal of the plane. */
		Vector3D normal;

		Plane3D(Vector3D point, Vector3D normal): point(point), normal(normal) {}

		/**
		 * Returns the distance along the ray
		 * at which it intersects with this plane.
		 * The distance is defined in terms of the ray's
		 * direction vector.
		 *
		 * If the ray and the plane never intersect,
		 * i.e. they are parallel, this will return
		 * a result indicating that they did not intersect.
		 */
		boost::optional<float> intersect(const Ray3D& ray) const;

		/**
		 * Returns the distance along the ray
		 * at which it intersects with this plane.
		 * The distance is defined in terms of the ray's
		 * direction vector.
		 *
		 * If the ray does not intersect, i.e. it is parallel,
		 * if the ray is in front of the plane we return infinity,
		 * otherwise we return negative infinity.
		 */
		float intersectOrInfinity(const Ray3D& ray) const;

		/**
		 * Returns true if the given point is in front of the plane.
		 * The point is in front of the plane if it is on the side of the plane
		 * that the normal is pointing to.
		 * Points that lie on the plane are not considered to be in front of it.
		 */
		bool isInFront(const Vector3D& p) const;
	};

	/** A plane along the XZ axis with the normal pointing towards positive Y. */
	static const Plane3D PlaneXZ = Plane3D(Vector3D(), Vector3D(0.0f, 1.0f, 0.0f));

	/** A plane along the XY axis with the normal pointing towards positive Z */
	static const Plane3D PlaneXY = Plane3D(Vector3D(), Vector3D(0.0f, 0.0f, 1.0f));

	/** An axis-aligned bounding box in 3D space */
	struct BoundingBox3D
	{
		struct RayIntersect
		{
			float enter;
			float exit;
			RayIntersect(float enter, float exit): enter(enter), exit(exit) {}
		};

		/** The position of the center of the box. */
		Vector3D center;

		/** The box's extents in all directions from the center. */
		Vector3D extents;

		BoundingBox3D(const Vector3D& center, const Vector3D& extents): center(center), extents(extents) {}

		/**
		 * Computes the intersection between the given ray and the bounding box.
		 * If the ray intersects, returns a result containing the distances
		 * at which the ray enters and exits the bounding box.
		 * Otherwise, returns a result indicating that the ray missed.
		 */
		boost::optional<RayIntersect> intersect(const Ray3D& ray) const;
	};

	struct Triangle3D
	{
		Vector3D a;
		Vector3D b;
		Vector3D c;

		Triangle3D() {}
		Triangle3D(const Vector3D& a, const Vector3D& b, const Vector3D& c): a(a), b(b), c(c) {}

		/**
		 * Returns the distance along the ray
		 * at which it intersects with this triangle.
		 * The distance is defined in terms of the ray's
		 * direction vector.
		 *
		 * If the ray and the triangle never intersect,
		 * this will return a result indicating that they did not intersect.
		 */
		boost::optional<float> intersect(const Ray3D& ray) const;

		/**
		 * Returns the point at which the given line intersects this triangle.
		 * If the line and the triangle do not intersect,
		 * returns a result indicating this.
		 *
		 * The intersection test is performed using scalar triple product.
		 * If another triangle shares an edge with this one,
		 * the test guarantees that the line intersects
		 * either with this triangle or the other triangle,
		 * provided that the edge goes in the same direction
		 * in both triangles.
		 * (That is, any of A->B, B->C, C->A)
		 *
		 * @param p The point at which the line starts
		 * @param q The point at which the line ends
		 */
		boost::optional<Vector3D> intersectLine(const Vector3D& p, const Vector3D& q) const;

		/**
		 * Converts the input world-space coordinates
		 * which lie on the triangle into barycentric coordinates.
		 */
		Vector3D toBarycentric(const Vector3D& p) const;

		/**
		 * Converts the input barycentric coordinates
		 * into world-space coordinates.
		 */
		Vector3D toCartesian(const Vector3D& p) const;

		/**
		 * Returns the plane on which this triangle lies.
		 */
		Plane3D toPlane() const;
	};

	/**
	 * Returns the vector that is closest to v.
	 * If either of the vectors a and b are none,
	 * the other vector is returned.
	 */
	const boost::optional<Vector3D>& closestTo(
		const Vector3D& v,
		const boost::optional<Vector3D>& a,
		const boost::optional<Vector3D>& b);
}

#endif //__TA3D_GEOMETRY_H__
