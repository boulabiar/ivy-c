/*
 *	Ivy, C interface
 *
 *      Copyright (C) 1997-1999
 *      Centre d'�tudes de la Navigation A�rienne
 *
 * 	Sockets
 *
 *      Authors: Fran�ois-R�gis Colin <colin@cenatoulouse.dgac.fr>
 *		 St�phane Chatty <chatty@cenatoulouse.dgac.fr>
 *
 *	$Id$
 * 
 *      Please refer to file version.h for the
 *      copyright notice regarding this software
 *
 */
#ifndef _IVYSOCKET_H
#define _IVYSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif
	
/* general Handle */

#define ANYPORT	0

#ifdef WIN32
#include <windows.h>
#define HANDLE SOCKET
#else
#define HANDLE int
#include <netinet/in.h>
#endif

/* General Init */
extern void SocketInit();

/* Forward def */
typedef struct _client *Client;
typedef void (*SocketInterpretation)( Client client, void *data, char *ligne);

/* Server Part */
typedef struct _server *Server;
extern Server SocketServer(unsigned short port, 
	void*(*create)(Client client),
	void(*handle_delete)(Client client, void *data),
	SocketInterpretation interpretation);
extern unsigned short SocketServerGetPort( Server server );
extern void SocketServerClose( Server server );

/* Client Part */

extern void SocketClose( Client client );
extern void SocketSend( Client client, char *fmt, ... );
extern void SocketSendRaw( Client client, char *buffer, int len );
extern char *SocketGetPeerHost( Client client );
extern void SocketSetData( Client client, void *data );
extern void *SocketGetData( Client client );
extern void SocketBroadcast( char *fmt, ... );
extern Client SocketConnect( char * host, unsigned short port,
			void *data, 
			SocketInterpretation interpretation,
			void (*handle_delete)(Client client, void *data)
 );
extern Client SocketConnectAddr( struct in_addr * addr, unsigned short port, 
			void *data, 
			SocketInterpretation interpretation,
			void (*handle_delete)(Client client, void *data)
			);
extern int SocketWaitForReply( Client client, char *buffer, int size, int delai);

/* Socket UDP */
/* Creation d'une socket en mode non connecte */
/* et ecoute des messages */
extern Client SocketBroadcastCreate(
			unsigned short port, 
			void *data, 
			SocketInterpretation interpretation
			);
/* recuperation de l'emetteur du message */
extern struct in_addr * SocketGetRemoteAddr( Client client );
extern void SocketGetRemote( Client client, char **host, unsigned short *port );
/* emmission d'un broadcast UDP */
extern void SocketSendBroadcast( Client client, unsigned long host, unsigned short port, char *fmt, ... );

#ifdef __cplusplus
}
#endif

#endif

