#ifndef __TA3D_GEOMETRY_H__
#define __TA3D_GEOMETRY_H__

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
		 * i.e. they are parallel, this will return either
		 * positive or negative infinity.
		 */
		float intersect(const Ray3D& ray) const;

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
		struct IntersectResult
		{
			/**
			 * True if the result is a hit, otherwise false.
			 * If this is false then enter and exit are not set.
			 */
			bool hit;

			/**
			 * The distance along the ray at which it entered the bounding box.
			 */
			float enter;

			/**
			 * The distance along the ray at which it exited the bounding box.
			 */
			float exit;

			/** Constructs a result representing a miss. */
			IntersectResult(): hit(false) {}

			/** Constructs a result representing a hit with the given entry and exit distances */
			IntersectResult(float enter, float exit): hit(true), enter(enter), exit(exit) {}
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
		IntersectResult intersect(const Ray3D& ray) const;
	};
}

#endif //__TA3D_GEOMETRY_H__
