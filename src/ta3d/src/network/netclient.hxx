#ifndef __TA3D_NETWORK_NETCLIENT_HXX__
#define __TA3D_NETWORK_NETCLIENT_HXX__

namespace TA3D
{

	inline NetClient::NetState NetClient::getState() const
	{
		return state;
	}

	inline String::Vector NetClient::getPeerList() const
	{
		return peerList;
	}

	inline bool NetClient::messageWaiting() const
	{
		return !messages.empty();
	}

	inline String NetClient::getLogin() const
	{
		return login;
	}

	inline String NetClient::getChan() const
	{
		return currentChan;
	}

	inline String NetClient::getServerJoined() const
	{
		return serverJoined;
	}

	inline String::Vector NetClient::getChanList() const
	{
		return chanList;
	}

	inline NetClient::Ptr NetClient::instance()
	{
		if (!pInstance)
			pInstance = NetClient::Ptr(new NetClient);
		return pInstance;
	}

} // namespace TA3D

#endif // __TA3D_NETWORK_NETCLIENT_HXX__
