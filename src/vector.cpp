
#include "vector.h"



void VECTOR2D::Unit()
{
    if (x != 0.0f || y != 0.0f) // Si le vecteur n'est pas nul
    {
        float n = 1.0f / sqrt(x*x + y*y);    // Inverse de la norme du vecteur
        x *= n;
        y *= n;
    }
}


void VECTOR3D::Unit()
{
    if(x != 0.0f || y != 0.0f || z != 0.0f) // Si le vecteur n'est pas nul
    {
        float n=Norm();				// Inverse de la norme du vecteur
        if(n!=0.0f)
        {
            n = 1.0f / n;
            x *= n;
            y *= n;
            z *= n;
        }
    }
}


