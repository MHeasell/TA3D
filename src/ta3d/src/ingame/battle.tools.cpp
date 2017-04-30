
#include "battle.h"
#include <UnitEngine.h>
#include <languages/table.h>
#include "players.h"
#include <input/keyboard.h>
#include <input/mouse.h>
#include <sounds/manager.h>
#include <console/console.h>

namespace TA3D
{

	void Battle::setTimeFactor(const float f)
	{
		lp_CONFIG->timefactor = f;
		show_timefactor = 1.0f;
	}

	Vector3D Battle::cursorOnMap(const Camera& cam, MAP& map, bool on_mini_map)
	{
		if (on_mini_map) // If the cursor is on the mini_map;
		{
			const float x = float(mouse_x - 64) * 252.0f / 128.0f * (float)map.map_w / (float)map.mini_w;
			const float z = float(mouse_y - 64) * 252.0f / 128.0f * (float)map.map_h / (float)map.mini_h;
			const float y = map.get_unit_h(x, z);
			return Vector3D(x, y, z);
		}

		const Vector3D cur_pos = cam.pos + cam.zoomFactor * (float(mouse_x - gfx->SCREEN_W_HALF) * cam.side - float(mouse_y - gfx->SCREEN_H_HALF) * cam.up);
		return map.hit(cur_pos, cam.dir, true, 2000000000.0f, true);
	}

	void Battle::updateZFAR()
	{
		cam.zfar = 600.0f + Math::Max((cam_h - 150.0f) * 2.0f, 0.0f);
		// Set View
		cam.setView();
	}

	void Battle::showGameStatus()
	{
		Gui::WND::Ptr statuswnd = pArea.get_wnd("gamestatus");
		if (statuswnd)
			statuswnd->y = (int)((float) gfx->height - float(statuswnd->height + 32) * show_gamestatus);

		uint32 game_time = units.current_tick / TICKS_PER_SEC;

		String tmp;

		if (pArea.get_state("gamestatus")) // Don't update things if we don't display them
		{
			// Time
			tmp << TranslationTable::gameTime << " : " << (game_time / 3600) << ':';
			int minutes = ((game_time / 60) % 60);
			tmp << (minutes < 10 ? String('0') : String()) << minutes << ':';
			int seconds = (game_time % 60);
			tmp << (seconds < 10 ? String('0') : String()) << seconds;
			pArea.caption("gamestatus.time_label", tmp);

			// Units
			tmp.clear();
			tmp << TranslationTable::units << " : " << players.nb_unit[players.local_human_id] << '/' << MAX_UNIT_PER_PLAYER;
			pArea.caption("gamestatus.units_label", tmp);

			// Speed
			tmp.clear();
			tmp << TranslationTable::speed << " : " << (int)round(lp_CONFIG->timefactor) << '('
				<< (int)round(units.apparent_timefactor) << ')';
			pArea.caption("gamestatus.speed_label", tmp);
		}

		statuswnd = pArea.get_wnd("playerstats");
		if (statuswnd)
			statuswnd->x = (int)((float) gfx->width - float(statuswnd->width + 10) * show_gamestatus);

		if (pArea.get_state("playerstats")) // Don't update things if we don't display them
			for (unsigned int i = 0; i < players.count(); ++i)
			{
				tmp.clear();
				tmp << "playerstats.p" << i << "_box";
				Gui::GUIOBJ::Ptr obj = pArea.get_object(tmp);
				if (obj)
				{
					obj->Data = gfx->makeintcol(
						player_color[3 * player_color_map[i]],
						player_color[3 * player_color_map[i] + 1],
						player_color[3 * player_color_map[i] + 2], 0.5f);
				}
				// Kills
				tmp.clear();
				tmp += players.kills[i];
				pArea.caption(String("playerstats.p") << i << "_kills", tmp);
				// Losses
				tmp.clear();
				tmp += players.losses[i];
				pArea.caption(String("playerstats.p") << i << "_losses", tmp);
			}
	}

	void Battle::nudgeCameraLeft()
	{
		cam.rpos.x -= SCROLL_SPEED * deltaTime * cam_h / 151.0f;
	}

	void Battle::nudgeCameraRight()
	{
		cam.rpos.x += SCROLL_SPEED * deltaTime * cam_h / 151.0f;
	}

	void Battle::nudgeCameraDown()
	{
		cam.rpos.z += SCROLL_SPEED * deltaTime * cam_h / 151.0f;
	}

	void Battle::nudgeCameraUp()
	{
		cam.rpos.z -= SCROLL_SPEED * deltaTime * cam_h / 151.0f;
	}

	void Battle::putCameraAt(const Vector2D& position)
	{
		putCameraAt(position.x, position.y);
	}

	void Battle::putCameraAt(float x, float z)
	{
		cam.rpos.x = x;
		cam.rpos.z = z;
	}

	Vector2D Battle::screenToMinimapCoordinates(const Vector2D& coordinates) const
	{
		float minimapWidthFactor = 252.0f / map->mini_w;
		float minimapHeightFactor = 252.0f / map->mini_h;
		return Vector2D(
			(coordinates.x / 128.0f) * minimapWidthFactor,
			(coordinates.y / 128.0f) * minimapHeightFactor);
	}

	Vector2D Battle::minimapToWorldCoordinates(const Vector2D& coordinates) const
	{
		return Vector2D(
			(coordinates.x - 0.5f) * map->map_w,
			(coordinates.y - 0.5f) * map->map_h
		);
	}

	void Battle::handleGameStatusEvents()
	{
		// Enable the game status if the `space` is pressed
		if (!pCacheShowGameStatus && isKeyDown(KEY_SPACE))
		{
			pCacheShowGameStatus = true;
			pArea.msg("gamestatus.show");  // Show it
			pArea.msg("playerstats.show"); // Show it
		}

		if (pCacheShowGameStatus)
		{
			if (isKeyDown(KEY_SPACE)) // Show gamestatus window
			{
				if (show_gamestatus < 1.f)
				{
					show_gamestatus += 10.f * deltaTime;
					if (show_gamestatus > 1.f)
						show_gamestatus = 1.f;
				}
			}
			else
			{ // Hide gamestatus window
				show_gamestatus -= 10.f * deltaTime;

				if (show_gamestatus < 0.f)
				{
					show_gamestatus = 0.f;
					pCacheShowGameStatus = false;
					pArea.msg("gamestatus.hide");  // Hide it
					pArea.msg("playerstats.hide"); // Hide it
					return;
				}
			}

			showGameStatus();
		}
	}

} // namespace TA3D
