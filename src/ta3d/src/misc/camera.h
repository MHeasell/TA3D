#ifndef __TA3D_MISC_CAMERA_H__
#define __TA3D_MISC_CAMERA_H__


#include <vector>
#include "vector.h"
#include "geometry.h"
#include "matrix.h"

namespace TA3D
{
	class Camera
	{
	public:
		static Camera* inGame;

	private:
		static const Vector3D _direction;
		static const Vector3D _side;
		static const Vector3D _up;
		static const Vector3D _forward;

		const float _viewportWidth;
		const float _viewportHeight;

		Vector3D _position;

		Matrix worldOrientation;
		Matrix inverseWorldOrientation;
		Matrix projection;
		Matrix inverseProjection;

	public:
		/**
		 * Constructs a new camera instance.
		 * @param width The width of the camera viewport
		 * @param height The height of the camera viewport
		 */
		Camera(const float width, const float height);

		float viewportWidth() const;

		float viewportHeight() const;

		const Vector3D& position() const;

		const Vector3D& direction() const;

		const Vector3D& up() const;

		const Vector3D& side() const;

		Vector3D& position();

		/**
		 * Returns a list of vertex positions representing the shadow
		 * that the camera's viewport casts on the XZ plane
		 * in world space.
		 */
		std::vector<Vector3D> getProjectionShadow() const;

		/**
		 * Transforms a point on the screen (in normalized device coordinates)
		 * to a ray in world space shooting into the world from that point.
		 */
		Ray3D screenToWorldRay(const Vector2D& point) const;

		Matrix getViewMatrix() const;

		Matrix getProjectionMatrix() const;

		Matrix getViewProjectionMatrix() const;

		bool viewportContains(const Vector3D& point) const;

		/**
		 * Returns the 8 points defining the frustum volume.
		 */
		void getFrustum(std::vector<Vector3D>& list) const;

		void applyToOpenGl() const;

		void translate(const float x, const float y);

		void setPosition(const float x, const float y);

	private:
		static Matrix createProjectionMatrix(float width, float height);
		static Matrix createInverseProjectionMatrix(float width, float height);
		static Matrix createOrientationMatrix();
		static Matrix createInverseOrientationMatrix();

		Matrix worldTranslation() const;
		Matrix inverseWorldTranslation() const;
	};
}


#endif // __TA3D_MISC_CAMERA_H__
