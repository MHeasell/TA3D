#include <GL/glew.h>
#include "camera.h"
#include "../TA3D_NameSpace.h"

namespace TA3D
{
	Camera* Camera::inGame = nullptr;

	const Vector3D Camera::_direction(0.0f, -1.0f, 0.0f);
	const Vector3D Camera::_forward(0.0f, 1.0f, 0.0f);
	const Vector3D Camera::_up(0.0f, 0.0f, -1.0f);
	const Vector3D Camera::_side(1.0f, 0.0f, 0.0f);

	Camera::Camera(const float width, const float height)
		:
		_viewportWidth(width),
		_viewportHeight(height),
		_position(0.0f, 0.0f, 0.0f),
		worldOrientation(createOrientationMatrix()),
		inverseWorldOrientation(createInverseOrientationMatrix()),
		projection(createProjectionMatrix(width, height)),
		inverseProjection(createInverseProjectionMatrix(width, height))
	{
	}

	Ray3D Camera::screenToWorldRay(const Vector2D& point) const
	{
		Matrix transform = inverseWorldTranslation() * inverseWorldOrientation * inverseProjection;
		Vector3D startPoint = transform * Vector3D(point.x, point.y, -1.0f);
		Vector3D endPoint = transform * Vector3D(point.x, point.y, 1.0f);
		Vector3D direction = endPoint - startPoint;
		return Ray3D(startPoint, direction);
	}

	std::vector<Vector3D> Camera::getProjectionShadow() const
	{
		std::vector<Vector2D> bounds {
			Vector2D(-1.0f, -1.0f),
			Vector2D(1.0f, -1.0f),
			Vector2D(1.0f, 1.0f),
			Vector2D(-1.0f, 1.0f),
		};

		std::vector<Vector3D> out;

		for (auto it = bounds.begin(); it != bounds.end(); ++it)
		{
			auto ray = screenToWorldRay(*it);
			auto point = ray.pointAt(PlaneXZ.intersect(ray));
			out.push_back(point);
		}

		return out;
	}

	Matrix Camera::getViewMatrix() const
	{
		return worldOrientation * worldTranslation();
	}

	Matrix Camera::getProjectionMatrix() const
	{
		return projection;
	}

	Matrix Camera::getViewProjectionMatrix() const
	{
		return getProjectionMatrix() * getViewMatrix();
	}

	void Camera::getFrustum(std::vector<Vector3D>& list) const
	{
		// transform from clip space back to world space
		Matrix transform = inverseWorldTranslation() * inverseWorldOrientation * inverseProjection;

		// near
		list.push_back(transform * Vector3D(-1.0f, 1.0f, -1.0f)); // top-left
		list.push_back(transform * Vector3D(1.0f, 1.0f, -1.0f)); // top-right
		list.push_back(transform * Vector3D(-1.0f, -1.0f, -1.0f)); // bottom-left
		list.push_back(transform * Vector3D(1.0f, -1.0f, -1.0f)); // bottom-right

		// far
		list.push_back(transform * Vector3D(-1.0f, 1.0f, 1.0f)); // top-left
		list.push_back(transform * Vector3D(1.0f, 1.0f, 1.0f)); // top-right
		list.push_back(transform * Vector3D(-1.0f, -1.0f, 1.0f)); // bottom-left
		list.push_back(transform * Vector3D(1.0f, -1.0f, 1.0f)); // bottom-right
	}

	void Camera::applyToOpenGl() const
	{
		Matrix view = worldOrientation * worldTranslation();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gfx->multMatrix(projection);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gfx->multMatrix(view);
	}

	void Camera::translate(const float x, const float y)
	{
		_position += Vector3D(x, 0.0f, y);
	}

	void Camera::setPosition(const float x, const float y)
	{
		_position = Vector3D(x, _position.y, y);
	}

	Matrix Camera::createProjectionMatrix(float width, float height)
	{
		float halfWidth = width / 2.0f;
		float halfHeight = height / 2.0f;
		Matrix ortho = OrthographicProjection(
			-halfWidth,
			halfWidth,
			-halfHeight,
			halfHeight,
			-1000.0f,
			1000.0f
		);

		Matrix cabinet = CabinetProjection(0.0f, 0.5f);

		return ortho * cabinet;
	}

	Matrix Camera::createInverseProjectionMatrix(float width, float height)
	{
		float halfWidth = width / 2.0f;
		float halfHeight = height / 2.0f;

		Matrix inverseOrtho = InverseOrthographicProjection(
			-halfWidth,
			halfWidth,
			-halfHeight,
			halfHeight,
			-1000.0f,
			1000.0f
		);

		Matrix inverseCabinet = CabinetProjection(0.0f, -0.5f);

		return inverseCabinet * inverseOrtho;
	}

	Matrix Camera::createOrientationMatrix()
	{
		return RotateToAxes(_side, _up, _forward);
	}

	Matrix Camera::createInverseOrientationMatrix()
	{
		return Transpose(RotateToAxes(_side, _up, _forward));
	}

	float Camera::viewportWidth() const
	{
		return _viewportWidth;
	}

	float Camera::viewportHeight() const
	{
		return _viewportHeight;
	}

	const Vector3D& Camera::position() const
	{
		return _position;
	}

	Vector3D& Camera::position()
	{
		return _position;
	}

	const Vector3D& Camera::direction() const
	{
		return _direction;
	}

	const Vector3D& Camera::up() const
	{
		return _up;
	}

	const Vector3D& Camera::side() const
	{
		return _side;
	}

	Matrix Camera::worldTranslation() const
	{
		return Translate(-1 * _position);
	}

	Matrix Camera::inverseWorldTranslation() const
	{
		return Translate(_position);
	}

	bool Camera::viewportContains(const Vector3D& point) const
	{
		// convert to clip space
		auto projectedPoint = getViewProjectionMatrix() * point;

		// test if it's inside the clip volume
		return
			projectedPoint.x >= -1.0f
			&& projectedPoint.x <= 1.0f
			&& projectedPoint.y >= -1.0f
			&& projectedPoint.y <= 1.0f
			&& projectedPoint.z >= -1.0f
			&& projectedPoint.z <= 1.0f;
	}
}
