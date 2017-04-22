#ifndef __TA3D_MISC_MATRIX_H__
#define __TA3D_MISC_MATRIX_H__

#include "vector.h"
#include <string.h>

namespace TA3D
{

	class Matrix
	{
	public:
		//! Default constructor
		Matrix() { clear(); }
		Matrix(const Matrix& rhs)
		{
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
					E[i][j] = rhs.E[i][j];
			}
		}

		/*!
		** \brief Clear the matrix
		*/
		inline void clear() { memset(E, 0, 64); }

		Matrix& operator=(const Matrix& rhs)
		{
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
					E[i][j] = rhs.E[i][j];
			}

			return *this;
		}

	public:
		/**
		 * Elements are stored in column-major order,
		 * i.e. the array is indexed E[column][row] or E[x][y].
		 */
		float E[4][4]; // Matrice 4x4
	};

	TA3D::Vector3D glNMult(const TA3D::Vector3D& A, const TA3D::Matrix& B);

	Matrix IdentityMatrix();

	// Crée une matrice de translation
	TA3D::Matrix Translate(const TA3D::Vector3D& A);

	// Crée une matrice de mise à l'échelle
	TA3D::Matrix Scale(const float Size);

	// Crée une matrice de rotation autour de l'axe X
	TA3D::Matrix RotateX(const float Theta);

	// Crée une matrice de rotation autour de l'axe Y
	TA3D::Matrix RotateY(const float Theta);

	// Crée une matrice de rotation autour de l'axe Z
	TA3D::Matrix RotateZ(const float Theta);

	// Returns RotateZ(Rz) * RotateY(Ry) * RotateX(Rx) but faster ;)
	TA3D::Matrix RotateZYX(const float Rz, const float Ry, const float Rx);

	// Returns RotateX(Rx) * RotateY(Ry) * RotateZ(Rz) but faster ;)
	TA3D::Matrix RotateXYZ(const float Rx, const float Ry, const float Rz);

	// Returns RotateX(Rx) * RotateZ(Rz) * RotateY(Ry) but faster ;)
	TA3D::Matrix RotateXZY(const float Rx, const float Rz, const float Ry);

	// Returns RotateY(Ry) * RotateZ(Rz) * RotateX(Rx) but faster ;)
	TA3D::Matrix RotateYZX(const float Ry, const float Rz, const float Rx);

	/**
	 * Returns a cabinet projection matrix
	 * where Z-axis lines will have the given proportion of length
	 * in the X and Y axes of the projected image.
	 * @param x The Z-axis contribution to X position
	 * @param y the Z-axis contribution to the Y position
	 * @return The created projection matrix
	 */
	Matrix CabinetProjection(float x, float y);

	/**
	 * Returns an orthographic projection matrix.
	 *
	 * Throws if left = right, or top = bottom,or near = far
	 *
	 * @param left The position of the left vertical clipping plane
	 * @param right The position of the right vertical clipping plane
	 * @param bottom The position of the bottom horizontal clipping plane
	 * @param top The position of the top horizontal clipping plane
	 * @param nearVal The position of the near depth clipping plane
	 * @param farVal The position of the right depth clipping plane
	 * @return The created projection matrix
	 */
	Matrix OrthographicProjection(float left, float right, float bottom, float top, float nearVal, float farVal);

	Matrix InverseOrthographicProjection(float left, float right, float bottom, float top, float nearVal, float farVal);

	// Transpose une matrice
	TA3D::Matrix Transpose(const TA3D::Matrix& A);

	/**
	 * Creates a rotation matrix from axis vectors.
	 */
	Matrix RotateToAxes(const Vector3D& side, const Vector3D& up, const Vector3D& forward);

} // namespace TA3D

namespace TA3D
{

	// Addition
	inline TA3D::Matrix operator+(TA3D::Matrix A, const TA3D::Matrix& B)
	{
		for (int i = 0; i < 16; i++)
			A.E[i >> 2][i & 3] += B.E[i >> 2][i & 3];
		return A;
	}

	// Soustraction
	inline TA3D::Matrix operator-(TA3D::Matrix A, const TA3D::Matrix& B)
	{
		for (int i = 0; i < 16; ++i)
			A.E[i >> 2][i & 3] -= B.E[i >> 2][i & 3];
		return A;
	}

	// Multiplication
	inline TA3D::Matrix operator*(const TA3D::Matrix& A, const TA3D::Matrix& B)
	{
		TA3D::Matrix C;
		C.E[0][0] = A.E[0][0] * B.E[0][0] + A.E[1][0] * B.E[0][1] + A.E[2][0] * B.E[0][2] + A.E[3][0] * B.E[0][3];
		C.E[0][1] = A.E[0][1] * B.E[0][0] + A.E[1][1] * B.E[0][1] + A.E[2][1] * B.E[0][2] + A.E[3][1] * B.E[0][3];
		C.E[0][2] = A.E[0][2] * B.E[0][0] + A.E[1][2] * B.E[0][1] + A.E[2][2] * B.E[0][2] + A.E[3][2] * B.E[0][3];
		C.E[0][3] = A.E[0][3] * B.E[0][0] + A.E[1][3] * B.E[0][1] + A.E[2][3] * B.E[0][2] + A.E[3][3] * B.E[0][3];

		C.E[1][0] = A.E[0][0] * B.E[1][0] + A.E[1][0] * B.E[1][1] + A.E[2][0] * B.E[1][2] + A.E[3][0] * B.E[1][3];
		C.E[1][1] = A.E[0][1] * B.E[1][0] + A.E[1][1] * B.E[1][1] + A.E[2][1] * B.E[1][2] + A.E[3][1] * B.E[1][3];
		C.E[1][2] = A.E[0][2] * B.E[1][0] + A.E[1][2] * B.E[1][1] + A.E[2][2] * B.E[1][2] + A.E[3][2] * B.E[1][3];
		C.E[1][3] = A.E[0][3] * B.E[1][0] + A.E[1][3] * B.E[1][1] + A.E[2][3] * B.E[1][2] + A.E[3][3] * B.E[1][3];

		C.E[2][0] = A.E[0][0] * B.E[2][0] + A.E[1][0] * B.E[2][1] + A.E[2][0] * B.E[2][2] + A.E[3][0] * B.E[2][3];
		C.E[2][1] = A.E[0][1] * B.E[2][0] + A.E[1][1] * B.E[2][1] + A.E[2][1] * B.E[2][2] + A.E[3][1] * B.E[2][3];
		C.E[2][2] = A.E[0][2] * B.E[2][0] + A.E[1][2] * B.E[2][1] + A.E[2][2] * B.E[2][2] + A.E[3][2] * B.E[2][3];
		C.E[2][3] = A.E[0][3] * B.E[2][0] + A.E[1][3] * B.E[2][1] + A.E[2][3] * B.E[2][2] + A.E[3][3] * B.E[2][3];

		C.E[3][0] = A.E[0][0] * B.E[3][0] + A.E[1][0] * B.E[3][1] + A.E[2][0] * B.E[3][2] + A.E[3][0] * B.E[3][3];
		C.E[3][1] = A.E[0][1] * B.E[3][0] + A.E[1][1] * B.E[3][1] + A.E[2][1] * B.E[3][2] + A.E[3][1] * B.E[3][3];
		C.E[3][2] = A.E[0][2] * B.E[3][0] + A.E[1][2] * B.E[3][1] + A.E[2][2] * B.E[3][2] + A.E[3][2] * B.E[3][3];
		C.E[3][3] = A.E[0][3] * B.E[3][0] + A.E[1][3] * B.E[3][1] + A.E[2][3] * B.E[3][2] + A.E[3][3] * B.E[3][3];

		return C;
	}

	// Multiplication(transformation d'un vecteur)
	inline TA3D::Vector3D operator*(const TA3D::Vector3D& A, const TA3D::Matrix& B)
	{
		TA3D::Vector3D C;
		C.x = A.x * B.E[0][0] + A.y * B.E[0][1] + A.z * B.E[0][2];
		C.y = A.x * B.E[1][0] + A.y * B.E[1][1] + A.z * B.E[1][2];
		C.z = A.x * B.E[2][0] + A.y * B.E[2][1] + A.z * B.E[2][2];
		return C;
	}

	/** Multiplication of a matrix M by column vector v */
	inline TA3D::Vector3D operator*(const TA3D::Matrix& M, const TA3D::Vector3D& v)
	{
		TA3D::Vector3D out;
		out.x = (M.E[0][0] * v.x) + (M.E[1][0] * v.y) + (M.E[2][0] * v.z) + (M.E[3][0]);
		out.y = (M.E[0][1] * v.x) + (M.E[1][1] * v.y) + (M.E[2][1] * v.z) + (M.E[3][1]);
		out.z = (M.E[0][2] * v.x) + (M.E[1][2] * v.y) + (M.E[2][2] * v.z) + (M.E[3][2]);
		return out;
	}

	// Multiplication d'une matrice par un réel
	inline TA3D::Matrix operator*(const float& A, TA3D::Matrix B)
	{
		for (int i = 0; i < 16; ++i)
			B.E[i >> 2][i & 3] *= A;
		return B;
	}
}

#endif // __TA3D_MISC_MATRIX_H__
