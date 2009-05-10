#ifndef __SOCKET_TCP_H__
#define __SOCKET_TCP_H__

#include "socket.h"
#include "../threads/thread.h"
#include <deque>

namespace TA3D
{

    class SocketTCP : public Socket
    {
    protected:
        TCPsocket           sock;
        SDLNet_SocketSet    set;
        bool                checked;
        bool                nonBlockingMode;
    public:
        SocketTCP();
        ~SocketTCP();

        void setNonBlockingMode(bool mode);

        void reset();

        /*virtual*/ bool isOpen() const;

        /*virtual*/ void open(const String &hostname, uint16 port);
        /*virtual*/ void close();
        void open(uint16 port);
        SocketTCP *accept();

        /*virtual*/ void check(uint32 msec);
        /*virtual*/ bool ready() const;

        /*virtual*/ void send(const char *data, int size);
        /*virtual*/ int recv(char *data, int size);
    };
}

#endif
