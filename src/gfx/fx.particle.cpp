
#include "fx.particle.h"
#include "particles/particles.h"
#include "../EngineClass.h"
#include "fx.manager.h"


namespace TA3D
{


    FXParticle::FXParticle(const VECTOR &P, const VECTOR &S, const float L )
        :Pos(P), Speed(S), life(L), timer(0.0f)
    {}



    bool FXParticle::move(float dt)
    {
        life -= dt;
        timer += dt;

        Speed.y -= dt * the_map->ota_data.gravity;		// React to gravity
        Pos = Pos + dt * Speed;

        float min_h = the_map->get_unit_h( Pos.x, Pos.z );
        if( Pos.y < min_h ) // Bouncing on the map :)
        {
            Pos.y = 2.0f * min_h - Pos.y;
            float dx = the_map->get_unit_h( Pos.x + 16.0f, Pos.z ) - min_h;
            float dz = the_map->get_unit_h( Pos.x, Pos.z + 16.0f ) - min_h;
            VECTOR Normal( -dx, 16.0f, -dz );
            Normal.unit();

            if( Speed % Normal < 0.0f )
                Speed = Speed - (1.5f * (Speed % Normal)) * Normal;
        }

        while( timer >= 0.1f ) // Emit smoke
        {
            timer -= 0.1f;
            particle_engine.make_dark_smoke( Pos, 0, 1, 0.0f, -1.0f, -1.0f, 0.5f );
        }

        return life <= 0.0f;			// When it shoud die, return true
    }



    void FXParticle::draw()
    {
        if(FXManager::currentParticleModel)
        {
            glPushMatrix();
            glTranslatef(Pos.x, Pos.y, Pos.z);
            glScalef(0.2f, 0.2f, 0.2f);
            FXManager::currentParticleModel->draw(life);
            glPopMatrix();
        }
    }



} // namespace TA3D

