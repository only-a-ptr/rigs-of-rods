
#include "stdafx.h"
#include <iostream>
#include <stdint.h>

void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

struct sdbox
{
    int32_t time; // time in seconds since game start
    int32_t posx; // position m
    int32_t posy;
    int32_t posz;
    float   velx; // velocity m.s-1
    float   vely;
    float   velz;
    float   angvx; // angular velocity, rad/s
    float   angvy;
    float   angvz;
    float   accx; // acceleration, m.s-2
    float   accy;
    float   accz;
    float   orientx; // orientation vector, radians
    float   orienty;
    float   orientz;
    int32_t game; // 4 letter game identification, e.g. "GAME" would be 0x71657769
};

const char    DBOX_GAME[]    = {'Y','e','s','!'};
const size_t  RATE_PER_SEC   = 50;
const size_t  DURATION_SEC   = 10;

using namespace std;

int main()
{
    cout<<"Start"<<endl;
    int res = enet_initialize();
    if (res != 0)
    {
        cout << "Failed to initialize ENET. Exit"<<endl;
        return -1;
    }

    ENetSocket socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    cout<<"socket created: " << socket <<endl;

    ENetAddress addr_local;
    addr_local.host = ENET_HOST_ANY;
    addr_local.port = 43000;
    res = enet_socket_bind(socket, &addr_local);
    cout<<"socket bound, result: "<<res<<endl;

    if (res != 0)
    {
#ifdef _MSC_VER
        char *s = NULL;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                       NULL, WSAGetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPSTR)&s, 0, NULL);
        cout << "Message: " << s << endl;
        LocalFree(s);
#endif
        cout << "Error binding socket, Exit (press enter!)."<<endl;
        getchar(); // pause
        return -1;
    }

    ENetAddress addr_remote;
    //addr_remote.host = htonl(0x7F000001); // = 127.0.0.1 localhost
    addr_remote.host = 0x0100007F;
    addr_remote.port = 4123;

    sdbox data;
    memset(&data, 0, sizeof(sdbox));
    data.game = reinterpret_cast<int32_t>(DBOX_GAME);

    cout << "Sending data..."<<endl;
    for (size_t i = 0; i < (DURATION_SEC*RATE_PER_SEC); ++i)
    {
        ENetBuffer buf;
        buf.data       = static_cast<void*>(&data);
        buf.dataLength = sizeof(sdbox);
        res = enet_socket_send(socket, &addr_remote, &buf, 1);
        cout << res << ", ";

        // Animate the data somehow
        data.accx += 0.1f;
        data.accy += 0.1f;
        data.accz += 0.1f;

        data.angvx +=  0.1f;
        data.angvy +=  0.1f;
        data.angvz +=  0.1f;

        data.orientx +=  0.1f;
        data.orienty +=  0.1f;
        data.orientz +=  0.1f;

        data.velx += 0.1f;
        data.vely += 0.1f;
        data.velz += 0.1f;

        sleep_ms(1000/RATE_PER_SEC);
    }

    enet_socket_destroy(socket);
    cout << endl << "Exit" << endl;
    return 0;
}

