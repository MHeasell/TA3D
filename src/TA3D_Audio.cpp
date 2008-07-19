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

#include "stdafx.h"
#include "TA3D_NameSpace.h"		//"TA3D_Audio.h" is in our namespace.
#include "ta3dbase.h"			// just to use the global camera object
#include "ingame/sidedata.h"
#include <fstream>
#include "misc/math.h"

using namespace TA3D::Interfaces;

# define TA3D_LOG_SECTION_AUDIO_PREFIX "[Audio] "

TA3D::Interfaces::cAudio *TA3D::VARS::sound_manager;


namespace TA3D
{
namespace Interfaces
{



    cAudio::cAudio( const float DistanceFactor, const float DopplerFactor, const float RolloffFactor )
        :m_FMODRunning( false ), m_InBattle(false), m_BattleTunes(0),
        m_lpFMODMusicsound( NULL ), m_lpFMODMusicchannel( NULL ),
        m_curPlayIndex(-1)
    {
        m_min_ticks = 500;

        basic_sound = NULL;
        basic_channel = NULL;
        startUpAudio();
        m_SoundList = new TA3D::UTILS::clpHashTable< m_SoundListItem * >;
        InitInterface();

        if (!m_FMODRunning)
            return;

        # ifdef TA3D_PLATFORM_MINGW
        FMOD_System_Set3DSettings(m_lpFMODSystem, DopplerFactor, DistanceFactor, RolloffFactor);
        # else
        m_lpFMODSystem->set3DSettings(DopplerFactor, DistanceFactor, RolloffFactor);
        # endif
    }




    void cAudio::setPlayListFileMode(const int idx, bool Battle, bool Deactivated)
    {
        if (idx < 0 || idx >= (int)m_Playlist.size())
            return;

        Battle &= !Deactivated;

        if (m_Playlist[idx]->m_BattleTune && !Battle)
            --m_BattleTunes;
        else
            if (!m_Playlist[idx]->m_BattleTune && Battle)
                ++m_BattleTunes;

        m_Playlist[idx]->m_BattleTune = Battle;
        m_Playlist[idx]->m_Deactivated = Deactivated;
    }



    bool cAudio::getPlayListFiles(String::Vector& out)
    {
        out.resize(m_Playlist.size());
        int indx(0);
        for (String::Vector::iterator i = out.begin(); i != out.end(); ++i, ++indx)
        {
            i->clear();
            if (m_Playlist[indx]->m_BattleTune)
                *i << "[B] " << m_Playlist[indx]->m_Filename;
            else
            {
                if (m_Playlist[indx]->m_Deactivated)
                    *i << "[ ] " << m_Playlist[indx]->m_Filename;
                else
                    *i << "[*] " << m_Playlist[indx]->m_Filename;
            }
        }
        return !out.empty();
    }




    void cAudio::updatePlayListFiles()
    {
        struct al_ffblk info;
        String search;
        search << GetClientPath() << "music/";

        for (plItor i = m_Playlist.begin(); i != m_Playlist.end(); ++i)
            (*i)->m_checked = false;
        bool default_deactivation = !m_Playlist.empty();

        if (al_findfirst((search + "*").c_str(), &info, FA_ALL) == 0) // Add missing files
        {
            do
            {
                if (String::ToLower(info.name) == "playlist.txt" || info.name[0] == '.')
                    continue;

                String filename;
                filename << search << info.name;

                plItor i;
                for (i = m_Playlist.begin(); i != m_Playlist.end(); ++i)
                {
                    if ((*i)->m_Filename == filename)
                    {
                        (*i)->m_checked = true;
                        break;
                    }
                }

                if (i == m_Playlist.end()) // It's missing, add it
                {
                    m_PlayListItem *m_Tune = new m_PlayListItem();
                    m_Tune->m_BattleTune = false;
                    m_Tune->m_Deactivated = default_deactivation;
                    m_Tune->m_checked = true;
                    m_Tune->m_Filename = filename;
                    Console->AddEntry("playlist adding : %s", (char*)filename.c_str());
                    m_Playlist.push_back(m_Tune);
                }

            } while (!al_findnext(&info));
            al_findclose(&info);
        }

        int e = 0;
        for (unsigned int i = 0 ; i + e < m_Playlist.size() ; ) // Do some cleaning
        {
            if (m_Playlist[i + e]->m_checked)
            {
                m_Playlist[i] = m_Playlist[i + e];
                ++i;
            }
            else
            {
                delete m_Playlist[i + e];
                ++e;
            }
        }

        m_Playlist.resize( m_Playlist.size() - e );	// Remove missing files
        savePlayList();	// Save our work :)
    }



    void cAudio::savePlayList()
    {
        String targetPlaylist;
        targetPlaylist << GetClientPath() << "music/playlist.txt";
        std::ofstream play_list_file(targetPlaylist.c_str(), std::ios::out | std::ios::trunc);
        if (!play_list_file.is_open())
            return;

        play_list_file << "#this file has been generated by TA3D_Audio module\n";
        for( plItor i = m_Playlist.begin(); i != m_Playlist.end(); ++i)
        {
            if ((*i)->m_BattleTune)
                play_list_file << "*" << (*i)->m_Filename << "\n";
            else 
            {
                if ((*i)->m_Deactivated)
                    play_list_file << "!" << (*i)->m_Filename << "\n";
                else
                    play_list_file << (*i)->m_Filename << "\n";
            }
        }
        play_list_file.flush();
        play_list_file.close();
    }




    void cAudio::loadPlayList()
    {
        String FileName;
        FileName << GetClientPath() << "music/playlist.txt";
        std::ifstream file( FileName.c_str(), std::ios::in);

        Console->AddEntry("opening playlist");

        if (!file.is_open()) // try to create the list if it doesn't exist
        {
            updatePlayListFiles();
            file.open(FileName.c_str(), std::ios::in);
            if (!file.is_open())
                return;
        }

        Console->AddEntry("loading playlist");

        String line;
        bool isBattle = false;
        bool isActivated = true;

        m_BattleTunes = 0;

        while( !file.eof() )
        {
            std::getline( file, line, '\n');

            line = String::Trim(line); // strip off spaces, linefeeds, tabs, newlines

            if (!line.length())
                continue;
            if (line[0] == '#' || line[0] == ';')
                continue;

            isActivated = true;

            if (line[0] == '*' )
            {
                isBattle=true;
                line = line.erase( 0,1 );
                m_BattleTunes++;
            }
            else
            {
                if (line[0] == '!' )
                    isActivated = false;
                else
                    isBattle = false;
            }

            m_PlayListItem *m_Tune = new m_PlayListItem();
            m_Tune->m_BattleTune = isBattle;
            m_Tune->m_Deactivated = !isActivated;
            m_Tune->m_Filename = line;

            Console->AddEntry("playlist adding : %s", (char*)line.c_str());
            m_Playlist.push_back(m_Tune);
        }

        file.close();
        updatePlayListFiles();
        if (m_Playlist.size() > 0)
            playMusic();
    }





    void cAudio::shutdownAudio(bool PurgeLoadedData)
    {
        if (m_FMODRunning) // only execute stop if we are running.
            stopMusic();

        if (PurgeLoadedData)
        {
            purgeSounds(); // purge sound list.
            purgePlayList(); // purge play list
        }

        if (m_FMODRunning)
        {
#ifdef TA3D_PLATFORM_MINGW
            if (basic_sound)
                FMOD_Sound_Release(basic_sound);
#else
            if (basic_sound)
                basic_sound->release();
#endif
            basic_sound = NULL;
            basic_channel = NULL;
#ifdef TA3D_PLATFORM_MINGW
            FMOD_System_Close(m_lpFMODSystem);
            FMOD_System_Release(m_lpFMODSystem);
#else
            m_lpFMODSystem->close(); // Commented because crashes with some FMOD versions, and since we're going to end the program ...
            m_lpFMODSystem->release();
#endif
            DeleteInterface();
            m_FMODRunning = false;
        }
    }




    bool cAudio::startUpAudio()
    {
        uint32 FMODVersion;

        m_lpFMODMusicsound = NULL;
        m_lpFMODMusicchannel = NULL;
        fCounter = 0;

        if (m_FMODRunning)
            return true;

#ifdef TA3D_PLATFORM_MINGW
        if (FMOD_System_Create(&m_lpFMODSystem) != FMOD_OK)
        {
            Console->AddEntry( "FMOD: failed to System_Create, sound disabled");
            return false;
        }

        if (FMOD_System_GetVersion( m_lpFMODSystem, &FMODVersion ) != FMOD_OK)
        {
            Console->AddEntry( "FMOD: Invalid Version of FMOD, sound disabled" );
            return false;
        }

        Console->stdout_on();
        Console->AddEntry("FMOD version: %x.%x.%x", ((FMODVersion & 0xFFFF0000) >> 16), ((FMODVersion & 0xFF00) >> 8), FMODVersion & 0xFF );
        Console->stdout_off();

        if (FMOD_System_SetStreamBufferSize( m_lpFMODSystem, 32768, FMOD_TIMEUNIT_RAWBYTES ) != FMOD_OK)
        {
            Console->AddEntry( "FMOD: Failed to set Stream Buffer Size, sound disabled" );
            return false;
        }

#ifndef TA3D_NO_SOUND
        // 32 channels, normal init, with 3d right handed.
        if (FMOD_System_Init( m_lpFMODSystem, 32, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0 ) != FMOD_OK)
        {
            Console->AddEntry("FMOD: Failed to init FMOD, sound disabled" );
            return false;
        }

        m_FMODRunning = true;

        loadPlayList();
#endif
#else
        if (FMOD::System_Create( &m_lpFMODSystem ) != FMOD_OK)
        {
            Console->AddEntry("FMOD: failed to System_Create, sound disabled");
            return false;
        }
        if (m_lpFMODSystem->getVersion( &FMODVersion ) != FMOD_OK)
        {
            Console->AddEntry("FMOD: Invalid Version of FMOD, sound disabled");
            return false;
        }
        Console->stdout_on();
        Console->AddEntry("FMOD version: %x.%x.%x", ((FMODVersion & 0xFFFF0000) >> 16), ((FMODVersion & 0xFF00) >> 8), FMODVersion & 0xFF);
        Console->stdout_off();

        if (m_lpFMODSystem->setStreamBufferSize( 32768, FMOD_TIMEUNIT_RAWBYTES ) != FMOD_OK)
        {
            Console->AddEntry( "FMOD: Failed to set Stream Buffer Size, sound disabled" );
            return false;
        }

#ifndef TA3D_NO_SOUND
#ifdef TA3D_PLATFORM_LINUX
        if (m_lpFMODSystem->setOutput(FMOD_OUTPUTTYPE_ALSA) != FMOD_OK)
        {
            Console->AddEntry( "FMOD: Failed to init FMOD, sound disabled");
            return false;
        }
#endif
        // 32 channels, normal init, with 3d right handed.
        if (m_lpFMODSystem->init(32, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0) != FMOD_OK)
        {
            Console->AddEntry("FMOD: Failed to init FMOD, sound disabled");
            return false;
        }
        m_FMODRunning = true;
        loadPlayList();
#endif
#endif
        return true;
    }



    cAudio::~cAudio()
    {
        shutdownAudio(true);
        delete m_SoundList;
    }



    void cAudio::stopMusic()
    {
        if (!m_FMODRunning)
            return;

        if (m_lpFMODMusicsound != NULL)
        {
            pMutex.lock();
#ifdef TA3D_PLATFORM_MINGW
            FMOD_Channel_Stop(m_lpFMODMusicchannel);
            FMOD_Sound_Release(m_lpFMODMusicsound);
#else
            m_lpFMODMusicchannel->stop();
            m_lpFMODMusicsound->release();
#endif
            m_lpFMODMusicsound = NULL;
            m_lpFMODMusicchannel = NULL;
            pMutex.unlock();
        }
    }




    void cAudio::purgePlayList()
    {
        stopMusic();

        m_curPlayIndex = -1;	// we don't change this in stop music in case
        // we want to do a play and contine through our list, so
        // we change it here to refelect no index.

        if (m_Playlist.empty()) // nothing in our list.
            return;

        pMutex.lock();
        // walk through vector and delete all the items.
        for (plItor k_Pos = m_Playlist.begin(); k_Pos != m_Playlist.end(); ++k_Pos)
            delete *k_Pos ;

        m_Playlist.clear();   // now purge the vector.
        pMutex.unlock();
    }

    
    
    
    void cAudio::togglePauseMusic()
    {
        if (m_FMODRunning || m_lpFMODMusicchannel == NULL)
            return;
#ifdef TA3D_PLATFORM_MINGW
        FMOD_BOOL paused;
        FMOD_Channel_GetPaused(m_lpFMODMusicchannel, &paused);
        FMOD_Channel_SetPaused(m_lpFMODMusicchannel, !paused);
#else
        bool paused;
        m_lpFMODMusicchannel->getPaused(&paused);
        m_lpFMODMusicchannel->setPaused(!paused);
#endif
    }



    void cAudio::pauseMusic()
    {
        if (m_FMODRunning)
            return;

        if (m_lpFMODMusicchannel == NULL)
            return;

#ifdef TA3D_PLATFORM_MINGW
        FMOD_Channel_SetPaused(m_lpFMODMusicchannel, true);
#else
        m_lpFMODMusicchannel->setPaused(true);
#endif
    }




    const String cAudio::selectNextMusic()
    {
        plItor cur;
        sint16 cIndex = -1;
        sint16 mCount = 0;
        String szResult;

        if (m_Playlist.empty())
            return szResult;

        if (m_InBattle && m_BattleTunes > 0)
        {
            srand((unsigned)time(NULL));
            cIndex =  (sint16)(TA3D_RAND() % m_BattleTunes ) + 1;
            mCount = 1;

            for( cur = m_Playlist.begin(); cur != m_Playlist.end(); ++cur)
            {
                if ((*cur)->m_BattleTune && mCount >= cIndex)		// If we get one that match our needs we take it
                {
                    szResult = (*cur)->m_Filename;
                    break;
                }
                else
                {
                    if ((*cur)->m_BattleTune) // Take the last one that can be taken if we try to go too far
                        szResult = (*cur)->m_Filename;
                }
            }
            return szResult;
        }

        mCount = 0;
        if (m_curPlayIndex > (sint32)m_Playlist.size())
            m_curPlayIndex = -1;

        bool found = false;

        for (cur = m_Playlist.begin(); cur != m_Playlist.end(); ++cur)
        {
            ++mCount;

            if ((*cur)->m_BattleTune || (*cur)->m_Deactivated)
                continue;

            if (m_curPlayIndex <= mCount || m_curPlayIndex <= 0)
            {
                szResult = (*cur)->m_Filename;
                m_curPlayIndex = mCount + 1;
                found = true;
                break;
            }
        }
        if (!found && m_curPlayIndex != -1)
        {
            m_curPlayIndex = -1;
            return selectNextMusic();
        }
        return szResult;
    }




    void cAudio::setMusicMode(const bool battleMode)
    {
        if (m_InBattle != battleMode)
        {
            m_InBattle = battleMode;
            playMusic();
        }
    }




    void cAudio::playMusic(const String& FileName)
    {
        stopMusic();

        if (!m_FMODRunning)
            return;

        if (!file_exists( FileName.c_str() ,FA_RDONLY | FA_ARCH,NULL ) )
        {
            if (!FileName.empty() )
                Console->AddEntry( "FMOD:Failed to find file: %s for play.", FileName.c_str() );
            return;
        }

#ifdef TA3D_PLATFORM_MINGW

        if (FMOD_System_CreateStream( m_lpFMODSystem, FileName.c_str(),
                                      FMOD_HARDWARE | FMOD_LOOP_OFF | FMOD_2D | FMOD_IGNORETAGS, 0,
                                      &m_lpFMODMusicsound ) != FMOD_OK )
        {
            Console->AddEntry( "FMOD: Failed to create stream. (%s)", FileName.c_str() );
            return;
        }

        if (FMOD_System_playSound( m_lpFMODSystem, FMOD_CHANNEL_FREE, m_lpFMODMusicsound,
                                   false, &m_lpFMODMusicchannel) != FMOD_OK )
        {
            Console->AddEntry( "FMOD: Failed to playSound/stream.");
            return;
        }

        Console->AddEntry( "FMOD: playing music file %s", FileName.c_str());
        FMOD_Channel_SetVolume( m_lpFMODMusicchannel, 1.0f);

#else

        if (m_lpFMODSystem->createStream( FileName.c_str(),
                                          FMOD_HARDWARE | FMOD_LOOP_OFF | FMOD_2D | FMOD_IGNORETAGS, 0,
                                          &m_lpFMODMusicsound ) != FMOD_OK )
        {
            Console->AddEntry("FMOD: Failed to create stream. (%s)", FileName.c_str());
            return;
        }

        if (m_lpFMODSystem->playSound(FMOD_CHANNEL_FREE, m_lpFMODMusicsound, false, &m_lpFMODMusicchannel) != FMOD_OK)
        {
            Console->AddEntry("FMOD: Failed to playSound/stream.");
            return;
        }

        Console->AddEntry("FMOD: playing music file %s", FileName.c_str());
        m_lpFMODMusicchannel->setVolume(1.0f);
#endif
    }




    void cAudio::playMusic()
    {
        if (!m_FMODRunning)
            return;

        if (m_lpFMODMusicchannel != NULL)
        {
#ifdef TA3D_PLATFORM_MINGW
            FMOD_BOOL paused;
            FMOD_Channel_GetPaused(m_lpFMODMusicchannel, &paused);

            if (paused == true)
            {
                FMOD_Channel_SetPaused(m_lpFMODMusicchannel, false);
                return;
            }
#else
            bool paused;

            m_lpFMODMusicchannel->getPaused(&paused);

            if (paused == true)
            {
                m_lpFMODMusicchannel->setPaused(false);
                return;
            }
#endif
        }
        playMusic(selectNextMusic());
    }



    // Begin sound managing routines.
    void cAudio::setListenerPos(const VECTOR3D& vec)
    {
        if (!m_FMODRunning)
            return;

        FMOD_VECTOR pos = { vec.x, vec.y, vec.z };
        FMOD_VECTOR vel = { 0,0,0 };
        FMOD_VECTOR forward        = { 0.0f, 0.0f, 1.0f };
        FMOD_VECTOR up             = { 0.0f, 1.0f, 0.0f };

#ifdef TA3D_PLATFORM_MINGW
        FMOD_System_Set3DListenerAttributes( m_lpFMODSystem, 0, &pos, &vel, &forward, &up );
#else
        m_lpFMODSystem->set3DListenerAttributes( 0, &pos, &vel, &forward, &up );
#endif
    }

    void cAudio::update3DSound()
    {
        if (!m_FMODRunning)
        {
            pMutex.lock();
            WorkList.clear();
            pMutex.unlock();
            return;
        }

        pMutex.lock();

#ifdef TA3D_PLATFORM_MINGW

        FMOD_System_Update(m_lpFMODSystem);

        for (std::list< m_WorkListItem >::iterator i = WorkList.begin(); i != WorkList.end(); ++i)
        {
            FMOD_CHANNEL *ch;
            if (FMOD_System_playSound(m_lpFMODSystem, FMOD_CHANNEL_FREE, i->m_Sound->m_SampleHandle, true, &ch) != FMOD_OK)
            {
                continue;
            }
            if (i->m_Sound->m_3DSound)
            {
                FMOD_VECTOR pos = { i->vec->x, i->vec->y, i->vec->z };
                FMOD_VECTOR vel = { 0,0,0 };

                FMOD_Channel_Set3DAttributes( ch, &pos, &vel );
            }
            FMOD_Channel_SetPaused(ch, false);
        }

#else

        m_lpFMODSystem->update();

        for (std::list< m_WorkListItem >::iterator i = WorkList.begin() ; i != WorkList.end() ; ++i)
        {
            FMOD::Channel *ch;
            if (m_lpFMODSystem->playSound( FMOD_CHANNEL_FREE,
                                           i->m_Sound->m_SampleHandle, true, &ch ) != FMOD_OK )
                continue;

            if (i->m_Sound->m_3DSound )
            {
                FMOD_VECTOR pos = { i->vec->x, i->vec->y, i->vec->z };
                FMOD_VECTOR vel = { 0,0,0 };

                ch->set3DAttributes( &pos, &vel );
            }

            ch->setPaused(false);
        }

#endif

        WorkList.clear();

        pMutex.unlock();

        if ((fCounter++) < 100)
            return;

        fCounter = 0;

        if (m_lpFMODMusicchannel == NULL)
        {
            playMusic();
            return;
        }

        pMutex.lock();

#ifdef TA3D_PLATFORM_MINGW
        FMOD_BOOL playing;
        FMOD_Channel_IsPlaying(m_lpFMODMusicchannel, &playing);
#else
        bool playing;
        m_lpFMODMusicchannel->isPlaying(&playing);
#endif
        if (!playing)
            playMusic();

        pMutex.unlock();
    }



    
    uint32 cAudio::InterfaceMsg(const lpcImsg msg)
    {
        if (msg->MsgID == TA3D_IM_GUI_MSG)	// for GUI messages, test if it's a message for us
        {
            if (msg->lpParm1 == NULL)
                return INTERFACE_RESULT_HANDLED; // Oups badly written things
            
            // Get the string associated with the signal
            String message((char*)msg->lpParm1);
            message.toLower();

            if (message == "music play")
            {
                playMusic();
                return INTERFACE_RESULT_HANDLED;
            }
            if (message == "music pause")
            {
                pauseMusic();
                return INTERFACE_RESULT_HANDLED;
            }
            if (message == "music stop")
            {
                stopMusic();
                return INTERFACE_RESULT_HANDLED;
            }
        }
        return INTERFACE_RESULT_CONTINUE;
    }




    void cAudio::playSoundFileNow(const String& Filename)
    {
#ifdef TA3D_PLATFORM_MINGW
        if (basic_sound)
            FMOD_Sound_Release(basic_sound);
#else
        if (basic_sound)
            basic_sound->release();
#endif
        basic_sound = NULL;
        basic_channel = NULL;
        uint32 sound_file_size = 0;
        byte *data = HPIManager->PullFromHPI(Filename, &sound_file_size);

        if (data)
        {
            FMOD_CREATESOUNDEXINFO exinfo;

            memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
            exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
            exinfo.length = sound_file_size;

#ifdef TA3D_PLATFORM_MINGW
            FMOD_System_CreateSound( m_lpFMODSystem, (const char *)data, FMOD_SOFTWARE | FMOD_OPENMEMORY, &exinfo, &basic_sound );
            FMOD_Sound_SetMode( basic_sound, FMOD_LOOP_OFF );
            FMOD_System_playSound( m_lpFMODSystem, FMOD_CHANNEL_FREE, basic_sound, 0, &basic_channel);
#else
            m_lpFMODSystem->createSound( (const char *)data, FMOD_SOFTWARE | FMOD_OPENMEMORY, &exinfo, &basic_sound);
            basic_sound->setMode(FMOD_LOOP_OFF);
            m_lpFMODSystem->playSound( FMOD_CHANNEL_FREE, basic_sound, 0, &basic_channel);
#endif
            delete[] data;
        }
    }

    void cAudio::stopSoundFileNow()				// Stop playing
    {
        # ifdef TA3D_PLATFORM_MINGW
        if (basic_sound)
            FMOD_Sound_Release(basic_sound);
        # else
        if (basic_sound)
            basic_sound->release();
        # endif

        basic_sound = NULL;
        basic_channel = NULL;
    }



    bool cAudio::loadSound(const String& Filename, const bool LoadAs3D, const float MinDistance, const float MaxDistance)
    {
        String szWav(Filename);
        szWav.toLower();

        I_Msg( TA3D::TA3D_IM_DEBUG_MSG, (char*)format("loading sound file %s\n",(char *)szWav.c_str()).c_str(), NULL, NULL );

        // if it has a .wav extension then remove it.
        int i = (int)szWav.find("wav");   
        if (i != -1)
            szWav.resize(szWav.length() - 4);

        // if its already loaded return true.
        if (m_SoundList->exists(szWav))
        {
            //		I_Msg( TA3D::TA3D_IM_DEBUG_MSG, (char*)format("sound file %s is already loaded\n",(char *)szWav.c_str()).c_str(), NULL, NULL );
            return true;
        }

        // pull the data from hpi.
        String theSound;
        uint32 Length;
        theSound << "sounds\\" << szWav << ".wav";
        byte* data = HPIManager->PullFromHPI(theSound, &Length);
        if (!data) // if no data, log a message and return false.
        {
            szWav = format("FMOD: LoadSound(%s), no such sound found in HPI.\n", szWav.c_str());
            I_Msg( TA3D::TA3D_IM_DEBUG_MSG, (void *)szWav.c_str(), NULL, NULL);
            return false;
        }

        m_SoundListItem *m_Sound = new m_SoundListItem;
        m_Sound->m_3DSound = LoadAs3D;

        FMOD_CREATESOUNDEXINFO exinfo;
        memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = Length;

#ifdef TA3D_PLATFORM_MINGW

        // Now get fmod to load the sample
        FMOD_RESULT FMODResult = FMOD_System_CreateSound( m_lpFMODSystem, (const char*)data,
                                                          FMOD_HARDWARE | FMOD_OPENMEMORY | ( (LoadAs3D) ? FMOD_3D : FMOD_2D ),
                                                          &exinfo,
                                                          &m_Sound->m_SampleHandle);
        delete[] data;

        if (FMODResult != FMOD_OK) // ahh crap fmod couln't load it.
        {
            delete m_Sound;  // delete the sound.
            m_Sound = NULL;

            // log a message and return false;
            if (m_FMODRunning)
            {
                szWav = format("FMOD: LoadSound(%s), Failed to construct sample.\n", szWav.c_str());
                I_Msg( TA3D::TA3D_IM_DEBUG_MSG, (void *)szWav.c_str(), NULL, NULL);
            }
            return false;
        }

        // if its a 3d Sound we need to set min/max distance.
        if (m_Sound->m_3DSound)
            FMOD_Sound_Set3DMinMaxDistance(m_Sound->m_SampleHandle, MinDistance, MaxDistance);

#else

        // Now get fmod to load the sample
        FMOD_RESULT FMODResult = m_lpFMODSystem->createSound( (const char *)data,
                                                              FMOD_HARDWARE | FMOD_OPENMEMORY | ( (LoadAs3D) ? FMOD_3D : FMOD_2D ),
                                                              &exinfo,
                                                              &m_Sound->m_SampleHandle);
        free(data); // we no longer need this.

        if (FMODResult != FMOD_OK) // ahh crap fmod couln't load it.
        {
            delete m_Sound;  // delete the sound.
            m_Sound = NULL;
            // log a message and return false;
            if (m_FMODRunning)
            {
                szWav = format("FMOD: LoadSound(%s), Failed to construct sample.\n", szWav.c_str());
                I_Msg(TA3D::TA3D_IM_DEBUG_MSG, (void *)szWav.c_str(), NULL, NULL);
            }
            return false;
        }

        // if its a 3d Sound we need to set min/max distance.
        if (m_Sound->m_3DSound)
            m_Sound->m_SampleHandle->set3DMinMaxDistance(MinDistance, MaxDistance);

#endif

        // add the sound to our soundlist hash table, and return true.
        m_SoundList->insertOrUpdate(szWav, m_Sound);
        return true;
    }

    
    class LoadAllTDFSound
    {
    public:
        LoadAllTDFSound(cAudio& a) : pAudio(a) {}
        bool operator () (const String& key, const String& value)
        {
            pAudio.loadSound(value, false);
            return true;
        }

    private:
        cAudio& pAudio;
    };


    void cAudio::loadTDFSounds(const bool allSounds)
    {
        pMutex.lock();
        String FileName(ta3dSideData.gamedata_dir);
        FileName += (allSounds) ? "allsound.tdf" : "sound.tdf";

        Console->AddEntry("Reading %s", FileName.c_str());
        pTable.load(FileName);
        Console->AddEntry("Loading sounds from %s", FileName.c_str());

        pTable.forEach(LoadAllTDFSound(*this));
        pMutex.unlock();
    }


    void cAudio::purgeSounds()
    {
        pMutex.lock();
        m_SoundList->emptyHashTable();
        pTable.clear();
        WorkList.clear();
        pMutex.unlock();
    }



    // Play sound directly from our sound pool
    void cAudio::playSound(const String& Filename, const VECTOR3D* vec)
    {
        MutexLocker locker(pMutex);
        if (vec && Camera::inGame && ((VECTOR)(*vec - Camera::inGame->rpos)).sq() > 360000.0f) // If the source is too far, does not even think about playing it!
            return;
        if (!m_FMODRunning)
            return;

        String szWav(Filename); // copy string to szWav so we can work with it.
        // if it has a .wav extension then remove it.
        String::size_type i = szWav.toLower().find(".wav");
        if (i != String::npos)
            szWav.resize(szWav.length() - 4);

        m_SoundListItem *m_Sound = m_SoundList->find(szWav);
        if (!m_Sound)
        {
            Console->AddEntry("%s not found, aborting", Filename.c_str());
            return;
        }

        if (msec_timer - m_Sound->last_time_played < m_min_ticks)
            return;			// Make sure it doesn't play too often, so it doesn't play too loud!

        m_Sound->last_time_played = msec_timer;

        if (!m_Sound->m_SampleHandle || (m_Sound->m_3DSound && !vec))
        {
            if (!m_Sound->m_SampleHandle)
                Console->AddEntry("%s not played the good way", (char*)Filename.c_str());
            else
                Console->AddEntry("%s : m_Sound->m_SampleHandle is false", (char*)Filename.c_str());
            return;
        }

        //	Console->AddEntry("plays %s", (char*)Filename.c_str());

        m_WorkListItem	m_Work;

        m_Work.m_Sound = m_Sound;
        m_Work.vec = (VECTOR *)vec;

        WorkList.push_back(m_Work);
    }



    void cAudio::playTDFSoundNow(const String& Key, const VECTOR3D* vec)		// Wrapper to playTDFSound + update3DSound
    {
        String szWav = pTable.pullAsString(String::ToLower(Key)); // copy string to szWav so we can work with it.
        String::size_type i = szWav.toLower().find(".wav");
        if (i != String::npos)
            szWav.resize(szWav.length() - 4);

        m_SoundListItem* m_Sound = m_SoundList->find(szWav);

        if (m_Sound)
        {
            m_Sound->last_time_played = msec_timer - 1000 - m_min_ticks;		// Make sure it'll be played
            playTDFSound(Key, vec);
        }
        update3DSound();
    }



    // Play sound from TDF by looking up sound filename from internal hash
    void cAudio::playTDFSound(String key, const VECTOR3D* vec)
    {
        if (key.empty())
            return;
        if (!pTable.exists(key.toLower()))
        {
            // output a report to the console but only once
            Console->AddEntryWarning("%sCan't find key %s", TA3D_LOG_SECTION_AUDIO_PREFIX, key.c_str());
            pTable.insertOrUpdate(key, "");
            return;
        }
        String szWav = pTable.pullAsString(key);
        if (!szWav.empty())
            playSound(szWav, vec);
    }



    // keys will be added together and then PlayTDF( key, vec ); if either key is
    // "" it aborts.
    void cAudio::playTDFSound(const String& keyA, const String& keyB, const VECTOR3D* vec)
    {
        if (!keyA.empty() && !keyB.empty())
        {
            String key;
            key << keyA << "." << keyB;
            playTDFSound(key, vec);
        }
    }


    cAudio::m_SoundListItem::~m_SoundListItem()
    {
        # ifdef TA3D_PLATFORM_MINGW
        if (m_SampleHandle)
            FMOD_Sound_Release(m_SampleHandle);
        # else
        if (m_SampleHandle)
            m_SampleHandle->release();
        # endif
        m_SampleHandle = NULL;
    }

} // namespace Interfaces
} // namespace TA3D
