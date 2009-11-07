#include "../stdafx.h"
#include "../TA3D_NameSpace.h"
#include "netclient.h"
#include "../mods/mods.h"
#include <algorithm>        // We need std::sort

#define BUFFER_SIZE     2048

namespace TA3D
{


	NetClient::Ptr NetClient::pInstance = NULL;



	NetClient::Ptr NetClient::instance()
	{
		if (!pInstance)
			pInstance = new NetClient;
		return pInstance;
	}

	void NetClient::destroyInstance()
	{
		pInstance = NULL;
	}

	NetClient::NetClient() : peerList(), login(), state( NetClient::DISCONNECTED ), messages(), sock(), serverJoined()
	{
		buffer = new char[BUFFER_SIZE];
		buffer_pos = 0;
		currentChan = "*";
		hostAck = false;
	}

	NetClient::~NetClient()
	{
		disconnect();
	}

	void NetClient::disconnect()
	{
		pMutex.lock();
		sendMessage("DISCONNECT");
		sock.close();
		state = DISCONNECTED;
		peerList.clear();
		chanList.clear();
		serverList.clear();
		serverListChanged = true;
		serverJoined.clear();
		buffer_pos = 0;
		currentChan = "*";
		hostAck = false;

		pMutex.unlock();
	}

	NetClient::NetState NetClient::getState()
	{
		MutexLocker mLock(pMutex);
		return state;
	}

	String::Vector NetClient::getPeerList()
	{
		MutexLocker mLock(pMutex);
		return peerList;
	}

	bool NetClient::messageWaiting()
	{
		MutexLocker mLock(pMutex);
		return !messages.empty();
	}

	String NetClient::getNextMessage()
	{
		MutexLocker mLock(pMutex);
		String msg = messages.front();
		messages.pop_front();
		return msg;
	}

	void NetClient::sendMessage(const String &msg)
	{
		pMutex.lock();
		if (sock.isOpen())
			sock.send((msg + "\n").c_str(), msg.size() + 1);
		else
		{
			state = DISCONNECTED;
			peerList.clear();
			chanList.clear();
		}
		pMutex.unlock();
	}

	void NetClient::clearMessageQueue()
	{
		pMutex.lock();
		messages.clear();
		pMutex.unlock();
	}

	void NetClient::reconnect()
	{
		connect(server, port, login, password);
	}

	void NetClient::connect(const String &server, const uint16 port, const String &login, const String &password, bool bRegister)
	{
		pMutex.lock();
		if (sock.isOpen())
			disconnect();

		messages.clear();
		serverJoined.clear();
		hostAck = false;

		sock.open(server, port);

		if (sock.isOpen())
		{
			this->password = password;
			this->login = login;
			this->port = port;
			this->server = server;

			sock.setNonBlockingMode(true);
			buffer_pos = 0;

			state = CONNECTING;
			sendMessage(String("CLIENT ") << TA3D_ENGINE_VERSION);
			if (bRegister)
				sendMessage("REGISTER " + login + " " + password);
			else
				sendMessage("LOGIN " + login + " " + password);
			uint32 timer = msec_timer;
			bool done = false;
			while(msec_timer - timer < 10000 && !done)   // 10s timeout
			{
				rest(1);
				receive();
				int i = 0;
				while(messageWaiting() && i < messages.size())
				{
					i++;
					String::Vector args;
					String msg = getNextMessage();
					msg.explode(args, ' ');

					if (args.empty())   continue;

					if (args[0] == "CONNECTED")     // Success !
					{
						state = CONNECTED;
						done = true;
					}
					else if (args[0] == "ERROR")
					{
						done = true;
						// Let the error message on top of the message queue
						messages.push_front(msg);
						break;
					}
					else
						messages.push_back(msg);     // Don't remove other messages
				}
			}

			if (state == CONNECTED)
			{
				peerList.clear();
				chanList.clear();
				chanList.push_back("*");
				serverList.clear();
				sendMessage("GET USER LIST");   // We want to know who is there
				sendMessage("GET CHAN LIST");   // and the chan list
                sendMessage("GET MOD LIST");    // and the mod list
				sendMessage("GET SERVER LIST");	// and the server list
            }
		}

		pMutex.unlock();
	}

	void NetClient::receive()
	{
		MutexLocker mLock(pMutex);
		if (!sock.isOpen())     // Socket is closed, we can't get anything
		{
			state = DISCONNECTED;
			peerList.clear();
			chanList.clear();
			return;
		}

		int n = sock.recv(buffer + buffer_pos, BUFFER_SIZE - buffer_pos);
		if (n > 0)
		{
			buffer_pos += n;
			int e = 0;
			modListChanged = false;
			for(int i = 0 ; i < buffer_pos ; i++)
			{
				if (buffer[i] == '\n')
				{
					String msg(buffer + e, i - e);
					e = i + 1;
					messages.push_back(msg);
					processMessage(msg);
				}
			}
			if (modListChanged)
				Mods::instance()->update();
			if (e > 0)
			{
				buffer_pos -= e;
				if (buffer_pos > 0)
					memmove(buffer, buffer + e, buffer_pos);
			}
		}
	}

	void NetClient::processMessage(const String &msg)
	{
		if (msg.empty())
			return;

		// We need to split the server command into its parameters
		String::Vector args = SplitCommand(msg);

		if (args.empty())
			return;

		if (args[0] == "USER" && args.size() == 2)
		{
			bool found = false;
			for(int i = 0 ; i < peerList.size() && !found ; i++)
				if (peerList[i] == args[1])
					found = true;
			if (!found)
			{
				peerList.push_back(args[1]);
				// Sort the list
				std::sort(peerList.begin(), peerList.end());
			}
		}
		else if (args[0] == "LEAVE" && args.size() == 2)
		{
			bool found = false;
			for(int i = 0 ; i < peerList.size() && !found ; i++)
				if (peerList[i] == args[1])
				{
					found = true;
					if (i + 1 < peerList.size())
						peerList[i] = peerList.back();
					peerList.resize(peerList.size() - 1);
				}
		}
		else if (args[0] == "CHAN" && args.size() == 2)
		{
			bool found = false;
			for(int i = 0 ; i < chanList.size() && !found ; i++)
				if (chanList[i] == args[1])
					found = true;
			if (!found)
			{
				chanList.push_back(args[1]);
				// Sort the list
				std::sort(chanList.begin(), chanList.end());
			}
		}
		else if (args[0] == "DECHAN" && args.size() == 2)
		{
			bool found = false;
			for(int i = 0 ; i < chanList.size() && !found ; i++)
				if (chanList[i] == args[1])
				{
					found = true;
					if (i + 1 < chanList.size())
						chanList[i] = chanList.back();
					chanList.resize(chanList.size() - 1);
				}
		}
		else if (args[0] == "CLOSE")
		{
			disconnect();
		}
        else if (args[0] == "CLEAR" && args.size() == 3 && args[1] == "MOD" && args[2] == "LIST")
		{
            modList.clear();
			modListChanged = true;
		}
		else if (args[0] == "MOD")
        {
            ModInfo mod(msg);
            if (mod.getID() >= 0)
            {
                bool found = false;
                for(ModInfo::List::iterator i = modList.begin() ; i != modList.end() && !found ; ++i)
                {
                    found = i->getID() == mod.getID();
                    if (found)
                        *i = mod;
                }
                if (!found)
                    modList.push_back(mod);
				modListChanged = true;
            }
        }
		else if (args[0] == "SERVER")
		{
			GameServer gameServer;
			for(int i = 1 ; i < args.size() - 1 ; ++i)
			{
				if (args[i] == "NAME")
					gameServer.name = args[i + 1];
				else if (args[i] == "MOD")
					gameServer.mod = args[i + 1];
				else if (args[i] == "VERSION")
					gameServer.version = args[i + 1];
				else if (args[i] == "HOST")
					gameServer.host = args[i + 1];
				else if (args[i] == "MAP")
					gameServer.map = args[i + 1];
				else if (args[i] == "SLOTS")
					gameServer.nb_open = args[i + 1];
			}
			serverList[gameServer.name] = gameServer;
			serverListChanged = true;
		}
		else if (args[0] == "UNSERVER" && args.size() == 2)
		{
			if (serverList.count(args[1]) == 1)
			{
				serverList.erase(args[1]);
				serverListChanged = true;
			}
			else	// We've a problem, server list lost sync OO!
				sendMessage("GET SERVER LIST");
		}
		else if (args[0] == "CLEAR" && args.size() == 3 && args[1] == "SERVER" && args[2] == "LIST")
		{
			serverList.clear();
			serverListChanged = true;
		}
		else if (args[0] == "JOIN" && args.size() == 2)
			serverJoined = args[1];
		else if (args[0] == "UNJOIN" && args.size() == 2 && serverJoined == args[1])
			serverJoined.clear();
		else if (args[0] == "HOST")		// Server acknowledged, no errors we're free to go
			hostAck = true;
	}

    ModInfo::List NetClient::getModList()
    {
        MutexLocker mLock(pMutex);
		modListChanged = false;
        return modList;
    }

	String::Vector NetClient::getChanList()
	{
		MutexLocker mLock(pMutex);
		return chanList;
	}

	NetClient::GameServer::List NetClient::getServerList()
	{
		MutexLocker mLock(pMutex);
		serverListChanged = false;
		return serverList;
	}

	void NetClient::changeChan(const String &chan)
	{
		pMutex.lock();
		currentChan = chan.empty() ? "*" : chan;
		sendMessage("CHAN " + chan);
		sendMessage("GET USER LIST");
		peerList.clear();
		pMutex.unlock();
	}

	void NetClient::sendChan(const String &msg)
	{
		pMutex.lock();
		sendMessage("SENDALL " + msg);
		pMutex.unlock();
	}

	String NetClient::getLogin()
	{
		MutexLocker mLock(pMutex);
		return login;
	}

	String NetClient::getChan()
	{
		MutexLocker mLock(pMutex);
		return currentChan;
	}

	String NetClient::getServerJoined()
	{
		MutexLocker mLock(pMutex);
		return serverJoined;
	}

	void NetClient::clearServerJoined()
	{
		lock();
		serverJoined.clear();
		unlock();
	}

	bool NetClient::getHostAck()
	{
		MutexLocker mLock(pMutex);
		if (hostAck)
		{
			hostAck = false;
			return true;
		}
		return false;
	}
}
