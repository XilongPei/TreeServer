/******************************************************************************\
 * tree_tcp.h
 * 1999.4.25
\******************************************************************************/

#ifndef __TCP_H__
#define __TCP_H__

int tcpipStartup(char *hostName, char *szSocketOrService);
int sendPacket(SOCKET sock, char *szPacket, int packetSize);
int recvPacket(SOCKET sock, char *szPacket, int bufSize);
int closeTcpip(SOCKET sock);

#endif