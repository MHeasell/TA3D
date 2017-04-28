/*  TA3D, a remake of Total Annihilation
	Copyright (C) 2006  Roland BROCHARD

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

#include <stdafx.h>
#include <ta3dbase.h> // just to use the global camera object
#include <ingame/sidedata.h>
#include <misc/math.h>
#include "manager.h"
#include <logs/logs.h>
#include <misc/camera.h>
#include <misc/paths.h>
#include <fstream>

TA3D::Audio::AudioManager* TA3D::sound_manager;

namespace TA3D
{
	namespace Audio
	{

		const int nbChannels = 16;

		AudioManager::AudioManager()
			: m_SDLMixerRunning(false), m_InBattle(false), pBattleTunesCount(0), pMusic(NULL), bPlayMusic(false), pBasicSound(NULL), pCurrentItemToPlay(-1), pCurrentItemPlaying(-1)
		{
			pMinTicks = 500;

			doStartUpAudio();
			if (isRunning())
				InitInterface();
		}

		void AudioManager::reset()
		{
			doShutdownAudio(true);
			doStartUpAudio();
			if (isRunning())
				InitInterface();
		}

		void AudioManager::setPlayListFileMode(const int idx, bool inBattle, bool disabled)
		{
			if (idx < 0 || idx >= (int)pPlaylist.size())
				return;

			inBattle &= !disabled;
			if (pPlaylist[idx]->battleTune && !inBattle)
				--pBattleTunesCount;
			else
			{
				if (!pPlaylist[idx]->battleTune && inBattle)
					++pBattleTunesCount;
			}

			pPlaylist[idx]->battleTune = inBattle;
			pPlaylist[idx]->disabled = disabled;
		}

		bool AudioManager::getPlayListFiles(String::Vector& out)
		{
			out.resize(pPlaylist.size());
			int indx(0);
			for (String::Vector::iterator i = out.begin(); i != out.end(); ++i, ++indx)
			{
				i->clear();
				String name = pPlaylist[indx]->filename;
				if (pPlaylist[indx]->battleTune)
					*i << "[B] " << name;
				else
				{
					if (pPlaylist[indx]->disabled)
						*i << "[ ] " << name;
					else
						*i << "[*] " << name;
				}
			}
			return !out.empty();
		}

		void AudioManager::updatePlayListFiles()
		{
			pMutex.lock();
			doUpdatePlayListFiles();
			pMutex.unlock();
		}

		void AudioManager::doUpdatePlayListFiles()
		{
			MutexLocker locker(pMutex);

			String::Vector file_list;
			VFS::Instance()->getFilelist("music/*.ogg", file_list);
			VFS::Instance()->getFilelist("music/*.mp3", file_list);
			VFS::Instance()->getFilelist("music/*.mid", file_list);
			VFS::Instance()->getFilelist("music/*.wav", file_list);
			VFS::Instance()->getFilelist("music/*.mod", file_list);

			sort(file_list.begin(), file_list.end());

			for (Playlist::iterator i = pPlaylist.begin(); i != pPlaylist.end(); ++i)
				(*i)->checked = false;
			bool default_deactivation = !pPlaylist.empty();

			String filename;
			for (String::Vector::iterator i = file_list.begin(); i != file_list.end(); ++i) // Add missing files
			{
				*i = Paths::ExtractFileName(*i);
				if (ToLower(*i) == "playlist.txt" || (*i)[0] == '.')
					continue;

				filename = *i;

				Playlist::const_iterator j;
				for (j = pPlaylist.begin(); j != pPlaylist.end(); ++j)
				{
					if ((*j)->filename == filename)
					{
						(*j)->checked = true;
						break;
					}
				}

				if (j == pPlaylist.end()) // It's missing, add it
				{
					PlaylistItem* m_Tune = new PlaylistItem();
					m_Tune->battleTune = false;
					m_Tune->disabled = default_deactivation;
					m_Tune->checked = true;
					m_Tune->filename = filename;
					logs.debug() << LOG_PREFIX_SOUND << "Added to the playlist: `" << filename << '`';
					pPlaylist.push_back(m_Tune);
				}
			}

			int e = 0;
			for (unsigned int i = 0; i + e < pPlaylist.size();) // Do some cleaning
			{
				if (pPlaylist[i + e]->checked)
				{
					pPlaylist[i] = pPlaylist[i + e];
					++i;
				}
				else
				{
					delete pPlaylist[i + e];
					++e;
				}
			}

			pPlaylist.resize(pPlaylist.size() - e); // Remove missing files
			doSavePlaylist();
		}

		void AudioManager::savePlaylist()
		{
			pMutex.lock();
			doSavePlaylist();
			pMutex.unlock();
		}

		void AudioManager::doSavePlaylist()
		{
			String targetPlaylist;
			targetPlaylist << TA3D::Paths::Resources << "music/playlist.txt";
			// Make sure the folder exists
			Paths::MakeDir(Paths::ExtractFilePath(targetPlaylist, true));
			std::ofstream play_list_file(targetPlaylist.c_str(), std::ios::binary);
			if (!play_list_file.is_open())
			{
				LOG_ERROR(LOG_PREFIX_SOUND << "could not open playlist file : '" << targetPlaylist << "'");
				return;
			}

			play_list_file << "#this file has been generated by TA3D_Audio module\n";
			for (Playlist::const_iterator i = pPlaylist.begin(); i != pPlaylist.end(); ++i)
			{
				if ((*i)->battleTune)
					play_list_file << "*";
				else
				{
					if ((*i)->disabled)
						play_list_file << "!";
				}
				play_list_file << (*i)->filename << "\n";
			}
			play_list_file.flush();
			play_list_file.close();

			LOG_INFO(LOG_PREFIX_SOUND << "playlist saved");
		}

		void AudioManager::doLoadPlaylist()
		{
			String filename;
			filename << TA3D::Paths::Resources << "music/playlist.txt";
			std::ifstream file(filename.c_str(), std::ios::binary);

			if (!file.is_open()) // try to create the list if it doesn't exist
			{
				doUpdatePlayListFiles();
				file.open(filename.c_str());
				if (!file.is_open())
				{
					LOG_WARNING(LOG_PREFIX_SOUND << "Impossible to load the playlist : '" << filename << "'");
					return;
				}
			}

			LOG_INFO(LOG_PREFIX_SOUND << "Loading the playlist...");

			String line;
			bool isBattle(false);
			bool isActivated(true);

			pBattleTunesCount = 0;

			while (!file.eof())
			{
				line.clear();
				char c;
				for (file.get(c); c != '\n' && !file.eof(); file.get(c))
				{
					line << c;
					if (file.eof())
						break;
				}

				line.trim(); // strip off spaces, linefeeds, tabs, newlines

				if (line.empty())
					continue;
				if (line[0] == '#' || line[0] == ';')
					continue;

				isActivated = true;

				if (line[0] == '*')
				{
					isBattle = true;
					line.erase(0, 1);
					++pBattleTunesCount;
				}
				else
				{
					if (line[0] == '!')
					{
						isActivated = false;
						line.erase(0, 1);
					}
					else
						isBattle = false;
				}

				PlaylistItem* m_Tune = new PlaylistItem();
				m_Tune->battleTune = isBattle;
				m_Tune->disabled = !isActivated;
				m_Tune->filename = line;

				logs.debug() << LOG_PREFIX_SOUND << "Added to the playlist: `" << line << '`';
				pPlaylist.push_back(m_Tune);
			}

			file.close();
			doUpdatePlayListFiles();
		}

		void AudioManager::doShutdownAudio(const bool purgeLoadedData)
		{
			if (m_SDLMixerRunning) // only execute stop if we are running.
				doStopMusic();

			if (purgeLoadedData)
			{
				purgeSounds();	 // purge sound list.
				doPurgePlaylist(); // purge play list
			}

			if (m_SDLMixerRunning)
			{
				Mix_AllocateChannels(0);

				Mix_CloseAudio();
				DeleteInterface();
				m_SDLMixerRunning = false;
				pMusic = NULL;
			}
		}

		bool AudioManager::doStartUpAudio()
		{
			pMusic = NULL;
			fCounter = 0;
			bPlayMusic = false;

			if (m_SDLMixerRunning)
				return true;

			// 44.1KHz, signed 16bit, system byte order,
			// stereo, 4096 bytes for chunks
			if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
			{
				logs.error() << LOG_PREFIX_SOUND << "Mix_OpenAudio: " << Mix_GetError();
				return false;
			}

			pCurrentItemPlaying = -1;

			Mix_AllocateChannels(nbChannels);

			m_SDLMixerRunning = true;
			doLoadPlaylist();

			setVolume(lp_CONFIG->sound_volume);
			setMusicVolume(lp_CONFIG->music_volume);
			return true;
		}

		AudioManager::~AudioManager()
		{
			doShutdownAudio(true);
		}

		void AudioManager::stopMusic()
		{
			pMutex.lock();
			bPlayMusic = false;
			doStopMusic();
			pMutex.unlock();
		}

		void AudioManager::doStopMusic()
		{
			if (m_SDLMixerRunning && pMusic != NULL)
			{
				Mix_HaltMusic();
				Mix_FreeMusic(pMusic);
				pMusic = NULL;
			}
		}

		void AudioManager::doPurgePlaylist()
		{
			pMutex.lock();
			doStopMusic();
			pCurrentItemToPlay = -1;
			// we don't change this in stop music in case
			// we want to do a play and contine through our list, so
			// we change it here to refelect no index.

			if (!pPlaylist.empty())
			{
				for (Playlist::iterator k_Pos = pPlaylist.begin(); k_Pos != pPlaylist.end(); ++k_Pos)
					delete *k_Pos;
				pPlaylist.clear();
			}
			pMutex.unlock();
		}

		void AudioManager::togglePauseMusic()
		{
			pMutex.lock();
			if (m_SDLMixerRunning && pMusic != NULL)
			{
				if (Mix_PausedMusic())
					Mix_PauseMusic();
				else
					Mix_ResumeMusic();
			}
			pMutex.unlock();
		}

		void AudioManager::pauseMusic()
		{
			pMutex.lock();
			doPauseMusic();
			pMutex.unlock();
		}

		void AudioManager::doPauseMusic()
		{
			if (m_SDLMixerRunning && pMusic != NULL)
				Mix_PauseMusic();
		}

		String AudioManager::doSelectNextMusic()
		{
			if (pPlaylist.empty())
				return nullptr;

			sint16 cIndex = -1;
			sint16 mCount = 0;
			String szResult;
			if (m_InBattle && pBattleTunesCount > 0)
			{
				srand((unsigned)time(NULL));
				cIndex = (sint16)((TA3D_RAND() % pBattleTunesCount) + 1);
				mCount = 1;

				for (Playlist::const_iterator cur = pPlaylist.begin(); cur != pPlaylist.end(); ++cur)
				{
					if ((*cur)->battleTune && mCount >= cIndex) // If we get one that match our needs we take it
					{
						szResult = VFS::Instance()->extractFile(String("music/") << (*cur)->filename);
						break;
					}
					else
					{
						if ((*cur)->battleTune) // Take the last one that can be taken if we try to go too far
							szResult = VFS::Instance()->extractFile(String("music/") << (*cur)->filename);
					}
				}
				return szResult;
			}

			mCount = 0;
			if (pCurrentItemToPlay > (sint32)pPlaylist.size())
				pCurrentItemToPlay = -1;

			bool found = false;

			for (Playlist::const_iterator cur = pPlaylist.begin(); cur != pPlaylist.end(); ++cur)
			{
				++mCount;
				if ((*cur)->battleTune || (*cur)->disabled)
					continue;

				if (pCurrentItemToPlay <= mCount || pCurrentItemToPlay <= 0)
				{
					szResult = VFS::Instance()->extractFile(String("music/") << (*cur)->filename);
					pCurrentItemToPlay = mCount + 1;
					found = true;
					break;
				}
			}
			if (!found && pCurrentItemToPlay != -1)
			{
				pCurrentItemToPlay = -1;
				return doSelectNextMusic();
			}
			return szResult;
		}

		void AudioManager::setMusicMode(const bool battleMode)
		{
			pMutex.lock();
			if (m_InBattle != battleMode)
			{
				m_InBattle = battleMode;
				doPlayMusic();
			}
			pMutex.unlock();
		}

		void AudioManager::doPlayMusic(const String& filename)
		{
			doStopMusic();

			if (!m_SDLMixerRunning || filename.empty())
				return;

			pCurrentItemPlaying = -1;
			for (uint32 i = 0; i < pPlaylist.size(); ++i)
			{
				if (pPlaylist[i]->filename == filename)
				{
					pCurrentItemPlaying = i;
					break;
				}
			}

			if (!Paths::Exists(filename))
			{
				if (!filename.empty())
					logs.error() << LOG_PREFIX_SOUND << "Failed to find file: `" << filename << '`';
				return;
			}

			pMusic = Mix_LoadMUS(filename.c_str());

			if (pMusic == NULL)
			{
				logs.error() << LOG_PREFIX_SOUND << "Failed to open music file : `" << filename << "` (" << Mix_GetError() << ')';
				return;
			}

			if (Mix_PlayMusic(pMusic, 0) == -1)
			{
				logs.error() << LOG_PREFIX_SOUND << "Failed to play music file : `" << filename << "` (" << Mix_GetError() << ')';
				return;
			}

			logs.debug() << LOG_PREFIX_SOUND << "Playing music file " << filename;
			setMusicVolume(lp_CONFIG->music_volume);
		}

		void AudioManager::playMusic()
		{
			pMutex.lock();
			bPlayMusic = true;
			doPlayMusic();
			pMutex.unlock();
		}

		void AudioManager::doPlayMusic()
		{
			if (!m_SDLMixerRunning || !bPlayMusic)
				return;

			bool playing = false;

			if (pMusic != NULL)
			{
				if (Mix_PausedMusic())
				{
					Mix_ResumeMusic();
					return;
				}
				playing = Mix_PlayingMusic();
			}
			if (!playing)
				doPlayMusic(doSelectNextMusic());
		}

		// Begin sound managing routines.
		void AudioManager::setListenerPos(const Vector3D&)
		{
			// disabled because not used pMutex.unlock();
			// if (m_SDLMixerRunning)
			{
#warning TODO: implement 3D stereo
				//            FMOD_VECTOR pos     = { vec.x, vec.y, vec.z };
				//            FMOD_VECTOR vel     = { 0, 0, 0 };
				//            FMOD_VECTOR forward = { 0.0f, 0.0f, 1.0f };
				//            FMOD_VECTOR up      = { 0.0f, 1.0f, 0.0f };
				//
				//            pFMODSystem->set3DListenerAttributes(0, &pos, &vel, &forward, &up);
			}
		}

		void AudioManager::update3DSound()
		{
			MutexLocker locker(pMutex);
			doUpdate3DSound();
		}

		void AudioManager::doUpdate3DSound()
		{
			if (!m_SDLMixerRunning)
			{
				pWorkList.clear();
				return;
			}

#warning TODO: implement 3D stereo

			//        pFMODSystem->update();

			for (std::list<WorkListItem>::iterator i = pWorkList.begin(); i != pWorkList.end(); ++i)
			{
				if (Mix_PlayChannel(-1, i->sound->sampleHandle, 0) == -1)
					continue;

				if (i->sound->is3DSound)
				{
					//                FMOD_VECTOR pos = { i->vec->x, i->vec->y, i->vec->z };
					//                FMOD_VECTOR vel = { 0, 0, 0 };
					//                ch->set3DAttributes(&pos, &vel);
				}
			}

			pWorkList.clear();
			if ((fCounter++) < 100)
				return;

			fCounter = 0;

			if (((pMusic == NULL || !Mix_PlayingMusic()) && pCurrentItemPlaying == -1))
			{
				doPlayMusic();
				return;
			}
		}

		uint32 AudioManager::InterfaceMsg(const uint32 MsgID, const String& msg)
		{
			if (MsgID == TA3D_IM_GUI_MSG) // for GUI messages, test if it's a message for us
			{
				if (msg.empty())
					return INTERFACE_RESULT_HANDLED; // Oups badly written things

				// Get the string associated with the signal
				String message(ToLower(msg));

				if (message == "music play")
				{
					doPlayMusic();
					return INTERFACE_RESULT_HANDLED;
				}
				if (message == "music pause")
				{
					doPauseMusic();
					return INTERFACE_RESULT_HANDLED;
				}
				if (message == "music stop")
				{
					doStopMusic();
					return INTERFACE_RESULT_HANDLED;
				}
			}
			return INTERFACE_RESULT_CONTINUE;
		}

		void AudioManager::playSoundFileNow(const String& filename)
		{
			stopSoundFileNow();

			File* file = VFS::Instance()->readFile(filename);
			if (file)
			{
				pBasicSound = Mix_LoadWAV_RW(SDL_RWFromMem((void*)file->data(), file->size()), 1);
				delete file;
				if (pBasicSound == NULL)
				{
					logs.error() << LOG_PREFIX_SOUND << "error loading file `" << filename << "` (" << Mix_GetError() << ')';
					return;
				}
				Mix_PlayChannel(-1, pBasicSound, 0);
			}
		}

		void AudioManager::stopSoundFileNow()
		{
			MutexLocker locker(pMutex);
			if (pBasicSound)
			{
				for (int i = 0; i < nbChannels; ++i)
				{
					if (Mix_GetChunk(i) == pBasicSound)
						Mix_HaltChannel(i);
				}
				Mix_FreeChunk(pBasicSound);
			}
			pBasicSound = NULL;
		}

		bool AudioManager::loadSound(const String& filename, const bool LoadAs3D, const float MinDistance, const float MaxDistance)
		{
			MutexLocker locker(pMutex);
			return doLoadSound(filename, LoadAs3D, MinDistance, MaxDistance);
		}

		bool AudioManager::doLoadSound(String filename, const bool LoadAs3D, const float /*MinDistance*/, const float /*MaxDistance*/)
		{
			if (filename.empty()) // We can't load a file with an empty name
				return false;
			filename.toLower();

			// if it has a .wav extension then remove it.
			if (filename.endsWith(".wav") || filename.endsWith(".ogg") || filename.endsWith(".mp3"))
				filename = Substr(filename, 0, filename.length() - 4);

			// if its already loaded return true.
			if (pSoundList.count(filename) != 0)
			{
				return true;
			}

			// pull the data from hpi.
			String theSound;
			theSound << "sounds\\" << filename;
			if (VFS::Instance()->fileExists(String(theSound) << ".wav"))
				theSound << ".wav";
			else
			{
				if (VFS::Instance()->fileExists(String(theSound) << ".ogg"))
					theSound << ".ogg";
				else
				{
					if (VFS::Instance()->fileExists(String(theSound) << ".mp3"))
						theSound << ".mp3";
				}
			}
			File* file = VFS::Instance()->readFile(theSound);
			if (!file) // if no data, log a message and return false.
			{
				// logs.debug() <<  LOG_PREFIX_SOUND << "AudioManager: LoadSound(" << filename << "), no such sound found in HPI.");
				return false;
			}

			SoundItemList* it = new SoundItemList(LoadAs3D);
			LOG_ASSERT(NULL != it);

			// Now get SDL_mixer to load the sample
			it->sampleHandle = Mix_LoadWAV_RW(SDL_RWFromMem((void*)file->data(), file->size()), 1);
			delete file; // we no longer need this.

			if (it->sampleHandle == NULL) // ahh crap SDL_mixer couln't load it.
			{
				delete it; // delete the sound.
				// log a message and return false;
				if (m_SDLMixerRunning)
					logs.debug() << LOG_PREFIX_SOUND << "AudioManager: LoadSound(" << filename << "), Failed to construct sample.";
				return false;
			}

// if its a 3d Sound we need to set min/max distance.
#warning TODO: implement 3D stereo
			//        if (it->is3DSound)
			//            it->sampleHandle->set3DMinMaxDistance(MinDistance, MaxDistance);

			// add the sound to our soundlist hash table, and return true.
			pSoundList[filename] = it;
			return true;
		}

		void AudioManager::loadTDFSounds(const bool allSounds)
		{
			MutexLocker locker(pMutex);
			// Which file to load ?
			String filename(ta3dSideData.gamedata_dir);
			filename += (allSounds) ? "allsound.tdf" : "sound.tdf";

			logs.debug() << LOG_PREFIX_SOUND << "Reading `" << filename << "`...";
			// Load the TDF file
			if (pTable.loadFromFile(filename))
			{
				LOG_INFO(LOG_PREFIX_SOUND << "Loading sounds from " << filename);
				// Load each sound file
				pTable.forEach(LoadAllTDFSound(*this));
				logs.debug() << LOG_PREFIX_SOUND << "Reading: Done.";
			}
			else
				logs.debug() << LOG_PREFIX_SOUND << "Reading: Aborted.";
		}

		void AudioManager::purgeSounds()
		{
			pMutex.lock();

			Mix_HaltChannel(-1);

			for (TA3D::HashMap<SoundItemList*>::Dense::iterator it = pSoundList.begin(); it != pSoundList.end(); ++it)
				delete *it;
			pSoundList.clear();
			pTable.clear();
			pWorkList.clear();
			pMutex.unlock();
		}

		// Play sound directly from our sound pool
		void AudioManager::playSound(const String& filename, const Vector3D* vec)
		{
			if (filename.empty())
				return;

			MutexLocker locker(pMutex);
			if (vec && Camera::inGame && ((Vector3D)(*vec - Camera::inGame->rpos)).sq() > 360000.0f) // If the source is too far, does not even think about playing it!
				return;
			if (!m_SDLMixerRunning)
				return;

			String szWav(filename); // copy string to szWav so we can work with it.
			// if it has a .wav extension then remove it.
			String::size_type i = szWav.toLower().find(".wav");
			if (i != String::npos)
				szWav.truncate(szWav.length() - 4);
			else
			{
				// if it has a .ogg extension then remove it.
				i = szWav.toLower().find(".ogg");
				if (i != String::npos)
					szWav.truncate(szWav.length() - 4);
				else
				{
					// if it has a .mp3 extension then remove it.
					i = szWav.toLower().find(".mp3");
					if (i != String::npos)
						szWav.truncate(szWav.length() - 4);
				}
			}

			SoundItemList* sound = pSoundList[szWav];
			if (!sound)
			{
				logs.error() << LOG_PREFIX_SOUND << "`" << filename << "` not found, aborting";
				return;
			}

			if (MILLISECONDS_SINCE_INIT - sound->lastTimePlayed < pMinTicks)
				return; // Make sure it doesn't play too often, so it doesn't play too loud!

			sound->lastTimePlayed = MILLISECONDS_SINCE_INIT;

			if (!sound->sampleHandle || (sound->is3DSound && !vec))
			{
				if (!sound->sampleHandle)
					logs.error() << LOG_PREFIX_SOUND << "`" << filename << "` not played the good way";
				else
					logs.error() << LOG_PREFIX_SOUND << "`" << filename << "` sound->sampleHandle is false";
				return;
			}

			pWorkList.push_back(WorkListItem(sound, vec));
		}

		void AudioManager::playTDFSoundNow(const String& Key, const Vector3D* vec)
		{
			pMutex.lock();
			String szWav = pTable.pullAsString(ToLower(Key)); // copy string to szWav so we can work with it.
			String::size_type i = szWav.toLower().find(".wav");
			if (i != String::npos)
				szWav.truncate(szWav.length() - 4);
			else
			{
				i = szWav.toLower().find(".ogg");
				if (i != String::npos)
					szWav.truncate(szWav.length() - 4);
				else
				{
					i = szWav.toLower().find(".mp3");
					if (i != String::npos)
						szWav.truncate(szWav.length() - 4);
				}
			}

			SoundItemList* it = pSoundList[szWav];
			if (it)
			{
				it->lastTimePlayed = MILLISECONDS_SINCE_INIT - 1000 - pMinTicks; // Make sure it'll be played
				doPlayTDFSound(Key, vec);
			}
			doUpdate3DSound();
			pMutex.unlock();
		}

		void AudioManager::playTDFSound(const String& key, const Vector3D* vec)
		{
			pMutex.lock();
			doPlayTDFSound(key, vec);
			pMutex.unlock();
		}

		void AudioManager::doPlayTDFSound(String key, const Vector3D* vec)
		{
			if (!key.empty())
			{
				if (!pTable.exists(key.toLower()))
				{
					// output a report to the console but only once
					logs.warning() << LOG_PREFIX_SOUND << "Can't find key `" << key << '`';
					pTable.insertOrUpdate(key, "");
					return;
				}
				const String& wav = pTable.pullAsString(key);
				if (!wav.empty())
					playSound(wav, vec);
			}
		}

		void AudioManager::doPlayTDFSound(const String& keyA, const String& keyB, const Vector3D* vec)
		{
			if (!keyA.empty() && !keyB.empty())
			{
				String key;
				key << keyA << '.' << keyB;
				doPlayTDFSound(key, vec);
			}
		}

		void AudioManager::playTDFSound(const String& keyA, const String& keyB, const Vector3D* vec)
		{
			if (!keyA.empty() && !keyB.empty())
			{
				String key;
				key << keyA << '.' << keyB;
				playTDFSound(key, vec);
			}
		}

		AudioManager::SoundItemList::~SoundItemList()
		{
			if (sampleHandle)
			{
				for (int i = 0; i < nbChannels; i++)
					if (Mix_GetChunk(i) == sampleHandle)
						Mix_HaltChannel(i);
				Mix_FreeChunk(sampleHandle);
			}
			sampleHandle = NULL;
		}

		void AudioManager::setVolume(int volume)
		{
			if (!isRunning())
				return;
			Mix_Volume(-1, volume);
		}

		void AudioManager::setMusicVolume(int volume)
		{
			if (!isRunning())
				return;
			Mix_VolumeMusic(volume);
		}
	} // namespace Audio
} // namespace TA3D
