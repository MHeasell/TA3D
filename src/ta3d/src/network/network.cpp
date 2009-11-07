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

#include "../stdafx.h"
#include "../TA3D_NameSpace.h"
#include "TA3D_Network.h"
#include "../misc/math.h"
#include "../logs/logs.h"
#include <fstream>
#include "http.h"


namespace TA3D
{

	Network	network_manager;




	/******************************/
	/**  methods for Network  *****/
	/******************************/

	Network::Network() :
		getfile_thread(), sendfile_thread(), transfer_progress(),
		specialq(64,sizeof(struct chat)),
		chatq(64,sizeof(struct chat)),
		syncq(128,sizeof(struct sync)),
		eventq(32,sizeof(struct event)),
		broadcastq(), broadcastaddressq()
	{
		myMode = 0;
		tohost_socket = NULL;
		playerDirty = false;
		fileDirty = false;
	}


	Network::~Network()
	{
		listen_thread.join();
		admin_thread.join();
		broadcast_thread.join();

		for(std::list< GetFileThread* >::iterator i = getfile_thread.begin() ; i != getfile_thread.end() ; ++i)
		{
			(*i)->join();
			delete *i;
		}
		for(std::list< SendFileThread* >::iterator i = sendfile_thread.begin() ; i != sendfile_thread.end() ; ++i)
		{
			(*i)->join();
			delete *i;
		}
		getfile_thread.clear();
		sendfile_thread.clear();
		transfer_progress.clear();

		listen_socket.close();
		broadcast_socket.close();
		broadcastq.clear();
		broadcastaddressq.clear();
		players.Shutdown();
	}

	void Network::InitBroadcast( uint16 port )
	{
		broadcast_socket.open( port );
		//spawn broadcast thread
		net_thread_params *params = new net_thread_params;
		params->network = this;
		LOG_DEBUG(LOG_PREFIX_NET << "Spawning a thread for broadcasting...");
		broadcast_thread.spawn(params);
	}



	//name is the name that shows up in a game list
	//port is the port the game listens on for connections
	//proto 4=ipv4only 6=ipv6only 0=automatic
	//not finished
	int Network::HostGame(const String &name, uint16 port)
	{
		if (myMode == 0)
		{
			myMode = 1;
		}
		else
		{
			LOG_WARNING(LOG_PREFIX_NET << "You can't host a game because you are already connected.");
			return -1;
		}


		myID = 0;
		adminDir[0] = 1;

		//setup game
		gamename = name;
		listen_socket.open(port);
		if (!listen_socket.isOpen())
		{
			LOG_WARNING(LOG_PREFIX_NET << "Failed to host game on port " << port);
			myMode = 0;
			return -1;
		}

		//spawn listening thread
		net_thread_params *params = new net_thread_params;
		params->network = this;
		LOG_DEBUG(LOG_PREFIX_NET << "Spawning a thread for listening...");
		listen_thread.spawn(params);

		//spawn admin thread
		params = new net_thread_params;
		params->network = this;
		LOG_DEBUG(LOG_PREFIX_NET << "Spawning a thread for admin...");
		admin_thread.spawn(params);

		LOG_INFO(LOG_PREFIX_NET << "Ready and working.");

		return 0;
	}



	//not finished
	int Network::Connect(const String &target, uint16 port)
	{
		if (myMode == 0)
			myMode = 2;
		else
		{
			LOG_ERROR(LOG_PREFIX_NET << "You can't connect to a game, you are already hosting one!");
			return -1;
		}

		tohost_socket = new TA3DSock();
		myID = -1;

		tohost_socket->open(target, port);
		if (!tohost_socket->isOpen())
		{
			//error couldnt connect to game
			LOG_ERROR(LOG_PREFIX_NET << "Error when connecting to game at [" << target << "]:" << port);
			delete tohost_socket;
			tohost_socket = NULL;
			myMode = 0;
			return -1;
		}

		addPlayer( tohost_socket );

		//get game info or start admin thread here
		LOG_DEBUG(LOG_PREFIX_NET << "Spawning a thread for admin");
		net_thread_params *params = new net_thread_params;
		params->network = this;
		admin_thread.spawn(params);

		LOG_INFO(LOG_PREFIX_NET << "Successfully connected to game at [" << target << "]:" << port);
		getMyID();
		return 0;
	}




	//not completely finished
	void Network::Disconnect()
	{
		listen_thread.join();
		listen_socket.close();

		broadcast_thread.join();
		broadcast_socket.close();

		ftmutex.lock();

		for (std::list< GetFileThread* >::iterator i = getfile_thread.begin() ; i != getfile_thread.end() ; ++i)
		{
			(*i)->join();
			delete *i;
		}
		for (std::list< SendFileThread* >::iterator i = sendfile_thread.begin() ; i != sendfile_thread.end() ; ++i)
		{
			(*i)->join();
			delete *i;
		}
		getfile_thread.clear();
		sendfile_thread.clear();
		ftmutex.unlock();

		ft2mutex.lock();
		transfer_progress.clear();
		ft2mutex.unlock();

		slmutex.lock();
		players.Shutdown();
		slmutex.unlock();

		cleanQueues();
		myMode = 0;

		tohost_socket = NULL;
	}

	void Network::stopFileTransfer( const String &port, int to_id )
	{
		if (port.empty())
		{
			ftmutex.lock();
			for (std::list< GetFileThread* >::iterator i = getfile_thread.begin() ; i != getfile_thread.end() ; ++i)
			{
				(*i)->join();
				delete *i;
			}
			for (std::list< SendFileThread* >::iterator i = sendfile_thread.begin() ; i != sendfile_thread.end() ; ++i)
			{
				(*i)->join();
				delete *i;
			}
			getfile_thread.clear();
			sendfile_thread.clear();
			ftmutex.unlock();
			ft2mutex.lock();
			transfer_progress.clear();
			ft2mutex.unlock();
		}
		else
		{
			ftmutex.lock();
			int nb_port = atoi( port.c_str() );
			for (std::list< GetFileThread* >::iterator i = getfile_thread.begin() ; i != getfile_thread.end() ; )
			{
				if ((*i)->port == nb_port)
				{
					GetFileThread *p = *i;
					getfile_thread.erase( i++ );

					p->join();
					delete p;

					break;
				}
				else
					++i;
			}
			for (std::list< SendFileThread* >::iterator i = sendfile_thread.begin() ; i != sendfile_thread.end() ; )
			{
				if ((*i)->port == nb_port && (to_id == -1 || to_id == (*i)->player_id ))
				{
					SendFileThread *p = *i;
					sendfile_thread.erase( i++ );

					p->join();
					delete p;

					break;
				}
				else
					++i;
			}
			ftmutex.unlock();
		}

		setFileDirty();
	}

	bool Network::isTransferFinished( const String &port )
	{
		MutexLocker mLock(ftmutex);
		int nb_port = atoi( port.c_str() );
		for (std::list< GetFileThread* >::iterator i = getfile_thread.begin() ; i != getfile_thread.end() ; i++ )
			if ((*i)->port == nb_port)
				return false;
		for (std::list< SendFileThread* >::iterator i = sendfile_thread.begin() ; i != sendfile_thread.end() ; i++ )
			if ((*i)->port == nb_port)
				return false;
		return true;
	}


	//not completely finished
	int Network::addPlayer(TA3DSock* sock)
	{
		int n;
		SocketThread* thread;

		slmutex.lock();
		n = players.Add(sock);
		slmutex.unlock();

		thread = players.getThread(n);

		if (thread == NULL)
		{
			LOG_WARNING(LOG_PREFIX_NET << "Thread not found ??? (" << __FILE__ << ":" << __LINE__ << ")");
			return -1;
		}

		net_thread_params *params = new net_thread_params;
		params->network = this;
		params->sockid = n;
		LOG_DEBUG(LOG_PREFIX_NET << "Spawning socket thread");
		thread->spawn(params);

		//send a new player event
		//eventNewPlayer(n);

		return 0;
	}

	int Network::dropPlayer(int num)
	{
		int v;

		slmutex.lock();
		v = players.Remove(num);
		playerDropped = true;
		slmutex.unlock();

		return v;
	}

	int Network::cleanPlayer()
	{
		if (!playerDirty)
			return 0;
		slmutex.lock();
		int v = 0;
		for (int i = 1; i <= players.getMaxId(); ++i)
		{
			TA3DSock *sock = players.getSock(i);
			if (sock && !sock->isOpen() )
			{
				v = players.Remove(i);
				if (sock == tohost_socket)
				{
					broadcast_thread.join();
					broadcast_socket.close();

					tohost_socket = NULL;
					myMode = 0;
				}
			}
		}
		slmutex.unlock();
		playerDirty = false;
		return v;
	}

	void Network::setPlayerDirty()
	{
		playerDirty = true;
	}

	void Network::setFileDirty()
	{
		fileDirty = true;
	}

	void Network::cleanFileThread()
	{
		if( !fileDirty )	return;
		ftmutex.lock();
		for (std::list< GetFileThread* >::iterator i = getfile_thread.begin() ; i != getfile_thread.end() ; )
		{
			if ((*i)->isDead())
			{
				ft2mutex.lock();
				for (std::list< FileTransferProgress >::iterator e = transfer_progress.begin() ; e != transfer_progress.end() ; )
					if( e->size == 0 )
						transfer_progress.erase( e++ );
					else
						e++;
				ft2mutex.unlock();

				(*i)->join();
				delete *i;
				getfile_thread.erase( i++ );
			}
			else
				i++;
		}
		for (std::list< SendFileThread* >::iterator i = sendfile_thread.begin() ; i != sendfile_thread.end() ; )
		{
			if ((*i)->isDead())
			{
				ft2mutex.lock();
				for (std::list< FileTransferProgress >::iterator e = transfer_progress.begin() ; e != transfer_progress.end() ; )
				{
					if (e->size == 0)
						transfer_progress.erase( e++ );
					else
						e++;
				}
				ft2mutex.unlock();

				(*i)->join();
				delete *i;
				sendfile_thread.erase( i++ );
			}
			else
				++i;
		}
		ftmutex.unlock();
		fileDirty = false;
	}





	int Network::getMyID()
	{
		if( myID != -1 )
			return myID;
		switch( myMode )
		{
			case 1:						// Server
				myID = 0;
				return myID;
			case 2:						// Client
				struct chat special_msg;
				if( sendSpecial( strtochat( &special_msg, "REQUEST PLAYER_ID" ) ) )
					return -1;
				else
				{
					int timeout = 5000;
					myID = -1;
					while( myID == -1 && timeout-- && myMode == 2 && tohost_socket && tohost_socket->isOpen() )
					{
						rest(1);
						if( getNextSpecial( &special_msg ) == 0 )
						{
							String::Vector params;
                            String((char*)(special_msg.message)).explode(params, ' ');
							if( params.size() == 3 && params[0] == "RESPONSE" && params[1] == "PLAYER_ID" )
							{
								myID = atoi( params[2].c_str() );
								break;
							}
						}
						if( (timeout % 1000) == 0 )				// Resend
							sendSpecial( strtochat( &special_msg, "REQUEST PLAYER_ID" ) );
					}
					return (!timeout) ? -1 /* timeout reached*/ : myID;
				}
				break;
		}
		return -1;	// Not connected
	}

	String Network::getStatus()
	{
		switch( myMode )
		{
			case 1:						// Server
				return nullptr;
			case 2:						// Client
				if (sendSpecial( "REQUEST STATUS" ))
					return nullptr;
				else
				{
					struct chat special_msg;
					int timeout = 5000;
					String status;
					while( status.empty() && timeout-- && myMode == 2 && tohost_socket && tohost_socket->isOpen() )
					{
						rest(1);
						if( getNextSpecial( &special_msg ) == 0 )
						{
							String::Vector params;
                            String((char*)(special_msg.message)).explode(params, ' ');
							if( params.size() == 3 && params[0] == "STATUS")
							{
								if (params[1] == "NEW")
									status = "";
								else if (params[1] == "SAVED")
								{
									status = params[2];
									status.replace(char(1), ' ');
								}
								break;
							}
						}
                        if( (timeout % 1000) == 0 )				// Resend
							sendSpecial( strtochat( &special_msg, "REQUEST STATUS" ) );
					}
					return (!timeout) ? nullptr /* timeout reached*/ : status;
				}
				break;
		}
		return nullptr; // Not connected
	}


	int Network::sendAll(const String& msg)
	{
		LOG_DEBUG("sendAll(\"" + msg + "\")");
		struct chat chat;
		return sendSpecial( strtochat( &chat, msg ), -1, -1, true);
	}

	int Network::sendSpecial( String msg, int src_id, int dst_id)
	{
		struct chat chat;
		return sendSpecial( strtochat( &chat, msg ), src_id, dst_id );
	}

	int Network::sendSpecial(struct chat* chat, int src_id, int dst_id, bool all)
	{
		if (src_id == -1)
			chat->from = myID;
		if (myMode == 1)
		{				// Server mode
			if( chat == NULL )	return -1;
			int v = 0;
			for(int i = 1 ; i <= players.getMaxId() ; i++)
			{
				TA3DSock *sock = players.getSock( i );
				if (sock && i != src_id && ( dst_id == -1 || i == dst_id ))
					v += sock->sendSpecial( chat, all );
			}
			return v;
		}
		else if (myMode == 2 && src_id == -1)			// Client mode
		{
			if( tohost_socket == NULL || !tohost_socket->isOpen() || chat == NULL )	return -1;
			return tohost_socket->sendSpecial( chat, all );
		}
		return -1;						// Not connected, it shouldn't be possible to get here if we're not connected ...
	}

	int Network::sendPing( int src_id, int dst_id )
	{
		if (myMode == 1)				// Server mode
		{
			int v = 0;
			for (int i = 1 ; i <= players.getMaxId() ; i++)
			{
				TA3DSock *sock = players.getSock( i );
				if( sock && i != src_id && ( dst_id == -1 || i == dst_id ) )
					v += sock->sendPing();
			}
			return v;
		}
		else if (myMode == 2 && src_id == -1)			// Client mode
		{
			if( tohost_socket == NULL || !tohost_socket->isOpen() )	return -1;
			return tohost_socket->sendPing();
		}
		return -1;						// Not connected, it shouldn't be possible to get here if we're not connected ...
	}

	int Network::sendChat(struct chat* chat, int src_id)
	{
		if (src_id == -1)
			chat->from = myID;
		if (myMode == 1)				// Server mode
		{
			if (chat == NULL)	return -1;
			int v = 0;
			for (int i = 1 ; i <= players.getMaxId() ; i++)
			{
				TA3DSock *sock = players.getSock( i );
				if (sock && i != src_id)
					v += sock->sendChat( chat );
			}
			return v;
		}
		else if (myMode == 2 && src_id == -1)			// Client mode
		{
			if (tohost_socket == NULL || !tohost_socket->isOpen() || chat == NULL)	return -1;
			return tohost_socket->sendChat( chat );
		}
		return -1;						// Not connected, it shouldn't be possible to get here if we're not connected ...
	}

	int Network::sendFileData( int player, uint16 port, byte *data, int size )
	{
		TA3DSock *sock = players.getSock( player );
		if (sock)
		{
			size += 3;
			byte buffer[size];
			buffer[0] = 'F';
			memcpy( buffer+1, &port, sizeof( port ) );
			memcpy( buffer+3, data, size - 3 );
			sock->send( buffer, size );
			return 0;
		}
		return -1;
	}

	int Network::sendFileResponse( int player, uint16 port, byte *data, int size )
	{
		TA3DSock *sock = players.getSock( player );
		if (sock)
		{
			size += 3;
			byte buffer[size];
			buffer[0] = 'R';
			memcpy( buffer+1, &port, sizeof( port ) );
			memcpy( buffer+3, data, size - 3 );
			sock->send( buffer, size );
			return 0;
		}
		return -1;
	}

	int Network::sendSync(struct sync* sync, int src_id)
	{
		if (myMode == 1)				// Server mode
		{
			if (sync == NULL)	return -1;
			int v = 0;
			for (int i = 1 ; i <= players.getMaxId() ; i++)
			{
				TA3DSock *sock = players.getSock( i );
				if (sock && i != src_id)
					v += sock->sendSync( sync );
			}
			return v;
		}
		else if (myMode == 2 && src_id == -1)			// Client mode
		{
			if (tohost_socket == NULL || !tohost_socket->isOpen() || sync == NULL)	return -1;
			return tohost_socket->sendSync( sync );
		}
		return -1;						// Not connected, it shouldn't be possible to get here if we're not connected ...
	}

	int Network::sendTick(uint32 tick, uint16 speed)
	{
		if (myMode == 1)				// Server mode
		{
			int v = 0;
			for (int i = 1 ; i <= players.getMaxId() ; i++)
			{
				TA3DSock *sock = players.getSock( i );
				if (sock)
					v += sock->sendTick(tick, speed);
			}
			return v;
		}
		else if (myMode == 2)			// Client mode
		{
			if (tohost_socket == NULL || !tohost_socket->isOpen())	return -1;
			return tohost_socket->sendTick(tick, speed);
		}
		return -1;						// Not connected, it shouldn't be possible to get here if we're not connected ...
	}

	int Network::sendEvent(struct event* event, int src_id)
	{
		if (myMode == 1)				// Server mode
		{
			if( event == NULL )	return -1;
			int v = 0;
			for (int i = 1 ; i <= players.getMaxId() ; i++)
			{
				if (i == src_id)	continue;
				TA3DSock *sock = players.getSock( i );
				if (sock)
					v = sock->sendEvent( event );
			}
			return v;
		}
		else if (myMode == 2)			// Client mode
		{
			if (tohost_socket == NULL || !tohost_socket->isOpen() || event == NULL)	return -1;
			return tohost_socket->sendEvent( event );
		}
		return -1;
	}

	int Network::sendFile(int player, const String &filename, const String &port)
	{
		ftmutex.lock();
		SendFileThread *thread = new SendFileThread();
		sendfile_thread.push_back( thread );
		thread->port = atoi( port.c_str() );
		thread->player_id = player;

		net_thread_params *params = new net_thread_params;
		params->network = this;
		params->sockid = player;
		params->filename = filename;
		LOG_DEBUG(LOG_PREFIX_NET << "Spawning a thread to send the file");
		thread->spawn(params);

		ftmutex.unlock();
		return 0;
	}


	int Network::getNextSpecial(struct chat* chat)
	{
		int v;
		xqmutex.lock();
		v = specialq.dequeue(chat);
		xqmutex.unlock();
		return v;
	}

	int Network::getNextChat(struct chat* chat)
	{
		int v;
		cqmutex.lock();
		v = chatq.dequeue(chat);
		cqmutex.unlock();
		return v;
	}

	int Network::getNextSync(struct sync* sync)
	{
		int v;
		sqmutex.lock();
		v = syncq.dequeue(sync);
		sqmutex.unlock();
		return v;
	}

	int Network::getNextEvent(struct event* event)
	{
		int v;
		eqmutex.lock();
		v = eventq.dequeue(event);
		eqmutex.unlock();
		return v;
	}

	String Network::getFile(int player, const String &filename)
	{
		ftmutex.lock();

		int port = 7776;						// Take the next port not in use
		for (std::list< GetFileThread* >::iterator i = getfile_thread.begin() ; i != getfile_thread.end(); ++i)
			port = Math::Max((*i)->port, port) ;
		++port;

		GetFileThread *thread = new GetFileThread();
		thread->port = port;
		getfile_thread.push_back( thread );

		net_thread_params *params = new net_thread_params;
		params->network = this;
		params->sockid = player;
		params->filename = filename;
		LOG_DEBUG(LOG_PREFIX_NET << "Spawning a thread to get the file");
		thread->spawn(params);

		ftmutex.unlock();
		return String(port);
	}


	int Network::broadcastMessage( const char *msg )
	{
		if( !broadcast_socket.isOpen() )
			return -1;

		broadcast_socket.send( msg, strlen(msg) + 1 );
		return broadcast_socket.isOpen() ? 0 : -1;
	}

	String Network::getNextBroadcastedMessage()
	{
		String msg;
		mqmutex.lock();
		if (!broadcastq.empty())
		{
			msg = broadcastq.front();
			broadcastq.pop_front();
			if( broadcastq.size() + 1 < broadcastaddressq.size() )
				broadcastaddressq.pop_front();
		}
		mqmutex.unlock();
		return msg;
	}

	String Network::getLastMessageAddress()
	{
		String address;
		mqmutex.lock();
		if( !broadcastaddressq.empty() )
			address = broadcastaddressq.front();
		mqmutex.unlock();
		return address;
	}

	void Network::cleanQueues()
	{
		struct chat		chat;
		struct sync		sync;
		struct event	event;

		while( getNextSpecial(&chat) == 0 )	{}
		while( getNextChat(&chat) == 0 )	{}
		while( getNextSync(&sync) == 0 )	{}
		while( getNextEvent(&event) == 0 )	{}

		mqmutex.lock();
		broadcastq.clear();
		broadcastaddressq.clear();
		mqmutex.unlock();
	}


	bool Network::BroadcastedMessages()
	{
		mqmutex.lock();
		bool result = !broadcastq.empty();
		mqmutex.unlock();
		return result;
	}

	bool Network::isConnected()
	{
		return myMode == 1 || ( myMode == 2 && tohost_socket != NULL && tohost_socket->isOpen() );
	}

	bool Network::isServer()
	{
		return myMode == 1;
	}

	bool Network::getPlayerDropped()
	{
		slmutex.lock();

		bool result = playerDropped;
		playerDropped = false;

		slmutex.unlock();

		return result;
	}

	bool Network::pollPlayer(int id)
	{
		slmutex.lock();

		bool result = (players.getSock( id ) != NULL);

		slmutex.unlock();

		return result;
	}

	float Network::getFileTransferProgress()
	{
		ft2mutex.lock();
		if( transfer_progress.empty() )
		{
			ft2mutex.unlock();
			return 100.0f;
		}

		int pos = 0;
		int size = 0;
		for (std::list< FileTransferProgress >::iterator i = transfer_progress.begin() ; i != transfer_progress.end() ; ++i)
		{
			pos += i->pos;
			size += i->size;
		}

		ft2mutex.unlock();
		return size ? 100.0f * pos / size : 100.0f;
	}

	void Network::updateFileTransferInformation( String id, int size, int pos )
	{
		ft2mutex.lock();
		for (std::list< FileTransferProgress >::iterator i = transfer_progress.begin() ; i != transfer_progress.end() ; ++i)
		{
			if( i->id == id )
			{
				i->size = size;
				i->pos = pos;
				ft2mutex.unlock();
				return;
			}
		}
		FileTransferProgress info;
		info.id = id;
		info.size = size;
		info.pos = pos;
		transfer_progress.push_back( info );

		ft2mutex.unlock();
	}

	int Network::listNetGames(std::list<SERVER_DATA>& list)
	{
        String gamelist = Http::request( lp_CONFIG->net_server, "/getserverlist.php" );

		// Remove internet servers to get a clean list
		for (std::list<SERVER_DATA>::iterator i = list.begin(); i != list.end(); )
		{
			if (i->internet)
				list.erase(i++);
			else
				++i;
		}

		if( gamelist.empty() )
			return 0;

		String::Vector line;
		gamelist.explode(line, '\n');

		int nb_servers = 0;
		int old = -1;
		SERVER_DATA cur_server;
		cur_server.internet = true;
		String server_version = "";
		String server_mod = "";
		for (String::Vector::const_iterator entry = line.begin(); entry != line.end(); ++entry)
		{
			String::Vector params;
			entry->explode(params, ' ');
			if (params.size() < 2)
				continue;
			if (params.size() == 2 && params[1] == "servers")
			{
				nb_servers = atoi( params[0].c_str() );
				continue;
			}
			int cur = atoi( params[0].c_str() );
			if( cur != old ) 						// We've all we need for this one
			{
				if( server_version != TA3D_ENGINE_VERSION || server_mod != TA3D_CURRENT_MOD )		// Not compatible!!
					nb_servers--;
				else
					list.push_back( cur_server );
			}

			if( params[1] == "name:" )
			{
				cur_server.name = "";
				for(unsigned int i = 2 ; i < params.size() ; ++i)
					cur_server.name += i > 2 ? " " + params[i] : params[i];
			}
			else if( params[1] == "IP:" )		cur_server.host = params.size() >= 3 ? params[2] : "";
			else if( params[1] == "slots:" )	cur_server.nb_open = params.size() >= 3 ? atoi( params[2].c_str() ) : 0;
			else if( params[1] == "mod:" ) {
				server_mod = "";
				for(unsigned int i = 2 ; i < params.size() ; ++i)
					server_mod += i > 2 ? " " + params[i] : params[i];
			}
			else if( params[1] == "version:" )
			{
				server_version = "";
				for(unsigned int i = 2 ; i < params.size() ; ++i)
					server_version += i > 2 ? " " + params[i] : params[i];
			}

			old = cur;
		}
		if( old != -1 ) {
			if( server_version != TA3D_ENGINE_VERSION || server_mod != TA3D_CURRENT_MOD )		// Not compatible!!
				nb_servers--;
			else
				list.push_back( cur_server );
		}
		return nb_servers;
	}

	int Network::registerToNetServer(const String &name, const int Slots)
	{
		String rName = name;
		String rMode = TA3D_CURRENT_MOD;
		String rEngine = TA3D_ENGINE_VERSION;

		rName.replace(" ", "%20");
		rMode.replace(" ", "%20");
		rEngine.replace(" ", "%20");

		String request = String::Format("/register.php?name=%s&mod=%s&version=%s&slots=%d",
			rName.c_str(), rMode.c_str(), rEngine.c_str(), Slots);
        String result = Http::request(lp_CONFIG->net_server, request);
		return 0; // TODO Fixe me with the good value !
	}


} // namespace TA3D

