/******************************************************************************\
 * tree_tcp.c
 * 1999.4.25
\******************************************************************************/

//#define __TEST_


#include <windows.h>
#include "tree_tcp.h"



int FillAddr(char *hostName, char *szSocketOrService, PSOCKADDR_IN psin);




/****************************************************************************\
*
*    FUNCTION:  FillAddr
*
*    PURPOSE:  Retrieves the IP address and port number.
*
\***************************************************************************/

int FillAddr(char *hostName, char *szSocketOrService, PSOCKADDR_IN psin)
{
	PHOSTENT phe;
	PSERVENT pse;
	unsigned long ul;


	psin->sin_family = AF_INET;

	if( isdigit(*hostName) ) {
		ul = inet_addr(hostName);
		memcpy((char FAR *)&(psin->sin_addr), &ul, 4);
		//phe = gethostbyaddr((const char *)&ul, 4, PF_INET);
		//if (phe == NULL) {
		//	return  1;
		//}
	} else {
		phe = gethostbyname(hostName);
		if (phe == NULL) {
			return  1;
		}
		memcpy((char FAR *)&(psin->sin_addr), phe->h_addr, phe->h_length);
	}


	if( isdigit(*szSocketOrService) ) {
		psin->sin_port = htons( (unsigned short)atoi(szSocketOrService) );
					// Convert to network ordering
	} else {
		 //
		 //   Find the service name, szBuff, which is a type tcp protocol in
		 //   the "services" file.
		 //
		 pse = getservbyname(szSocketOrService, "tcp");
		 if (pse == NULL)  {
			return  2;
		 }
		 psin->sin_port = pse->s_port;
   }

   return  0;

} //FillAddr()




int tcpipStartup(char *hostName, char *szSocketOrService)
{
	int 			status;
	WSADATA 		WSAData;
	SOCKADDR_IN 	dest_sin;  //DESTination Socket INternet
	SOCKET 			sock;
//	int				zero;

	if ((status = WSAStartup(MAKEWORD(1,1), &WSAData)) != 0) {
		 return  -1;
	}

   /*
		 When a network client wants to connect to a server,
		 it must have:
			1.) a TCP port number (gotten via getservbyname())
			and
			2.) an IP address of the remote host (gotten via gethostbyname()).

		 The following summarizes the steps used to connect.
		 Get the name of the remote host computer in which
		  to connect from the user
	   * Check to see if the hosts file knows the computer (gethostbyname)
	   * Get the host information (hostent structure filled)
	   * Fill in the address of the remote host into the servent structure (memcpy)
	   * Get the NAME of the port to connect to on the remote host from the
		 user.
	   * Get the port number (getservbyname)
	   * Fill in the port number of the servent structure
		 Establish a connection (connect)

		 The * prefixed steps are done in the FillAddr() procedure.
   */

	sock = socket( AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		return  -2;
	}

	if( FillAddr(hostName, szSocketOrService, &dest_sin) ) {
		closesocket( sock );
		return  -3;
	}

	//
	// Disable send bufferring on the socket.  Setting SO_SNDBUF
	// to 0 causes winsock to stop bufferring sends and perform
	// sends directly from our buffers, thereby reducing CPU
	// usage.
	//

	/*
	zero = 0;
	status = setsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero) );
	if ( status == SOCKET_ERROR ) {
		closesocket( sock );
		return  -5;
	}*/

	if( connect( sock, (PSOCKADDR) &dest_sin, sizeof( dest_sin)) < 0 ) {
		closesocket( sock );
		return  -4;
	}

	return  sock;

} //tcpipStartup()


int sendPacket(SOCKET sock, char *szPacket, int packetSize)
{
	unsigned char buf[4098];
	int           i;

	if( packetSize <= 0 )
		return  0;

	memcpy(buf+1, szPacket, packetSize);

	i = (packetSize+15) / 16;

	// 4096/16 = 256
	// a unsigned char means 0 ~ 255
	*buf = (unsigned char)(i - 1);

	return  send(sock, buf, 16 * i + 1, 0);

} //sendPacket()

int recvPacket(SOCKET sock, char *szPacket, int bufSize)
{
	char 	buf[4098];
	int  	transPacketSize, sizep;
	unsigned char 	c;
	int		i;

	if( bufSize <= 0 )
		return  -1;

	if( recv(sock, &c, 1, 0) <= 0 )
		return  -1;

	sizep = 16 * c + 16;

	transPacketSize = 0;
	while( transPacketSize < sizep ) {
		if( (i = recv(sock, &buf[transPacketSize], sizep-transPacketSize, 0)) <= 0 )
			return  -1;

		transPacketSize += i;
	}
	memcpy( szPacket, buf, min(transPacketSize, bufSize) );

	return  transPacketSize;

} //recvPacket()



int closeTcpip(SOCKET sock)
{
	LINGER lingerStruct;

	lingerStruct.l_onoff = 0;
	lingerStruct.l_linger = 10;
	setsockopt( sock, SOL_SOCKET, SO_LINGER,
					(char *)&lingerStruct, sizeof(lingerStruct) );

	return  closesocket(sock);

} //closeTcpip()





#ifdef __TEST_

main()
{
	SOCKET 			sock;
	char            buf[500];
	char            buf1[1000];
	int             i;

	sock = tcpipStartup("192.192.2.197", "110");
	if( (long)sock < 0 ) {
		printf("error\n");
		return  0;
	}
	//send(sock, "hello", 5, 0);
	for(i = 0; i < 1000;  i++) {
		//recv(sock, buf, 50, 0);

		sprintf(buf1, "test %d", i);
		printf("sending %s\n", buf1);
		sendPacket(sock, buf1, strlen(buf1)+1);
		//send(sock, buf1, 50, /*strlen(buf1)+1,*/ 0);
	}
	closesocket( sock );
	return  0;
}

#endif
