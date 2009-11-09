#ifndef __SOCKET_UDP_H__
#define __SOCKET_UDP_H__

# include <misc/string.h>
#include "socket.h"

namespace TA3D
{

    class SocketUDP : public Socket
    {
    protected:
        UDPsocket           sock;
        SDLNet_SocketSet    set;
        bool                checked;
        IPaddress           remoteIP;
    public:
        SocketUDP();
        virtual ~SocketUDP();

        virtual bool isOpen() const;

        virtual void open(const String &hostname, uint16 port);
        virtual void close();

        virtual void check(uint32 msec);
        virtual bool ready() const;

        virtual void send(const String &str);
        virtual void send(const char *data, int size);
        virtual int recv(char *data, int size);

        IPaddress getRemoteIP_sdl() const;
        String getRemoteIPstr() const;
        uint32 getRemoteIP() const;
    };
}
#endif
