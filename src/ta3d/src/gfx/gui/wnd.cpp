/*  TA3D, a remake of Total Annihilation
	Copyright (C) 2005  Roland BROCHARD

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/
#include "wnd.h"
#include <languages/i18n.h>
#include <ta3dbase.h>
#include <misc/ta3d_math.h>
#include <sounds/manager.h>
#include <console/console.h>
#include <gfx/glfunc.h>
#include <misc/tdf.h>
#include <misc/paths.h>
#include <input/keyboard.h>
#include <input/mouse.h>
#include <cassert>

#define FIX_COLOR(col) col = makeacol(getb(col), getg(col), getr(col), getr(col))

namespace TA3D
{
	namespace Gui
	{

		WND::WND()
			: x(gfx->width >> 2), y(gfx->height >> 2), width(gfx->width >> 1), height(gfx->height >> 1), title_h(0), Title(), Name(), obj_hashtable(), background(0), repeat_bkg(false), bkg_w(1), bkg_h(1), Lock(false), show_title(true), draw_borders(true), hidden(false), was_hidden(false), tab_was_pressed(false), background_wnd(false), get_focus(false), delete_gltex(false), size_factor(1.), ingame_window(false)
		{
			color = makeacol(0x7F, 0x7F, 0x7F, 0xFF); // Default : grey
			pCacheFontHeight = gui_font->height();
			scrolling = 0;
			scrollable = false;
			background_clamp = false;
			background_width = 1;
			background_height = 1;
		}

		WND::WND(const String& filename)
			: x(gfx->width >> 2), y(gfx->height >> 2), width(gfx->width >> 1), height(gfx->height >> 1), title_h(0), Title(), Name(), obj_hashtable(), background(0), repeat_bkg(false), bkg_w(1), bkg_h(1), Lock(false), show_title(true), draw_borders(true), hidden(false), was_hidden(false), tab_was_pressed(false), background_wnd(false), get_focus(false), delete_gltex(false), size_factor(1.), ingame_window(false)

		{
			scrolling = 0;
			scrollable = false;
			color = makeacol(0x7F, 0x7F, 0x7F, 0xFF); // Default : grey
			pCacheFontHeight = gui_font->height();
			load_tdf(filename);
		}

		WND::~WND()
		{
			obj_hashtable.clear();
			destroy();
		}

		void WND::draw(String& helpMsg, const bool focus, const bool deg, Skin* skin)
		{
			/* Asserts */
			assert(NULL != this);
			assert(NULL != skin);

			MutexLocker locker(pMutex);
			if (!hidden) // If it's hidden don't draw it
			{
				// Shadow
				doDrawWindowShadow(skin);

				// Take scrolling into account
				gfx->set_2D_clip_rectangle(x, y, width, height);
				glPushMatrix();
				glTranslatef(0.0f, float(-scrolling), 0.0f);

				// Background
				doDrawWindowBackground(skin);

				// We have to disable clipping now because of windows borders
				// and we must render things in this specific order, otherwise title
				// bars are hidden by background
				gfx->set_2D_clip_rectangle();
				glPopMatrix();

				// Skin
				doDrawWindowSkin(skin, focus, deg);

				gfx->set_2D_clip_rectangle(x, y, width, height);
				glPushMatrix();
				glTranslatef(0.0f, float(-scrolling), 0.0f);

				// Gui Font
				gui_font = (ingame_window) ? gfx->TA_font : gfx->ta3d_gui_font;

				if (!pObjects.empty())
				{
					// Background objects
					for (unsigned int i = 0; i != pObjects.size(); ++i)
					{
						// Affiche les objets d'arrière plan
						if (!(pObjects[i]->Flag & FLAG_HIDDEN))
							doDrawWindowBackgroundObject(helpMsg, i, focus, skin);
					}
					gfx->set_2D_clip_rectangle();
					for (unsigned int i = 0; i < pObjects.size(); ++i)
					{
						// Affiche les objets de premier plan
						if (!(pObjects[i]->Flag & FLAG_HIDDEN))
							doDrawWindowForegroundObject(skin, i);
					}
				}
				else
					gfx->set_2D_clip_rectangle();
				glPopMatrix();
				gui_font = gfx->ta3d_gui_font;
			}
		}

		void WND::doDrawWindowShadow(Skin* skin)
		{
			if (!skin || !draw_borders || Lock)
				return;

			skin->ObjectShadow((float)x - skin->wnd_border.x1, (float)y - skin->wnd_border.y1,
				(float)(x + width) - skin->wnd_border.x2, (float)(y + height) - skin->wnd_border.y2,
				3, 3,
				0.5f, 10.0f);
		}

		void WND::doDrawWindowBackground(Skin* skin)
		{
			if (!geta(color)) // We don't need to render things in full transparency
				return;		  // we can just not render them
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if (!background)
			{
				if (skin && skin->wnd_background)
				{
					gfx->set_color(color);
					gfx->drawtexture(skin->wnd_background, (float)x, (float)y, (float)(x + width), (float)(y + height));
					glBindTexture(GL_TEXTURE_2D, 0);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, 0);
					gfx->rectfill((float)x, (float)y, (float)(x + width), (float)(y + height), color);
				}
			}
			else
			{
				gfx->set_color(color);
				if (background_clamp)
					gfx->drawtexture(background, (float)x, (float)y, (float)(x + background_width), (float)(y + background_height));
				else
				{
					if (repeat_bkg)
						gfx->drawtexture(background, (float)x, (float)y, (float)(x + width), (float)(y + height), 0.0f, 0.0f, ((float)width) / (float)bkg_w, ((float)height) / (float)bkg_h);
					else
						gfx->drawtexture(background, (float)x, (float)y, (float)(x + width), (float)(y + height));
				}
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			glDisable(GL_BLEND);
		}

		void WND::doDrawWindowSkin(Skin* skin, const bool focus, const bool deg)
		{
			if (skin)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Alpha blending activated
				gfx->set_color(color);
				if (draw_borders && skin->wnd_border.tex)
				{
					skin->wnd_border.draw((float)x - skin->wnd_border.x1,
						(float)y - skin->wnd_border.y1,
						(float)(x + width) - skin->wnd_border.x2,
						(float)(y + height) - skin->wnd_border.y2, false);
				}
				if (show_title && skin->wnd_title_bar.tex)
				{
					title_h = (int)(Math::Max(2.0f + pCacheFontHeight, (float)skin->wnd_title_bar.y1) - skin->wnd_title_bar.y2);
					skin->wnd_title_bar.draw((float)x + 3.0f, (float)y + 3.0f, (float)(x + width - 4), (float)(y + 3 + title_h));
					float maxTitleLength = (float)(width - 10) - skin->wnd_title_bar.x1 + skin->wnd_title_bar.x2;
					String cutTitle = Title;
					if (gui_font->length(cutTitle) > maxTitleLength)
					{
						String::size_type smax = Title.utf8size();
						String::size_type smin = 0;
						String::size_type s;
						do
						{
							s = (smin + smax) >> 1;
							cutTitle = SubstrUTF8(Title, 0, s) << "...";
							bool test = gui_font->length(cutTitle) > maxTitleLength;
							if (test)
							{
								if (smax == smin + 1)
								{
									cutTitle = SubstrUTF8(Title, 0, smin) << "...";
									break;
								}
								smax = s;
							}
							else
							{
								if (s == smin)
									break;
								smin = s;
							}
						} while (s > 0 && smin != smax);
					}
					gfx->print(gui_font,
						(float)(x + 5) + skin->wnd_title_bar.x1,
						(float)(y + 3) + ((float)title_h - pCacheFontHeight) * 0.5f,
						0, White, cutTitle);
				}
				glDisable(GL_BLEND);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else
			{
				if (draw_borders)
				{
					const float x0 = (float)x;
					const float x1 = (float)(x + width);
					const float y0 = (float)y;
					const float y1 = (float)(y + height);
					gfx->rect(x0 - 2, y0 - 2, x1 + 1, y1 + 1, Black);
					gfx->rect(x0 - 1, y0 - 1, x1, y1, DGray);
					gfx->line(x0 - 2, y0 - 2, x1 + 1, y0 - 2, White);
					gfx->line(x0 - 2, y0 - 2, x0 - 2, y1 + 1, White);
					gfx->line(x0 - 1, y0 - 1, x1, y0 - 1, LGray);
					gfx->line(x0 - 1, y0 - 1, x0 - 1, y1, LGray);
				}
				if (show_title)
				{
					title_h = (int)(2 + pCacheFontHeight);
					if (deg)
					{
						if (focus)
						{
							glBegin(GL_QUADS);
							glColor3f(0.0f, 0.0f, 1.0f);
							glVertex2f((float)(x + 3), (float)(y + 3));
							glColor3f(0.5f, 0.5f, 0.75f);
							glVertex2f((float)(x + width - 4), (float)(y + 3));
							glColor3f(0.5f, 0.5f, 0.75f);
							glVertex2f((float)(x + width - 4), (float)(y + 5) + pCacheFontHeight);
							glColor3f(0.0f, 0.0f, 1.0f);
							glVertex2f((float)(x + 3), (float)(y + 5) + pCacheFontHeight);
							glEnd();
						}
						else
						{
							glBegin(GL_QUADS);
							glColor3f(0.75f, 0.75f, 0.75f);
							glVertex2f((float)(x + 3), (float)(y + 3));
							glColor3f(0.5f, 0.5f, 0.5f);
							glVertex2f((float)(x + width - 4), (float)(y + 3));
							glColor3f(0.5f, 0.5f, 0.5f);
							glVertex2f((float)(x + width - 4), (float)(y + 5) + pCacheFontHeight);
							glColor3f(0.75f, 0.75f, 0.75f);
							glVertex2f((float)(x + 3), (float)(y + 5) + pCacheFontHeight);
							glEnd();
						}
					}
					else
					{
						if (focus)
							gfx->rectfill((float)(x + 3), (float)(y + 3), (float)(x + width - 4), (float)(y + 5) + pCacheFontHeight, Blue);
						else
							gfx->rectfill((float)(x + 3), (float)(y + 3), (float)(x + width - 4), (float)(y + 5) + pCacheFontHeight, DGray);
					}
					gfx->print(gui_font, (float)(x + 4), (float)(y + 4), 0, White, Title);
				}
			}
		}

		void WND::doDrawWindowBackgroundObject(String& helpMsg, const int i, const bool focus, Skin* skin)
		{
			// Keeping a reference to the object
			GUIOBJ::Ptr objectPtr = pObjects[i];
			// For performance reason, we will directly working on the pointer
			// It is safe due to the reference is guaranted to be valid until the end of the scope
			GUIOBJ* object = objectPtr.get();

			if (object->MouseOn && !object->help_msg.empty())
				helpMsg = object->help_msg;

			bool disabled = object->Flag & FLAG_DISABLED;

			const float x = (float)this->x;
			const float y = (float)this->y;
			switch (object->Type)
			{

				case OBJ_IMG:
					if (object->Data) // Draws the texture associated with the image
					{
						gfx->set_alpha_blending();
						glEnable(GL_TEXTURE_2D);
						glBindTexture(GL_TEXTURE_2D, (GLuint)object->Data);
						gfx->set_color(0xFFFFFFFF);
						glBegin(GL_QUADS);
						glTexCoord2f(object->u1, object->v1);
						glVertex2f(x + object->x1, y + object->y1);
						glTexCoord2f(object->u2, object->v1);
						glVertex2f(x + object->x2, y + object->y1);
						glTexCoord2f(object->u2, object->v2);
						glVertex2f(x + object->x2, y + object->y2);
						glTexCoord2f(object->u1, object->v2);
						glVertex2f(x + object->x1, y + object->y2);
						glEnd();
						gfx->unset_alpha_blending();
						glBindTexture(GL_TEXTURE_2D, 0);
					}
					else // No texture present, draw a black frame
					{
						gfx->rect(x + object->x1, y + object->y1, x + object->x2, y + object->y2, makeacol(0x7F, 0x7F, 0x7F, 0xFF));
						gfx->line(x + object->x1, y + object->y1, x + object->x2, y + object->y2, makeacol(0x7F, 0x7F, 0x7F, 0xFF));
						gfx->line(x + object->x2, y + object->y1, x + object->x1, y + object->y2, makeacol(0x7F, 0x7F, 0x7F, 0xFF));
					}
					break;

				case OBJ_TEXT:
					if (object->Text.empty())
						object->Text.push_back(nullptr);
					if (!(object->Flag & FLAG_TEXT_ADJUST))
					{
						skin->text_color.print(gui_font, x + object->x1, object->y1 + y, object->Data, object->Text.front());
					}
					else
					{
						object->Data = skin->draw_text_adjust(x + object->x1, y + object->y1, x + object->x2, y + object->y2,
							object->Text[0], object->Pos, object->Flag & FLAG_MISSION_MODE);
						if (object->Data > 0)
							object->Pos %= object->Data;
					}
					break;

				// Button
				case OBJ_TA_BUTTON:
				{
					const int cur_img = (object->Flag & FLAG_DISABLED)
						? (int)object->gltex_states.size() - 1
						: ((object->activated && object->nb_stages == 1)
								  ? (int)object->gltex_states.size() - 2
								  : (int)object->current_state);
					if (cur_img < (int)object->gltex_states.size() && cur_img >= 0)
					{
						gfx->set_color(0xFFFFFFFF);
						gfx->set_alpha_blending();
						object->gltex_states[cur_img].draw(x + object->x1, y + object->y1);
						gfx->unset_alpha_blending();
					}
					disabled = false;
					break;
				}

				case OBJ_BUTTON: // Button
					if (object->Text.empty())
						object->Text.push_back(nullptr);
					skin->button(x + object->x1, y + object->y1, x + object->x2, y + object->y2, object->Text.front(), object->activated, !disabled);
					if (object->Focus && focus)
						gfx->rectdot(object->x1 + x - 2, object->y1 + y - 2, object->x2 + x + 2, object->y2 + y + 2, DGray);
					disabled = false;
					break;

				case OBJ_HSLIDER:
				case OBJ_VSLIDER:
					skin->ScrollBar(x + object->x1, y + object->y1, x + object->x2, y + object->y2,
						((float)(object->Value - object->Data)) / float(object->Pos - object->Data),
						object->Type == OBJ_VSLIDER);
					break;

				case OBJ_LIST:
					skin->ListBox(x + object->x1, y + object->y1, x + object->x2, y + object->y2,
						object->Text, object->Pos, object->Data, object->Flag);
					break;

				case OBJ_LINE:
					gfx->disable_texturing();
					gfx->line(x + object->x1, y + object->y1, x + object->x2, y + object->y2, object->Data);
					gfx->enable_texturing();
					break;

				case OBJ_BOX:
					gfx->set_alpha_blending();
					gfx->disable_texturing();
					if (object->Flag & FLAG_FILL)
						gfx->rectfill(x + object->x1, y + object->y1,
							x + object->x2, y + object->y2, object->Data);
					else
						gfx->rect(x + object->x1, y + object->y1, x + object->x2, y + object->y2, object->Data);
					gfx->enable_texturing();
					gfx->unset_alpha_blending();
					break;

				case OBJ_OPTIONC: // Checkbox
					if (object->Text.empty())
						object->Text.push_back(nullptr);
					skin->OptionCase(x + object->x1, y + object->y1, object->Text[0], object->Etat);
					if (object->Focus && focus)
						gfx->rectdot(object->x1 + x - 2, object->y1 + y - 2, object->x2 + x + 2, object->y2 + y + 2, DGray);
					break;

				case OBJ_OPTIONB: // Boutton d'option
					if (object->Text.empty())
						object->Text.push_back(nullptr);
					skin->OptionButton(x + object->x1, y + object->y1, object->Text[0], object->Etat);
					if (object->Focus && focus)
						gfx->rectdot(object->x1 + x - 2, object->y1 + y - 2, object->x2 + x + 2, object->y2 + y + 2, DGray);
					break;

				case OBJ_PBAR: // Progress Bar
					skin->ProgressBar(x + object->x1, y + object->y1, x + object->x2, y + object->y2, object->Data);
					if (object->Focus && focus)
						gfx->rectdot(object->x1 + x - 2, object->y1 + y - 2, object->x2 + x + 2, object->y2 + y + 2, DGray);
					break;

				case OBJ_TEXTBAR: // Text edit
					if (object->Text.empty())
						object->Text.push_back(nullptr);
					skin->TextBar(x + object->x1, y + object->y1, x + object->x2, y + object->y2, object->Text[0], object->Focus);
					if (object->Focus && focus)
						gfx->rectdot(object->x1 + x - 2, object->y1 + y - 2, object->x2 + x + 2, object->y2 + y + 2, DGray);
					break;

				case OBJ_TEXTEDITOR: // Large text edit
					if (object->Text.empty())
						object->Text.push_back(nullptr);
					skin->TextEditor(x + object->x1, y + object->y1, x + object->x2, y + object->y2, object->Text, object->Data, object->Pos, object->Focus);
					if (object->Focus && focus)
						gfx->rectdot(object->x1 + x - 2, object->y1 + y - 2, object->x2 + x + 2, object->y2 + y + 2, DGray);
					break;

				case OBJ_MENU: // Menu
					if (object->Text.empty())
						object->Text.push_back(nullptr);
					if (!object->Etat)
					{
						skin->button(x + object->x1, y + object->y1, x + object->x2, y + object->y2,
							object->Text[0], object->activated || object->Etat, !disabled);
						disabled = false;
					}
					break;
			}

			// Make it darker when disabled
			if (disabled)
			{
				glEnable(GL_BLEND);
				glDisable(GL_TEXTURE_2D);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
				gfx->rectfill(x + object->x1, y + object->y1, x + object->x2, y + object->y2);
				glEnable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);
			}

			// Highlight the object
			if ((object->Flag & FLAG_HIGHLIGHT) && object->MouseOn)
			{
				glEnable(GL_BLEND);
				glDisable(GL_TEXTURE_2D);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
				gfx->rectfill(x + object->x1, y + object->y1, x + object->x2, y + object->y2);
				glEnable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);
			}
		}

		void WND::doDrawWindowForegroundObject(Skin* skin, const int i)
		{
			GUIOBJ::Ptr object = pObjects[i];
			const float x = (float)this->x;
			const float y = (float)this->y;
			switch (object->Type)
			{
				case OBJ_FMENU: // Menu flottant
					skin->FloatMenu(x + object->x1, y + object->y1, object->Text,
						object->Data, 0);
					break;
				case OBJ_MENU: // Menu déroulant
					if (object->Etat)
					{
						skin->button(x + object->x1, y + object->y1,
							x + object->x2, y + object->y2,
							object->Text[0],
							object->activated || object->Etat, true);
						skin->FloatMenu(x + object->x1, y + object->y2 + 1.0f,
							object->Text, object->Data + 1,
							1 + object->Pos);
					}
					break;
			}
		}

		byte WND::WinMov(const int AMx, const int AMy, const int AMb, const int Mx, const int My, const int Mb, Skin* skin)
		{
			MutexLocker locker(pMutex);
			if (AMb == 1 && Mb == 1 && !Lock)
			{
				if (AMx >= x + 3 && AMx <= x + width - 4)
				{
					if (AMy >= y + 3 && AMy <= y + 3 + title_h)
					{
						x += Mx - AMx;
						y += My - AMy;
					}
				}
			}
			if (skin)
			{
				if ((float)Mx >= (float)x - skin->wnd_border.x1 && (float)Mx <= (float)x + (float)width - skin->wnd_border.x2 && (float)My >= (float)y - skin->wnd_border.y1 && (float)My <= (float)y + (float)height - skin->wnd_border.y2)
					return 1;
			}
			else
			{
				if (Mx >= x && Mx <= x + width && My >= y && My <= y + height)
					return 1;
			}
			return 0;
		}

		void WND::destroy()
		{
			pMutex.lock();
			Title.clear();
			Name.clear();
			if (delete_gltex)
			{
				gfx->destroy_texture(background);
				delete_gltex = false;
			}
			background = 0;
			background_clamp = false;
			pObjects.clear();
			pMutex.unlock();
		}

		void WND::doCheckWasOnFLoattingMenu(const int i, bool& wasOnFloattingMenu, int& indxMenu, Skin* skin)
		{
			GUIOBJ::Ptr object = pObjects[i];

			if (object->Type == OBJ_TA_BUTTON && object->current_state < object->gltex_states.size())
			{
				object->x2 = object->x1 + float(object->gltex_states[object->current_state].width - 1);
				object->y2 = object->y1 + float(object->gltex_states[object->current_state].height - 1);
			}

			// Vérifie si la souris est sur l'objet
			if ((float)mouse_x >= (float)x + object->x1 && (float)mouse_x <= (float)x + object->x2 && (float)mouse_y >= (float)y + object->y1 && (float)mouse_y <= (float)y + object->y2)
				return;

			if (object->Type == OBJ_MENU && object->Etat && object->MouseOn && !wasOnFloattingMenu)
			{
				float m_width = 168.0f;
				if (skin)
				{
					for (unsigned int e = 0; e < object->Text.size() - (1 + object->Pos); ++e)
						m_width = Math::Max(m_width, gui_font->length(object->Text[e]));

					m_width += skin->menu_background.x1 - skin->menu_background.x2;
				}
				else
					m_width = 168.0f;

				if ((float)mouse_x >= (float)x + object->x1 && (float)mouse_x <= (float)x + object->x1 + m_width && (float)mouse_y > (float)y + object->y2 && (float)mouse_y <= (float)y + object->y2 + 1.0f + pCacheFontHeight * (float)object->Text.size())
				{
					wasOnFloattingMenu = true;
					indxMenu = i;
				}
			}
		}

		int WND::check(int AMx, int AMy, int AMz, int AMb, bool timetoscroll, Skin* skin)
		{
			MutexLocker locker(pMutex);
			if (hidden)
			{
				was_hidden = true;
				return 0; // if it's hidden you cannot interact with it
			}
			if (was_hidden)
			{
				const ObjectList::iterator end = pObjects.end();
				for (ObjectList::iterator i = pObjects.begin(); i != end; ++i)
				{
					if ((*i)->Type == OBJ_MENU || (*i)->Type == OBJ_FMENU)
						(*i)->Etat = false;
				}
			}
			was_hidden = false;
			int IsOnGUI;
			// Checks whether the mouse is on the window and/or moves it
			IsOnGUI = WinMov(AMx, AMy, AMb, mouse_x, mouse_y, mouse_b, skin);
			// If there are no objects, we stop
			if (pObjects.empty())
				return IsOnGUI;

			// Handle scrolling
			if (scrollable && IsOnGUI)
			{
				scrolling += 10 * (AMz - mouse_z);
				scrolling = Math::Min(scrolling, y + height - gfx->height);
				scrolling = Math::Max(0, scrolling);
			}

			// User/Object interactions
			unsigned int index, e;
			CodePoint Key;
			bool was_on_floating_menu = false;
			int on_menu = -1;
			bool close_all = false;
			bool already_clicked = false;
			int hasFocus = -1;

			GUIOBJ::Ptr object;

			y -= scrolling;

			for (unsigned int i = 0; i < pObjects.size(); ++i)
			{
				object = pObjects[i];
				if (object->Type != OBJ_NONE)
					doCheckWasOnFLoattingMenu(i, was_on_floating_menu, on_menu, skin);
				if (object->Focus && object->Type != OBJ_TEXTEDITOR)
					hasFocus = i;
			}
			if (hasFocus >= 0 && isKeyDown(KEY_TAB) && !tab_was_pressed) // Select another widget with TAB key
			{
				for (unsigned int e = 1; e < pObjects.size(); ++e)
				{
					const size_t i = (e + hasFocus) % pObjects.size();
					object = pObjects[i];
					if (object->Flag & FLAG_CAN_GET_FOCUS)
					{
						pObjects[hasFocus]->Focus = false;
						object->Focus = true;
						break;
					}
				}
			}
			tab_was_pressed = isKeyDown(KEY_TAB);

			for (unsigned int i = 0; i < pObjects.size(); ++i)
			{
				object = pObjects[i];
				if (object->Type == OBJ_NONE)
					continue;

				bool MouseWasOn = object->MouseOn;
				object->MouseOn = false;
				if (object->wait_a_turn)
				{
					object->wait_a_turn = false;
					continue;
				}
				// Object is hidden so don't handle its events
				if ((object->Flag & FLAG_HIDDEN) == FLAG_HIDDEN)
					continue;

				if (on_menu == (int)i)
					was_on_floating_menu = false;

				// Checks whether the mouse is on the object
				if ((float)mouse_x >= (float)x + object->x1 && (float)mouse_x <= (float)x + object->x2 && (float)mouse_y >= (float)y + object->y1 && (float)mouse_y <= (float)y + object->y2 && !was_on_floating_menu)
				{
					object->MouseOn = true;
				}

				if (object->Type == OBJ_MENU && object->Etat && !object->MouseOn && !was_on_floating_menu)
				{
					float m_width = 168.0f;
					if (skin)
					{
						for (unsigned int e = 0; e < object->Text.size() - (1 + object->Pos); ++e)
							m_width = Math::Max(m_width, gui_font->length(object->Text[e]));

						m_width += skin->menu_background.x1 - skin->menu_background.x2;
					}

					if ((float)mouse_x >= (float)x + object->x1 && (float)mouse_x <= (float)x + object->x1 + m_width && (float)mouse_y > (float)y + object->y2 && (float)mouse_y <= (float)y + object->y2 + 1.0f + pCacheFontHeight * (float)object->Text.size())
						object->MouseOn = true;
				}

				if (object->MouseOn)
					IsOnGUI |= 2;

				if (mouse_b != 0 && object->MouseOn && !was_on_floating_menu) // Gets focus
				{
					for (e = 0; e < pObjects.size(); ++e)
						pObjects[e]->Focus = false;
					object->Focus = true;
				}

				if (mouse_b != 0 && !object->MouseOn) // Hav lost the focus
				{
					object->Focus = false;
					switch (object->Type)
					{
						case OBJ_MENU:
							object->Etat = false;
							break;
					}
				}

				if (object->MouseOn && (object->Type == OBJ_FMENU || object->Type == OBJ_MENU))
				{
					for (e = 0; e < pObjects.size(); ++e)
					{
						pObjects[e]->Focus = false;
						if (pObjects[e]->Type == OBJ_BUTTON)
							pObjects[e]->Etat = false;
					}
					was_on_floating_menu = object->Etat;
					object->Focus = true;
				}

				if (!(object->Flag & FLAG_CAN_GET_FOCUS))
					object->Focus = false;

				const bool previous_state = object->Etat;

				switch (object->Type)
				{
					case OBJ_MENU: // Things to do whatever happens
					{
						object->Data = (unsigned int)(-1); // No selection
						if (!object->Etat)
							object->Value = -1;
						{
							float m_width = 168.0f;
							if (skin)
							{
								for (unsigned int e = 0; e < object->Text.size() - (1 + object->Pos); ++e)
									m_width = Math::Max(m_width, gui_font->length(object->Text[e]));

								m_width += skin->menu_background.x1 - skin->menu_background.x2;
							}
							else
								m_width = 168.0f;

							if (object->MouseOn && (float)mouse_x >= (float)x + object->x1 && (float)mouse_x <= (float)x + object->x1 + m_width && (float)mouse_y > (float)y + object->y2 + 4.0f && (float)mouse_y <= (float)y + object->y2 + 1.0f + pCacheFontHeight * (float)object->Text.size() && object->Etat)
							{
								if (timetoscroll)
								{
									if ((float)mouse_y < (float)y + object->y2 + 12.0f && object->Pos > 0)
										object->Pos--;
									if (mouse_y > gfx->height - 8 && (float)y + object->y2 + 1.0f + pCacheFontHeight * float(object->Text.size() - object->Pos) > (float) gfx->height)
										object->Pos++;
								}
								object->Data = (int)(((float)mouse_y - (float)y - object->y2 - 5.0f) / float(pCacheFontHeight) + (float)object->Pos);
								if (object->Data >= object->Text.size() - 1)
									object->Data = (unsigned int)(-1);
							}
						}
						break;
					}
					case OBJ_FMENU:
					{
						object->Data = (unsigned int)(-1); // No selection
						if (object->MouseOn && (float)mouse_y >= (float)y + object->y1 + 4.0f && (float)mouse_y <= (float)y + object->y2 - 4.0f)
						{
							object->Data = (int)(((float)mouse_y - (float)y - object->y1 - 4.0f) / float(pCacheFontHeight));
							if (object->Data >= object->Text.size())
								object->Data = (unsigned int)(-1);
						}
						break;
					}
					case OBJ_TEXTBAR: // Allows inputting text
					{
						object->Etat = false;
						if (object->Focus && keyboardBufferContainsElements())
						{
							const auto keyElement = getNextKeyboardBufferElement();
							Key = keyElement.codePoint;
							const KeyCode scancode = keyElement.keyCode;

							switch (scancode)
							{
								case KEY_ENTER:
									object->Etat = true;
									if (object->Func != NULL)
										(*object->Func)(object->Text[0].utf8size());
									break;
								case KEY_BACKSPACE:
									if (object->Text[0].utf8size() > 0)
										object->Text[0] = SubstrUTF8(object->Text[0], 0, object->Text[0].utf8size() - 1);
									break;
								case KEY_TAB:
								case KEY_ESC:
									break;
								default:
									switch (Key)
									{
										case 9:
										case 27:
										case 0:
											break;
										default:
											if (object->Text[0].utf8size() + 1 < object->Data)
												object->Text[0] << InttoUTF8(Key);
									}
							}
						}
						break;
					}

					case OBJ_TEXTEDITOR: // Enable text input
					{
						if (object->Text.empty())
							object->Text.push_back(nullptr);
						if (object->Data >= object->Text.size())
							object->Data = GLuint(object->Text.size() - 1);

						if (object->Pos > object->Text[object->Data].utf8size())
							object->Pos = object->Text[object->Pos].utf8size();
						object->Etat = false;
						if (object->Focus && keyboardBufferContainsElements())
						{
							const auto keyElement = getNextKeyboardBufferElement();
							Key = keyElement.codePoint;
							const KeyCode scancode = keyElement.keyCode;
							switch (scancode)
							{
								case KEY_ESC:
									break;
								case KEY_TAB:
									object->Text[object->Data] << "    ";
									object->Pos += 4;
									break;
								case KEY_ENTER:
									object->Text.push_back(nullptr);
									if (object->Data + 1 < object->Text.size())
									{
										for (uint32 e = (uint32)object->Text.size() - 1; e > object->Data + 1; --e)
											object->Text[e] = object->Text[e - 1];
									}

									if (object->Text[object->Data].utf8size() - object->Pos > 0)
										object->Text[object->Data + 1] = SubstrUTF8(object->Text[object->Data], object->Pos, object->Text[object->Data].utf8size() - object->Pos);
									else
										object->Text[object->Data + 1].clear();
									object->Text[object->Data] = SubstrUTF8(object->Text[object->Data], 0, object->Pos);
									object->Pos = 0;
									object->Data++;
									break;
								case KEY_DEL:
									// Remove next character
									if (object->Pos < object->Text[object->Data].utf8size())
									{
										object->Text[object->Data] = SubstrUTF8(object->Text[object->Data], 0, object->Pos)
											<< SubstrUTF8(object->Text[object->Data], object->Pos + 1, object->Text[object->Data].utf8size() - object->Pos - 1);
									}
									else if (object->Data + 1 < object->Text.size())
									{
										object->Text[object->Data] << object->Text[object->Data + 1];
										for (uint32 e = object->Data + 1; e < object->Text.size() - 1; e++)
											object->Text[e] = object->Text[e + 1];
										object->Text.resize(object->Text.size() - 1);
									}
									break;
								case KEY_BACKSPACE: // Remove previous character
									if (object->Pos > 0)
									{
										object->Text[object->Data] = SubstrUTF8(object->Text[object->Data], 0, object->Pos - 1)
											<< SubstrUTF8(object->Text[object->Data], object->Pos, object->Text[object->Data].utf8size() - object->Pos);
										object->Pos--;
									}
									else if (object->Data > 0)
									{
										object->Data--;
										object->Pos = object->Text[object->Data].utf8size();
										object->Text[object->Data] << object->Text[object->Data + 1];
										for (unsigned int e = object->Data + 1; e < object->Text.size() - 1; ++e)
											object->Text[e] = object->Text[e + 1];
										object->Text.resize(object->Text.size() - 1);
									}
									break;
								case KEY_LEFT: // Left
									if (object->Pos > 0)
										object->Pos--;
									else if (object->Data > 0)
									{
										object->Data--;
										object->Pos = object->Text[object->Data].utf8size();
									}
									break;
								case KEY_RIGHT: // Right
									if (object->Pos < object->Text[object->Data].utf8size())
										object->Pos++;
									else if (object->Data + 1 < object->Text.size())
									{
										object->Data++;
										object->Pos = 0;
									}
									break;
								case KEY_UP: // Up
									if (object->Data > 0)
									{
										object->Data--;
										object->Pos = Math::Min((uint32)object->Text[object->Data].utf8size(), object->Pos);
									}
									break;
								case KEY_DOWN: // Down
									if (object->Data + 1 < object->Text.size())
									{
										object->Data++;
										object->Pos = Math::Min((uint32)object->Text[object->Data].utf8size(), object->Pos);
									}
									break;
								default:
									switch (Key)
									{
										case 0:
										case 27:
											break;
										default:
											object->Text[object->Data] = SubstrUTF8(object->Text[object->Data], 0, object->Pos)
												<< InttoUTF8(Key)
												<< SubstrUTF8(object->Text[object->Data], object->Pos, object->Text[object->Data].utf8size() - object->Pos);
											object->Pos++;
									}
							}
						}
						break;
					}

					case OBJ_LIST:
					{
						if ((object->MouseOn || object->Focus) && skin)
						{
							bool onDeco = (mouse_x - x <= object->x1 + skin->text_background.x1 || mouse_x - x >= object->x2 + skin->text_background.x2 || mouse_y - y <= object->y1 + skin->text_background.y1 || mouse_y - y >= object->y2 + skin->text_background.y2); // We're on ListBox decoration!
							int widgetSize = (int)((object->y2 - object->y1 - skin->text_background.y1 + skin->text_background.y2) / pCacheFontHeight);
							int TotalScroll = (int)object->Text.size() - widgetSize;
							if (TotalScroll < 0)
								TotalScroll = 0;

							if (isMouseButtonDown(LeftMouseButton) && !onDeco && mouse_x - x >= object->x2 + skin->text_background.x2 - skin->scroll[0].sw && mouse_x - x <= object->x2 + skin->text_background.x2 && mouse_y - y >= object->y1 + skin->text_background.y1 && mouse_y - y <= object->y2 + skin->text_background.y2) // We're on the scroll bar!
							{

								if (mouse_y - y > object->y1 + skin->text_background.y1 + skin->scroll[0].y1 && mouse_y - y < object->y2 + skin->text_background.y2 + skin->scroll[0].y2) // Set scrolling position
								{
									object->Data = (int)(0.5f + (float)TotalScroll * ((float)mouse_y - (float)y - object->y1 - skin->text_background.y1 - skin->scroll[0].y1 - (skin->scroll[0].sw - skin->scroll[0].x1 + skin->scroll[0].x2) * 0.5f) / (object->y2 - object->y1 - skin->text_background.y1 + skin->text_background.y2 - skin->scroll[0].y1 + skin->scroll[0].y2 - (skin->scroll[0].sw - skin->scroll[0].x1 + skin->scroll[0].x2) * 0.5f));
								}
								if (object->Data > (unsigned int)TotalScroll)
									object->Data = TotalScroll;
							}
							else
							{
								int nscroll = (int)object->Data - mouse_z + AMz;
								int npos = object->Pos;
								if (object->Focus)
								{
									KeyCode key_code = getNextKeyboardBufferElement().keyCode;
									if (key_code == KEY_UP)
										npos--;
									if (key_code == KEY_DOWN)
										npos++;
									if (npos != (int)object->Pos)
									{
										if (npos < 0)
											npos = 0;
										if (npos >= (int)object->Text.size())
											npos = (int)object->Text.size() - 1;
										if (nscroll > npos)
											nscroll = npos;
										if (nscroll + widgetSize <= npos)
											nscroll = npos - widgetSize + 1;
									}
								}

								if (nscroll < 0)
									nscroll = 0;
								else if (nscroll > TotalScroll)
									nscroll = TotalScroll;

								object->Data = nscroll;
								object->Pos = npos;
							}
						}
						break;
					}
					case OBJ_VSLIDER:
					{
						if (object->MouseOn)
						{
							if (mouse_y - y <= object->y1 + skin->scroll[0].y1) // Decrease
							{
							}
							else if (mouse_y - y >= object->y2 + skin->scroll[0].y2) // Increase
							{
							}
							else if (isMouseButtonDown(LeftMouseButton)) // Set
							{
								const int s = (int)skin->scroll[2].sh;
								const int nValue = (int)(((float)mouse_y - (float)y - object->y1 - skin->scroll[0].y1 - (float)(s / 2)) * (float)(object->Pos - object->Data + 1) / (object->y2 - object->y1 - skin->scroll[0].y1 + skin->scroll[0].y2 - (float)s)) + object->Data;
								if (nValue >= (int)object->Data && nValue <= (int)object->Pos)
									object->Value = nValue;
							}
							else if (AMz != mouse_z)
							{
								object->Value -= mouse_z - AMz;
								if (object->Value < (int)object->Data)
									object->Value = (int)object->Data;
								else if (object->Value > (int)object->Pos)
									object->Value = (int)object->Pos;
							}
						}
						break;
					}
					case OBJ_HSLIDER:
					{
						if (object->MouseOn)
						{
							if (mouse_x - x <= object->x1 + skin->scroll[1].x1) // Decrease
							{
							}
							else if (mouse_x - x >= object->x2 + skin->scroll[1].x2) // Increase
							{
							}
							else if (isMouseButtonDown(LeftMouseButton)) // Set
							{
								const int s = (int)skin->scroll[2].sw;
								const int nValue = (int)(((float)mouse_x - (float)x - object->x1 - skin->scroll[1].x1 - (float)(s / 2)) * (float)(object->Pos - object->Data + 1) / (object->x2 - object->x1 - skin->scroll[1].x1 + skin->scroll[1].x2 - (float)s)) + object->Data;
								if (nValue >= (int)object->Data && nValue <= (int)object->Pos)
									object->Value = nValue;
							}
							else if (AMz != mouse_z)
							{
								object->Value -= mouse_z - AMz;
								if (object->Value < (int)object->Data)
									object->Value = (int)object->Data;
								else if (object->Value > (int)object->Pos)
									object->Value = (int)object->Pos;
							}
						}
						break;
					}
				}
				if (object->Flag & FLAG_DISABLED)
				{
					object->activated = false;
					object->Etat = false;
				}
				else
				{
					if ((isMouseButtonUp(LeftMouseButton) || !object->MouseOn || mouse_b == AMb) && (object->Flag & FLAG_CAN_BE_CLICKED) && !(object->Flag & FLAG_SWITCH) && !(object->Etat ^ previous_state) && object->Etat && !was_on_floating_menu)
					{
						if (object->Func != NULL)
							(*object->Func)(0); // Lance la fonction associée
						object->Etat = false;
					}
					if (!object->activated && isMouseButtonDown(LeftMouseButton) && object->MouseOn && ((object->Flag & FLAG_CAN_BE_CLICKED) || (object->Flag & FLAG_SWITCH)))
					{
						switch (object->Type)
						{
							case OBJ_BOX:
							case OBJ_BUTTON:
							case OBJ_MENU:
							case OBJ_TA_BUTTON:
							case OBJ_OPTIONB:
							case OBJ_OPTIONC:
								if (sound_manager)
									sound_manager->playTDFSoundNow("SPECIALORDERS.sound");
						}
					}
					object->activated = isMouseButtonDown(LeftMouseButton) && object->MouseOn;

					bool clicked = false;
					if (
						object->shortcut_key >= 0
						&& object->shortcut_key <= 255
						&& lp_CONFIG->enable_shortcuts
						&& !isControlKeyDown()
						&& !isShiftKeyDown()
						&& !Console::Instance()->activated()
						&& isAsciiCharacterKeyDown((byte)object->shortcut_key))
					{
						if (!object->Etat)
							clicked = true;
						object->activated = object->Etat = true;
					}

					if (((isMouseButtonUp(LeftMouseButton) && AMb == 1) || clicked) && object->MouseOn && MouseWasOn && ((object->Flag & FLAG_CAN_BE_CLICKED) || (object->Flag & FLAG_SWITCH)) && !already_clicked) // Click sur l'objet
					{
						already_clicked = true;
						switch (object->Type)
						{
							case OBJ_VSLIDER:
							{
								if (mouse_y - y <= object->y1 + skin->scroll[0].y1) // Decrease
								{
									object->Value--;
									if (object->Value < (int)object->Data)
										object->Value = (int)object->Data;
								}
								else if ((float)mouse_y - (float)y >= object->y2 + skin->scroll[0].y2) // Increase
								{
									object->Value++;
									if (object->Value > (int)object->Pos)
										object->Value = (int)object->Pos;
								}
								else // Set
								{
									const int s = (int)skin->scroll[2].sh;
									const int nValue = (int)(((float)mouse_y - (float)y - object->y1 - skin->scroll[0].y1 - (float)(s / 2)) * (float)(object->Pos - object->Data + 1) / (object->y2 - object->y1 - skin->scroll[0].y1 + skin->scroll[0].y2 - (float)s)) + object->Data;
									if (nValue >= (int)object->Data && nValue <= (int)object->Pos)
										object->Value = nValue;
								}
								break;
							}
							case OBJ_HSLIDER:
							{
								if ((float)mouse_x - (float)x <= object->x1 + skin->scroll[1].x1) // Decrease
								{
									object->Value--;
									if (object->Value < (int)object->Data)
										object->Value = (int)object->Data;
								}
								else if ((float)mouse_x - (float)x >= object->x2 + skin->scroll[1].x2) // Increase
								{
									object->Value++;
									if (object->Value > (int)object->Pos)
										object->Value = (int)object->Pos;
								}
								else // Set
								{
									const int s = (int)skin->scroll[2].sw;
									int nValue = (int)(((float)mouse_x - (float)x - object->x1 - skin->scroll[1].x1 - (float)(s / 2)) * (float)(object->Pos - object->Data + 1) / (object->x2 - object->x1 - skin->scroll[1].x1 + skin->scroll[1].x2 - (float)s)) + object->Data;
									if (nValue >= (int)object->Data && nValue <= (int)object->Pos)
										object->Value = nValue;
								}
								break;
							}
							case OBJ_LIST:
								if (skin && mouse_x - x >= object->x2 + skin->text_background.x2 - skin->scroll[0].sw && mouse_x - x <= object->x2 + skin->text_background.x2 && mouse_y - y >= object->y1 + skin->text_background.y1 && mouse_y - y <= object->y2 + skin->text_background.y2) // We're on the scroll bar!
								{

									int TotalScroll = (int)object->Text.size() - (int)((object->y2 - object->y1 - skin->text_background.y1 + skin->text_background.y2) / pCacheFontHeight);
									if (TotalScroll < 0)
										TotalScroll = 0;

									if ((float)mouse_y - (float)y <= object->y1 + skin->text_background.y1 + skin->scroll[0].y1) // Scroll up
									{
										if (object->Data > 0)
											object->Data--;
										if (sound_manager)
											sound_manager->playTDFSoundNow("SPECIALORDERS.sound");
									}
									else
									{
										if ((float)mouse_y - (float)y >= object->y2 + skin->text_background.y2 + skin->scroll[0].y2) // Scroll down
										{
											object->Data++;
											if (sound_manager)
												sound_manager->playTDFSoundNow("SPECIALORDERS.sound");
										}
										else
										{ // Set scrolling position
											object->Data = (int)(0.5f + (float)TotalScroll * ((float)mouse_y - (float)y - object->y1 - skin->text_background.y1 - skin->scroll[0].y1 - (skin->scroll[0].sw - skin->scroll[0].x1 + skin->scroll[0].x2) * 0.5f) / (object->y2 - object->y1 - skin->text_background.y1 + skin->text_background.y2 - skin->scroll[0].y1 + skin->scroll[0].y2 - (skin->scroll[0].sw - skin->scroll[0].x1 + skin->scroll[0].x2) * 0.5f));
										}
									}
									if (object->Data > (unsigned int)TotalScroll)
										object->Data = (unsigned int)TotalScroll;
								}
								else
								{
									if (skin && (mouse_x - x <= object->x1 + skin->text_background.x1 || mouse_x - x >= object->x2 + skin->text_background.x2 || mouse_y - y <= object->y1 + skin->text_background.y1 || mouse_y - y >= object->y2 + skin->text_background.y2)) // We're on ListBox decoration!
										break;
									object->Pos = (uint32)(((float)mouse_y - (float)y - object->y1 - (skin ? skin->text_background.y1 : 4)) / pCacheFontHeight + (float)object->Data);
									object->Etat = true;
								}
								break;

							case OBJ_TA_BUTTON:
								if (object->nb_stages > 0)
									object->current_state = (++object->current_state) % object->nb_stages;
								object->Etat = true;
								break;

							case OBJ_BOX:	// Rectangle
							case OBJ_IMG:	// Image
							case OBJ_BUTTON: // Boutton
								if (was_on_floating_menu)
									break;
								object->Etat = true;
								break;

							case OBJ_OPTIONC: // Case à cocher
								if (was_on_floating_menu)
									break;
								if (skin && skin->checkbox[0].tex && skin->checkbox[1].tex)
								{
									if ((float)mouse_x <= (float)x + object->x1 + skin->checkbox[object->Etat ? 1 : 0].sw && (float)mouse_y <= (float)y + object->y1 + skin->checkbox[object->Etat ? 1 : 0].sh)
										object->Etat ^= true;
								}
								else if ((float)mouse_x <= (float)x + object->x1 + 12.0f && (float)mouse_y <= (float)y + object->y1 + 12.0f)
									object->Etat ^= true;
								if (object->Func != NULL)
									(*object->Func)(object->Etat); // Lance la fonction associée
								break;

							case OBJ_OPTIONB: // Bouton d'option
								if (was_on_floating_menu)
									break;
								if (skin && skin->option[0].tex && skin->option[1].tex)
								{
									if ((float)mouse_x <= (float)x + object->x1 + skin->option[object->Etat ? 1 : 0].sw && (float)mouse_y <= (float)y + object->y1 + skin->option[object->Etat ? 1 : 0].sh)
										object->Etat ^= true;
								}
								else if ((float)mouse_x <= (float)x + object->x1 + 12.0f && (float)mouse_y <= (float)y + object->y1 + 12.0f)
									object->Etat ^= true;
								if (object->Func != NULL)
									(*object->Func)(object->Etat); // Lance la fonction associée
								break;

							case OBJ_FMENU: // Menu Flottant
								if ((float)mouse_y >= (float)y + object->y1 + (skin ? skin->menu_background.y1 : 0.0f) + 4.0f && (float)mouse_y <= (float)y + object->y2 + (skin ? skin->menu_background.y2 : 0.0f) - 4.0f)
								{
									index = (int)(((float)mouse_y - (float)y - object->y1 - 4.0f - (skin ? skin->menu_background.y1 : 0.0f)) / pCacheFontHeight);
									if (index >= object->Text.size())
										index = (int)object->Text.size() - 1;
									if (object->Func != NULL)
										(*object->Func)(index); // Lance la fonction associée
								}
								break;

							case OBJ_MENU: // Menu déroulant
							{
								float m_width = 168.0f;
								if (skin)
								{
									for (unsigned int e = 0; e < object->Text.size() - (1 + object->Pos); e++)
										m_width = Math::Max(m_width, gui_font->length(object->Text[e]));

									m_width += skin->menu_background.x1 - skin->menu_background.x2;
								}
								else
									m_width = 168.0f;
								if ((float)mouse_x >= (float)x + object->x1 + (skin ? skin->menu_background.x1 : 0.0f) && (float)mouse_x <= (float)x + object->x1 + m_width + (skin ? skin->menu_background.x2 : 0.0f) && (float)mouse_y > (float)y + object->y2 + (skin ? skin->menu_background.y1 : 0.0f) && (float)mouse_y <= (float)y + object->y2 + (skin ? skin->menu_background.y2 : 0.0f) + 1.0f + pCacheFontHeight * (float)object->Text.size() && object->Etat)
								{
									index = (int)(((float)mouse_y - (float)y - object->y2 - 5.0f - (skin ? skin->menu_background.y1 : 0.0f)) / pCacheFontHeight + (float)object->Pos);
									if (index >= (unsigned int)(object->Text.size() - 1))
										index = (int)object->Text.size() - 2;
									if (object->Func != NULL)
										(*object->Func)(index); // Lance la fonction associée
									object->Value = object->Data;
									object->Etat = false;
									close_all = true;
								}
								else
									object->Etat ^= true;
							}
							break;

							default:
								object->Etat = true;
						}
						// Send a signal to the interface (the OnClick signal defined at initialization time)
						for (unsigned int cur = 0; cur < object->OnClick.size(); ++cur)
							I_Msg(TA3D::TA3D_IM_GUI_MSG, object->OnClick[cur]);
					}
					else if (object->MouseOn) // Send a signal to the interface (the OnHover signal defined at initialization time)
						for (unsigned int cur = 0; cur < object->OnHover.size(); cur++)
							I_Msg(TA3D::TA3D_IM_GUI_MSG, object->OnHover[cur]);
				}

				for (unsigned int cur = 0; cur < object->SendDataTo.size(); ++cur) // Send Data to an Object
				{
					String::size_type e = object->SendDataTo[cur].find('.');
					if (e != String::npos)
					{
						unsigned int target = Substr(object->SendDataTo[cur], 0, e).to_uint();
						if (target < pObjects.size())
						{
							if (Substr(object->SendDataTo[cur], e + 1, object->SendDataTo[cur].length() - e) == "data")
								pObjects[target]->Data = object->Data;
							else
								pObjects[target]->Pos = object->Data;
						}
					}
				}
				for (unsigned int cur = 0; cur < object->SendPosTo.size(); ++cur) // Send Pos to an Object
				{
					String::size_type e = object->SendPosTo[cur].find('.');
					if (e != String::npos)
					{
						unsigned int target = Substr(object->SendPosTo[cur], 0, e).to_uint();
						if (target < pObjects.size())
						{
							if (Substr(object->SendPosTo[cur], e + 1, object->SendPosTo[cur].length() - e) == "data")
								pObjects[target]->Data = object->Pos;
							else
								pObjects[target]->Pos = object->Pos;
						}
					}
				}
			}
			if (close_all)
			{
				ObjectList::iterator end = pObjects.end();
				for (ObjectList::iterator i = pObjects.begin(); i != end; ++i)
				{
					if ((*i)->Type == OBJ_MENU)
						(*i)->Etat = false;
				}
			}
			y += scrolling;
			return IsOnGUI;
		}

		uint32 WND::msg(const String& message)
		{
			MutexLocker locker(pMutex);
			String::size_type i = message.find('.');

			if (i != String::npos) // When it targets a subobject
			{
				GUIOBJ::Ptr obj = doGetObject(Substr(message, 0, i));
				if (obj)
					return obj->msg(Substr(message, i + 1, message.size() - i - 1), this);
			}
			else // When it targets the window itself
			{
				if (ToLower(message) == "show")
				{
					hidden = false;
					return INTERFACE_RESULT_HANDLED;
				}
				if (ToLower(message) == "hide")
				{
					hidden = true;
					return INTERFACE_RESULT_HANDLED;
				}
				if (ToLower(message) == "enablescrolling")
				{
					scrollable = true;
					return INTERFACE_RESULT_HANDLED;
				}
				if (ToLower(message) == "disablescrolling")
				{
					scrollable = false;
					return INTERFACE_RESULT_HANDLED;
				}
			}
			return INTERFACE_RESULT_CONTINUE;
		}

		bool WND::get_state(const String& message)
		{
			MutexLocker locker(pMutex);
			GUIOBJ::Ptr obj = doGetObject(message);
			if (obj)
				return obj->Etat;
			if (message.empty())
				return !hidden;
			return false;
		}

		sint32 WND::get_value(const String& message)
		{
			MutexLocker locker(pMutex);
			GUIOBJ::Ptr obj = doGetObject(message);
			return (!obj) ? -1 : obj->Value;
		}

		String WND::caption(const String& message)
		{
			MutexLocker locker(pMutex);
			GUIOBJ::Ptr obj = doGetObject(message);
			if (obj)
			{
				if (!obj->Text.empty())
				{
					if (obj->Type == OBJ_TEXTEDITOR)
					{
						String result = obj->Text[0];
						for (size_t i = 1; i < obj->Text.size(); ++i)
							result << '\n'
								   << obj->Text[i];
						return result;
					}
					return obj->Text[0];
				}
				return nullptr;
			}
			return (message.empty()) ? Title : nullptr;
		}

		GUIOBJ::Ptr WND::doGetObject(const String& message)
		{
			if (message.empty())
				return GUIOBJ::Ptr();

			const sint16 e = sint16(obj_hashtable[ToLower(message)] - 1);
			return (e >= 0) ? pObjects[e] : GUIOBJ::Ptr();
		}

		GUIOBJ::Ptr WND::get_object(const String& message)
		{
			if (message.empty())
				return GUIOBJ::Ptr();
			String lowered = ToLower(message);
			if (obj_hashtable.count(lowered) == 0)
				return GUIOBJ::Ptr();

			MutexLocker locker(pMutex);
			const sint16 e = sint16(obj_hashtable[lowered] - 1);
			return (e >= 0) ? pObjects[e] : GUIOBJ::Ptr();
		}

		void WND::load_gui(const String& filename, TA3D::HashMap<std::vector<TA3D::Interfaces::GfxTexture>*>::Dense& gui_hashtable)
		{
			ingame_window = true;

			TDFParser wndFile(filename, false, false, true);

			// Grab the window's name, so we can send signals to it (to hide/show for example)
			Name = Paths::ExtractFileNameWithoutExtension(filename);

			hidden = !wndFile.pullAsBool("gadget0.common.active");

			Title.clear();
			x = wndFile.pullAsInt("gadget0.common.xpos");
			y = wndFile.pullAsInt("gadget0.common.ypos");

			width = wndFile.pullAsInt("gadget0.common.width");
			height = wndFile.pullAsInt("gadget0.common.height");

			float x_factor = 1.0f;
			float y_factor = 1.0f;

			Lock = true;
			draw_borders = false;
			show_title = false;
			delete_gltex = false;

			String panel = wndFile.pullAsString("gadget0.panel"); // Look for the panel texture
			int w;
			int h;

			background = Gaf::ToTexture(String("anims\\") << Name, panel, &w, &h, true);
			if (background == 0) // Try GAF-like directory structure
				background = Gaf::ToTexture(String("anims\\") << Name << ".gaf", panel, &w, &h, true);
			if (background == 0) // Try GAF-like directory structure
				background = Gaf::ToTexture("anims\\commongui", panel, &w, &h, true);
			if (background == 0)
				background = Gaf::ToTexture("anims\\commongui.gaf", panel, &w, &h, true);
			if (background == 0)
			{
				String::Vector file_list;
				VFS::Instance()->getDirlist("anims\\*", file_list);		 // GAF-like directories
				VFS::Instance()->getFilelist("anims\\*.gaf", file_list); // Normal GAF files
				for (String::Vector::const_iterator i = file_list.begin(); i != file_list.end() && background == 0; ++i)
				{
					LOG_DEBUG("trying(1) " << *i << " (" << Name << ")");
					background = Gaf::ToTexture(*i, panel, &w, &h, true, FILTER_LINEAR);
				}
			}

			background_clamp = true;
			background_width = w;
			background_height = h;

			delete_gltex = background;
			background_wnd = background;
			color = background ? makeacol(0xFF, 0xFF, 0xFF, 0xFF) : 0x0;
			unsigned int NbObj = wndFile.pullAsInt("gadget0.totalgadgets");

			pObjects.clear();
			pObjects.reserve(NbObj);

			String obj_key;
			for (unsigned int i = 0; i < NbObj; ++i)
			{
				GUIOBJ::Ptr object = GUIOBJ::Ptr(new GUIOBJ());
				pObjects.push_back(object);

				obj_key.clear();
				obj_key << "gadget" << i + 1 << ".";
				int obj_type = wndFile.pullAsInt(String(obj_key) << "common.id");

				object->Name = wndFile.pullAsString(String(obj_key) << "common.name", String("gadget") << (i + 1));
				obj_hashtable[ToLower(object->Name)] = i + 1;

				int X1 = (int)((float)wndFile.pullAsInt(String(obj_key) << "common.xpos") * x_factor); // Reads data from TDF
				int Y1 = (int)((float)wndFile.pullAsInt(String(obj_key) << "common.ypos") * y_factor);
				int W = (int)((float)wndFile.pullAsInt(String(obj_key) << "common.width") * x_factor - 1);
				int H = (int)((float)wndFile.pullAsInt(String(obj_key) << "common.height") * y_factor - 1);

				//float size = Math::Min(x_factor, y_factor);
				uint32 obj_flags = 0;

				if (X1 < 0)
					X1 += gfx->width;
				if (Y1 < 0)
					Y1 += gfx->height;

				if (!wndFile.pullAsBool(String(obj_key) << "common.active"))
					obj_flags |= FLAG_HIDDEN;

				String::Vector Caption;
				wndFile.pullAsString(String(obj_key) << "text").explode(Caption, ',', true, true, true);
				I18N::Translate(Caption);

				if (TA_ID_BUTTON == obj_type)
				{
					int t_w[100];
					int t_h[100];
					String key(object->Name);
					key.toLower();
					std::vector<TA3D::Interfaces::GfxTexture>* result = gui_hashtable[key];

					std::vector<GLuint> gaf_imgs;
					bool found_elsewhere = false;

					if (!result)
					{
						Gaf::ToTexturesList(gaf_imgs, String("anims\\") << Name, object->Name, t_w, t_h, true, FILTER_LINEAR);
						if (!gaf_imgs.size()) // Try GAF-like directory
							Gaf::ToTexturesList(gaf_imgs, String("anims\\") << Name << ".gaf", object->Name, t_w, t_h, true, FILTER_LINEAR);
						if (!gaf_imgs.size())
						{
							Gaf::ToTexturesList(gaf_imgs, "anims\\commongui", object->Name, t_w, t_h, true, FILTER_LINEAR);
							found_elsewhere = true;
						}
						if (!gaf_imgs.size())
						{
							Gaf::ToTexturesList(gaf_imgs, "anims\\commongui.gaf", object->Name, t_w, t_h, true, FILTER_LINEAR);
							found_elsewhere = true;
						}
						if (!gaf_imgs.size())
						{
							String::Vector file_list;
							VFS::Instance()->getDirlist("anims\\*", file_list);		 // GAF-like directories
							VFS::Instance()->getFilelist("anims\\*.gaf", file_list); // Normal GAF files
							for (String::Vector::const_iterator e = file_list.begin(); e != file_list.end() && gaf_imgs.size() == 0; ++e)
							{
								LOG_DEBUG("trying(0) " << *e << " (" << Name << ")");
								Gaf::ToTexturesList(gaf_imgs, *e, object->Name, t_w, t_h, true, FILTER_LINEAR);
							}
							if (gaf_imgs.size() > 0)
								found_elsewhere = true;
						}
					}
					else
					{
						gaf_imgs.resize(result->size());
						for (unsigned int e = 0; e < result->size(); ++e)
						{
							gaf_imgs[e] = (*result)[e].tex;
							t_w[e] = (*result)[e].width;
							t_h[e] = (*result)[e].height;
						}
					}

					const int nb_stages = wndFile.pullAsInt(String(obj_key) << "stages");
					object->create_ta_button((float)X1, (float)Y1, Caption, gaf_imgs, nb_stages > 0 ? nb_stages : (int)gaf_imgs.size() - 2);
					if (result == NULL && found_elsewhere)
						gui_hashtable[key] = &object->gltex_states;
					for (unsigned int e = 0; e < object->gltex_states.size(); ++e)
					{
						object->gltex_states[e].width = t_w[e];
						object->gltex_states[e].height = t_h[e];
						if (result)
							object->gltex_states[e].destroy_tex = false;
						else
							object->gltex_states[e].destroy_tex = true;
					}
					object->current_state = (byte)wndFile.pullAsInt(String(obj_key) << "status");
					object->shortcut_key = (sint16)wndFile.pullAsInt(String(obj_key) << "quickkey", -1);
					if (wndFile.pullAsBool(String(obj_key) << "common.grayedout"))
						object->Flag |= FLAG_DISABLED;
					if (wndFile.pullAsInt(String(obj_key) << "common.attribs") == 32)
						object->Flag |= FLAG_HIDDEN | FLAG_BUILD_PIC;
				}
				else
				{
					if (obj_type == TA_ID_TEXT_FIELD)
						object->create_textbar((float)X1, (float)Y1, (float)X1 + (float)W, (float)Y1 + (float)H, Caption.size() > 0 ? Caption[0] : "", wndFile.pullAsInt(String(obj_key) << "maxchars"), NULL);
					else
					{
						if (obj_type == TA_ID_LABEL)
							object->create_text((float)X1, (float)Y1, Caption.size() ? Caption[0] : String(), 0xFFFFFFFF, 1.0f);
						else
						{
							if (obj_type == TA_ID_BLANK_IMG || obj_type == TA_ID_IMG)
							{
								object->create_img((float)X1, (float)Y1, (float)X1 + (float)W, (float)Y1 + (float)H, gfx->load_texture(wndFile.pullAsString(String(obj_key) << "source"), FILTER_LINEAR));
								object->destroy_img = object->Data != 0 ? true : false;
							}
							else
							{
								if (obj_type == TA_ID_LIST_BOX)
									object->create_list((float)X1, (float)Y1, (float)X1 + (float)W, (float)Y1 + (float)H, Caption);
								else
									object->Type = OBJ_NONE;
							}
						}
					}
				}

				object->OnClick.clear();
				object->OnHover.clear();
				object->SendDataTo.clear();
				object->SendPosTo.clear();

				object->Flag |= obj_flags;
			}
		}

		void WND::print(std::ostream& out)
		{
			out << "[Window]\n{\n";
			out << "\tx = " << x << "\n";
			out << "\ty = " << y << "\n";
			out << "\tname = " << Name << "\n";
			out << "\ttitle = " << Title << "\n";
			out << "}\n"
				<< std::endl;
		}

		void WND::load_tdf(const String& filename, Skin* skin)
		{
			TDFParser wndFile(filename, false, false, false, false, true);

			Name = Paths::ExtractFileNameWithoutExtension(filename); // Grab the window's name, so we can send signals to it (to hide/show for example)

			Name = wndFile.pullAsString("window.name", Name);
			hidden = wndFile.pullAsBool("window.hidden");

			Title = I18N::Translate(wndFile.pullAsString("window.title"));
			x = wndFile.pullAsInt("window.x");
			y = wndFile.pullAsInt("window.y");
			width = wndFile.pullAsInt("window.width");
			height = wndFile.pullAsInt("window.height");
			repeat_bkg = wndFile.pullAsBool("window.repeat background", false);
			get_focus = wndFile.pullAsBool("window.get focus", false);

			float x_factor = 1.0f;
			float y_factor = 1.0f;
			if (wndFile.pullAsBool("window.fullscreen"))
			{
				const int ref_width = wndFile.pullAsInt("window.screen width", width);
				const int ref_height = wndFile.pullAsInt("window.screen height", height);
				if (ref_width > 0.0f)
					x_factor = ((float)gfx->width) / (float)ref_width;
				if (ref_height > 0.0f)
					y_factor = ((float)gfx->height) / (float)ref_height;
				width = (int)((float)width * x_factor);
				height = (int)((float)height * y_factor);
				x = (int)((float)x * x_factor);
				y = (int)((float)y * y_factor);
			}

			if (x < 0)
				x += gfx->width;
			if (y < 0)
				y += gfx->height;
			if (width < 0)
				width += gfx->width;
			if (height < 0)
				height += gfx->height;

			if (wndFile.pullAsBool("window.centered"))
			{
				x = (gfx->width - width) >> 1;
				y = (gfx->height - height) >> 1;
			}

			size_factor = (float)gfx->height / 600.0f; // For title bar

			ingame_window = wndFile.pullAsBool("window.ingame", false);

			background_wnd = wndFile.pullAsBool("window.background window");
			Lock = wndFile.pullAsBool("window.lock");
			draw_borders = wndFile.pullAsBool("window.draw borders");
			show_title = wndFile.pullAsBool("window.show title");
			delete_gltex = false;
			String backgroundImage = wndFile.pullAsString("window.background");
			if (VFS::Instance()->fileExists(backgroundImage))
			{
				background = gfx->load_texture(backgroundImage, FILTER_LINEAR, &bkg_w, &bkg_h, false);
				delete_gltex = true;
			}
			else
			{
				background = skin->wnd_background;
				bkg_w = skin->bkg_w;
				bkg_h = skin->bkg_h;
				delete_gltex = false;
			}
			color = wndFile.pullAsInt("window.color", delete_gltex ? 0xFFFFFFFF : makeacol(0x7F, 0x7F, 0x7F, 0xFF));
			FIX_COLOR(color);

			pObjects.clear();

			String obj_key;
			String obj_type;
			String caption;
			String::Vector Entry;
			String entryList;

			for (unsigned int i = 0; wndFile.exists(String("widget") << i); ++i) // Loads each object
			{
				obj_key.clear();
				obj_key << "window.widget" << i << ".";

				// Type of the new object
				obj_type = wndFile.pullAsString(String(obj_key) << "type");
				if (obj_type.empty())
					continue;
				obj_type.toUpper();

				// Creating a new instance
				GUIOBJ::Ptr object = GUIOBJ::Ptr(new GUIOBJ());
				pObjects.push_back(object);

				object->Name = wndFile.pullAsString(String(obj_key) << "name", String("widget") << i);
				obj_hashtable[ToLower(object->Name)] = i + 1;
				object->help_msg = I18N::Translate(wndFile.pullAsString(String(obj_key) << "help"));

				float X1 = wndFile.pullAsFloat(String(obj_key) << "x1") * x_factor; // Reads data from TDF
				float Y1 = wndFile.pullAsFloat(String(obj_key) << "y1") * y_factor;
				float X2 = wndFile.pullAsFloat(String(obj_key) << "x2") * x_factor;
				float Y2 = wndFile.pullAsFloat(String(obj_key) << "y2") * y_factor;
				caption = I18N::Translate(wndFile.pullAsString(String(obj_key) << "caption"));
				float size_factor = wndFile.pullAsFloat(String(obj_key) << "size", 1.0f);
				float size = size_factor * Math::Min(x_factor, y_factor);
				int val = wndFile.pullAsInt(String(obj_key) << "value");
				uint32 obj_flags = 0;
				uint32 obj_negative_flags = 0;

				if (X1 < 0)
					X1 += (float)width;
				if (X2 < 0)
					X2 += (float)width;
				if (Y1 < 0)
					Y1 += (float)height;
				if (Y2 < 0)
					Y2 += (float)height;

				if (wndFile.pullAsBool(String(obj_key) << "can be clicked"))
					obj_flags |= FLAG_CAN_BE_CLICKED;
				if (wndFile.pullAsBool(String(obj_key) << "can get focus"))
					obj_flags |= FLAG_CAN_GET_FOCUS;
				if (wndFile.pullAsBool(String(obj_key) << "highlight"))
					obj_flags |= FLAG_HIGHLIGHT;
				if (wndFile.pullAsBool(String(obj_key) << "fill"))
					obj_flags |= FLAG_FILL;
				if (wndFile.pullAsBool(String(obj_key) << "hidden"))
					obj_flags |= FLAG_HIDDEN;
				if (wndFile.pullAsBool(String(obj_key) << "no border"))
					obj_flags |= FLAG_NO_BORDER;
				if (wndFile.pullAsBool(String(obj_key) << "cant be clicked"))
					obj_negative_flags |= FLAG_CAN_BE_CLICKED;
				if (wndFile.pullAsBool(String(obj_key) << "cant get focus"))
					obj_negative_flags |= FLAG_CAN_GET_FOCUS;

				if (wndFile.pullAsBool(String(obj_key) << "centered"))
				{
					obj_flags |= FLAG_CENTERED;
					X1 -= gui_font->length(caption) * 0.5f;
				}

				entryList.clear();
				entryList << ' ' << wndFile.pullAsString(String(obj_key) << "entry");
				entryList.explode(Entry, ',', true, true, true);
				I18N::Translate(Entry);

				switch (obj_type.first())
				{
					case 'B':
					{
						if (obj_type == "BUTTON")
						{
							object->create_button(X1, Y1, X2, Y2, caption, NULL, size);
							break;
						}
						if (obj_type == "BOX")
						{
							FIX_COLOR(val);
							object->create_box(X1, Y1, X2, Y2, val);
						}
						break;
					}
					case 'F':
					{
						if (obj_type == "FMENU")
							object->create_menu(X1, Y1, Entry, NULL, size);
						break;
					}
					case 'H':
					{
						if (obj_type == "HSLIDER")
						{
							object->create_hslider(X1, Y1, X2, Y2, wndFile.pullAsInt(String(obj_key) << "min"), wndFile.pullAsInt(String(obj_key) << "max"), val);
							break;
						}
						break;
					}
					case 'I':
					{
						if (obj_type == "IMG")
						{
							object->create_img(X1, Y1, X2, Y2, gfx->load_texture(I18N::Translate(wndFile.pullAsString(String(obj_key) << "source"))));
							object->destroy_img = object->Data != 0 ? true : false;
						}
						break;
					}
					case 'M':
					{
						if (obj_type == "MENU")
						{
							object->create_menu(X1, Y1, X2, Y2, Entry, NULL, size);
							break;
						}
						if (obj_type == "MULTISTATE")
						{
							String::Vector imageNames;
							caption.explode(imageNames, ',', false, true, true);
							std::vector<GLuint> gl_imgs;
							std::vector<uint32> t_w;
							std::vector<uint32> t_h;

							for (String::Vector::iterator e = imageNames.begin(); e != imageNames.end(); e++)
							{
								uint32 tw, th;
								GLuint texHandle = gfx->load_texture(*e, FILTER_LINEAR, &tw, &th);
								if (texHandle)
								{
									gl_imgs.push_back(texHandle);
									t_w.push_back(tw);
									t_h.push_back(th);
								}
							}

							object->create_ta_button(X1, Y1, Entry, gl_imgs, (int)gl_imgs.size());
							for (unsigned int e = 0; e < object->gltex_states.size(); ++e)
							{
								object->x2 = X1 + (float)t_w[e] * size_factor * x_factor;
								object->y2 = Y1 + (float)t_h[e] * size_factor * y_factor;
								object->gltex_states[e].width = (int)((float)t_w[e] * size_factor * x_factor);
								object->gltex_states[e].height = (int)((float)t_h[e] * size_factor * x_factor);
								object->gltex_states[e].destroy_tex = true; // Make sure it'll be destroyed
							}
							break;
						}
						if (obj_type == "MISSION")
						{
							object->create_text(X1, Y1, caption, val, size);
							if (X2 > 0 && Y2 > Y1)
							{
								object->x2 = X2;
								object->y2 = Y2;
								object->Flag |= FLAG_TEXT_ADJUST | FLAG_MISSION_MODE | FLAG_CAN_BE_CLICKED;
							}
							break;
						}
						break;
					}
					case 'L':
					{
						if (obj_type == "LINE")
						{
							FIX_COLOR(val);
							object->create_line(X1, Y1, X2, Y2, val);
							break;
						}
						if (obj_type == "LIST")
							object->create_list(X1, Y1, X2, Y2, Entry, size);
						break;
					}
					case 'O':
					{
						if (obj_type == "OPTIONB")
						{
							object->create_optionb(X1, Y1, caption, val, NULL, skin, size);
							break;
						}
						if (obj_type == "OPTIONC")
						{
							object->create_optionc(X1, Y1, caption, val, NULL, skin, size);
							break;
						}
						break;
					}
					case 'P':
					{
						if (obj_type == "PBAR")
							object->create_pbar(X1, Y1, X2, Y2, val, size);
						break;
					}
					case 'T':
					{
						if (obj_type == "TEXTEDITOR")
						{
							object->create_texteditor(X1, Y1, X2, Y2, caption, size);
							break;
						}
						if (obj_type == "TEXTBAR")
						{
							object->create_textbar(X1, Y1, X2, Y2, caption, val, NULL, size);
							break;
						}
						if (obj_type == "TEXT")
						{
							FIX_COLOR(val);
							object->create_text(X1, Y1, caption, val, size);
							if (X2 > 0 && Y2 > Y1)
							{
								object->x2 = X2;
								object->y2 = Y2;
								object->Flag |= FLAG_TEXT_ADJUST;
							}
							break;
						}
						break;
					}
					case 'V':
					{
						if (obj_type == "VSLIDER")
						{
							object->create_vslider(X1, Y1, X2, Y2, wndFile.pullAsInt(String(obj_key) << "min"), wndFile.pullAsInt(String(obj_key) << "max"), val);
							break;
						}

						break;
					}
				}

				wndFile.pullAsString(String(obj_key) << "on click").explode(object->OnClick, ',', true, true, true);
				wndFile.pullAsString(String(obj_key) << "on hover").explode(object->OnHover, ',', true, true, true);
				wndFile.pullAsString(String(obj_key) << "send data to").toLower().explode(object->SendDataTo, ',', true, true, true);
				wndFile.pullAsString(String(obj_key) << "send pos to").toLower().explode(object->SendPosTo, ',', true, true, true);

				object->Flag |= obj_flags;
				object->Flag &= ~obj_negative_flags;
			}
		}

		unsigned int WND::size()
		{
			MutexLocker locker(pMutex);
			return (unsigned int)pObjects.size();
		}

		unsigned int WND::count()
		{
			MutexLocker locker(pMutex);
			return (unsigned int)pObjects.size();
		}

		void WND::focus(bool value)
		{
			MutexLocker locker(pMutex);
			const ObjectList::iterator end = pObjects.end();
			for (ObjectList::iterator i = pObjects.begin(); i != end; ++i)
				(*i)->Focus = value;
		}

		GUIOBJ::Ptr WND::object(unsigned int indx)
		{
			MutexLocker locker(pMutex);
			assert(indx < pObjects.size());
			return pObjects[indx];
		}

	} // namespace Gui
} // namespace TA3D
