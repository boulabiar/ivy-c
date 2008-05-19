/*
 *	Ivy, C interface
 *
 *	Copyright 1997-2000
 *	Centre d'Etudes de la Navigation Aerienne
 *
 *	Sockets
 *
 *	Authors: Francois-Regis Colin <fcolin@cena.fr>
 *
 *	$Id$
 *
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 */

#ifdef OPENMP
#include <omp.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

#ifdef WIN32
typedef int ssize_t;
#define close closesocket
/*#define perror (a ) printf(a" error=%d\n",WSAGetLastError());*/
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#endif

#include "param.h"
#include "list.h"
#include "ivychannel.h"
#include "ivysocket.h"
#include "ivyloop.h"
#include "ivybuffer.h"
#include "ivyfifo.h"
#include "ivydebug.h"

struct _server {
	Server next;
	HANDLE fd;
	Channel channel;
	unsigned short port;
	void *(*create)(Client client);
	void (*handle_delete)(Client client, const void *data);
	void (*handle_decongestion)(Client client, const void *data);
	SocketInterpretation interpretation;
};

struct _client {
	Client next;
	HANDLE fd;
	Channel channel;
	unsigned short port;
        char app_uuid[128];
	struct sockaddr_in from;
	SocketInterpretation interpretation;
	void (*handle_delete)(Client client, const void *data);
	void (*handle_decongestion)(Client client, const void *data);
	char terminator;	/* character delimiter of the message */ 
	/* Buffer de reception */
	long buffer_size;
	char *buffer;		/* dynamicaly reallocated */
	char *ptr;
	/* Buffer d'emission */
        IvyFifoBuffer *ifb;             /* le buffer circulaire en cas de congestion */
  	/* user data */
	const void *data;
#ifdef OPENMP
	omp_lock_t fdLock;
#endif
};

static Server servers_list = NULL;
static Client clients_list = NULL;

static int debug_send = 0;

#ifdef WIN32
WSADATA	WsaData;
#endif


static SendState BufferizedSocketSendRaw (const Client client, const char *buffer, const int len );


void SocketInit()
{
	if ( getenv( "IVY_DEBUG_SEND" )) debug_send = 1;
	IvyChannelInit();
}

static void DeleteSocket(void *data)
{
	Client client = (Client )data;
	if (client->handle_delete )
		(*client->handle_delete) (client, client->data );
	shutdown (client->fd, 2 );
	close (client->fd );
#ifdef OPENMP
	omp_destroy_lock (&(client->fdLock));
#endif
	if (client->ifb != NULL) {
	  IvyFifoDelete (client->ifb);
	  client->ifb = NULL;
	}
	
	IVY_LIST_REMOVE (clients_list, client );
}


static void DeleteServerSocket(void *data)
{
        Server server = (Server )data;
#ifdef BUGGY_END
        if (server->handle_delete )
                (*server->handle_delete) (server, NULL );
#endif
        shutdown (server->fd, 2 );
        close (server->fd );
        IVY_LIST_REMOVE (servers_list, server);
}


static void HandleSocket (Channel channel, HANDLE fd, void *data)
{
	Client client = (Client)data;
	char *ptr;
	char *ptr_nl;
	long nb_to_read = 0;
	long nb;
	long nb_occuped;
	socklen_t len;
	
	/* limitation taille buffer */
	nb_occuped = client->ptr - client->buffer;
	nb_to_read = client->buffer_size - nb_occuped;
	if (nb_to_read == 0 ) {
		client->buffer_size *= 2; /* twice old size */
		client->buffer = realloc( client->buffer, client->buffer_size );
		if (!client->buffer )
		{
		fprintf(stderr,"HandleSocket Buffer Memory Alloc Error\n");
		exit(0);
		}
		fprintf(stderr, "Buffer Limit reached realloc new size %ld\n", client->buffer_size );
		nb_to_read = client->buffer_size - nb_occuped;
		client->ptr = client->buffer + nb_occuped; 
	}
	len = sizeof (client->from );
	nb = recvfrom (fd, client->ptr, nb_to_read,0,(struct sockaddr *)&client->from,
		       &len);
	if (nb  < 0) {
		perror(" Read Socket ");
		IvyChannelRemove (client->channel );
		return;
	}
	if (nb == 0 ) {
		IvyChannelRemove (client->channel );
		return;
	}
	client->ptr += nb;
	ptr = client->buffer;
	while ((ptr_nl = memchr (ptr, client->terminator,  client->ptr - ptr )))
		{
		*ptr_nl ='\0';
		if (client->interpretation )
			(*client->interpretation) (client, client->data, ptr );
			else fprintf (stderr,"Socket No interpretation function ???\n");
		ptr = ++ptr_nl;
		}
	if (ptr < client->ptr )
		{ /* recopie ligne incomplete au debut du buffer */
		len = client->ptr - ptr;
		memmove (client->buffer, ptr, len  );
		client->ptr = client->buffer + len;
		}
		else
		{
		client->ptr = client->buffer;
		}
}



static void HandleCongestionWrite (Channel channel, HANDLE fd, void *data)
{
  Client client = (Client)data;
  
  if (IvyFifoSendSocket (client->ifb, fd) == 0) {
    // Not congestionned anymore
    IvyChannelClearWritableEvent (channel);
    //    printf ("DBG> Socket *DE*congestionnee\n");
    IvyFifoDelete (client->ifb);
    client->ifb = NULL;
    if (client->handle_decongestion )
      (*client->handle_decongestion) (client, client->data );

  }
}


static void HandleServer(Channel channel, HANDLE fd, void *data)
{
	Server server = (Server ) data;
	Client client;
	HANDLE ns;
	socklen_t addrlen;
	struct sockaddr_in remote2;
#ifdef WIN32
	u_long iMode = 1; /* non blocking Mode */
#else
	long   socketFlag;
#endif
	TRACE( "Accepting Connection...\n");

	addrlen = sizeof (remote2 );
	if ((ns = accept (fd, (struct sockaddr *)&remote2, &addrlen)) <0)
		{
		perror ("*** accept ***");
		return;
		};

	TRACE( "Accepting Connection ret\n");

	IVY_LIST_ADD_START (clients_list, client );
	
	client->buffer_size = IVY_BUFFER_SIZE;
	client->buffer = malloc( client->buffer_size );
	if (!client->buffer )
		{
		fprintf(stderr,"HandleSocket Buffer Memory Alloc Error\n");
		exit(0);
		}
		client->terminator = '\n';
	client->from = remote2;
	client->fd = ns;
	client->ifb = NULL;
	strcpy (client->app_uuid, "init by HandleServer");

#ifdef WIN32
	if ( ioctlsocket(client->fd,FIONBIO, &iMode ) )
		fprintf(stderr,"Warning : Setting socket in nonblock mode FAILED\n");
#else
	socketFlag = fcntl (client->fd, F_GETFL);
	if (fcntl (client->fd, F_SETFL, socketFlag|O_NONBLOCK)) {
	  fprintf(stderr,"Warning : Setting socket in nonblock mode FAILED\n");
	}
#endif



	client->channel = IvyChannelAdd (ns, client,  DeleteSocket, HandleSocket,
					 HandleCongestionWrite);
	client->interpretation = server->interpretation;
	client->ptr = client->buffer;
	client->handle_delete = server->handle_delete;
	client->handle_decongestion = server->handle_decongestion;
	client->data = (*server->create) (client );
#ifdef OPENMP
	omp_init_lock (&(client->fdLock));
#endif


	IVY_LIST_ADD_END (clients_list, client );
	
}

Server SocketServer(unsigned short port, 
	void*(*create)(Client client),
	void(*handle_delete)(Client client, const void *data),
        void(*handle_decongestion)(Client client, const void *data),
	void(*interpretation) (Client client, const void *data, char *ligne))
{
	Server server;
	HANDLE fd;
	int one=1;
	struct sockaddr_in local;
	socklen_t addrlen;
		

	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0){
		perror ("***open socket ***");
		exit(0);
		};

	
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY; 
	local.sin_port = htons (port);

	
	if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char*)&one,sizeof(one)) < 0)
	  {
#ifdef WIN32
	    fprintf(stderr," setsockopt %d\n",WSAGetLastError());
#endif
	    perror ("*** set socket option SO_REUSEADDR ***");
	    exit(0);
	  } 

#ifdef SO_REUSEPORT

	if (setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, (char *)&one, sizeof (one)) < 0)
	  {
	    perror ("*** set socket option REUSEPORT ***");
	    exit(0);
	  }
#endif
	
	

	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0)
		{
		perror ("*** bind ***");
		exit(0);
		}

	addrlen = sizeof (local );
	if (getsockname(fd,(struct sockaddr *)&local, &addrlen) < 0)
		{
		perror ("***get socket name ***");
		exit(0);
		} 
	
	if (listen (fd, 128) < 0){
		perror ("*** listen ***");
		exit(0);
		};
	

	IVY_LIST_ADD_START (servers_list, server );
	server->fd = fd;
	server->channel = IvyChannelAdd (fd, server, DeleteServerSocket, 
					 HandleServer, NULL);
	server->create = create;
	server->handle_delete = handle_delete;
	server->handle_decongestion = handle_decongestion;
	server->interpretation = interpretation;
	server->port = ntohs(local.sin_port);
	IVY_LIST_ADD_END (servers_list, server );
	
	return server;
}

unsigned short SocketServerGetPort (Server server )
{
	return server ? server->port : 0;
}

void SocketServerClose (Server server )
{
	if (!server)
		return;
	IvyChannelRemove (server->channel );
}

char *SocketGetPeerHost (Client client )
{
	int err;
	struct sockaddr_in name;
	struct hostent *host;
	socklen_t len = sizeof(name);

	if (!client)
		return "undefined";

	err = getpeername (client->fd, (struct sockaddr *)&name, &len );
	if (err < 0 ) return "can't get peer";
	host = gethostbyaddr ((char *)&name.sin_addr.s_addr,sizeof(name.sin_addr.s_addr),name.sin_family);
	if (host == NULL ) return "can't translate addr";
	return host->h_name;
}

struct in_addr * SocketGetRemoteAddr (Client client )
{
	return client ? &client->from.sin_addr : 0;
}

void SocketGetRemoteHost (Client client, char **host, unsigned short *port )
{
	struct hostent *hostp;

	if (!client)
		return;

	/* extract hostname and port from last message received */
	hostp = gethostbyaddr ((char *)&client->from.sin_addr.s_addr,
			sizeof(client->from.sin_addr.s_addr),client->from.sin_family);
	if (hostp == NULL ) *host = "unknown";
		else *host = hostp->h_name;
	*port = ntohs (client->from.sin_port );
}

void SocketClose (Client client )
{
	if (client)
		IvyChannelRemove (client->channel );
}

SendState SocketSendRaw (const Client client, const char *buffer, const int len )
{
  SendState state;
  
  if (!client)
    return SendParamError;
  
#ifdef OPENMP
  omp_set_lock (&(client->fdLock));
#endif
  
  state = BufferizedSocketSendRaw (client, buffer, len);

#ifdef OPENMP
  omp_unset_lock (&(client->fdLock));
#endif
  
  return state;
}


static SendState BufferizedSocketSendRaw (const Client client, const char *buffer, const int len )
{
  ssize_t reallySent;
  SendState state;

  if (client->ifb != NULL) {
    // Socket en congestion : on rajoute juste le flux dans le buffer, 
    // quand la socket sera dispo en ecriture, le select appellera la callback
    // pour vider ce buffer
    IvyFifoWrite (client->ifb, buffer, len);
    state = IvyFifoIsFull (client->ifb) ? SendStateFifoFull : SendStillCongestion;
  } else {
    // on tente d'ecrire direct dans la socket
    reallySent =  send (client->fd, buffer, len, 0);
    if (reallySent == len) 
	{
      state = SendOk; // PAS CONGESTIONNEE
    } else if (reallySent == -1) 
	{
#ifdef WIN32
	if ( WSAGetLastError() == WSAEWOULDBLOCK) {
#else
      if (errno == EWOULDBLOCK) {
#endif
	// Aucun octet n'a �t� envoy�, mais le send ne rend pas 0
	// car 0 peut �tre une longueur pass�e au send, donc dans ce cas
	// send renvoie -1 et met errno a EWOULDBLOCK
	client->ifb = IvyFifoNew ();
	IvyFifoWrite (client->ifb, buffer, len);
	// on ajoute un fdset pour que le select appelle une callback pour vider
	// le buffer quand la socket sera ?? nouveau libre
	IvyChannelAddWritableEvent (client->channel);
	state = SendStateChangeToCongestion;
      } else {
	state = SendError; // ERREUR
      }
    } else {
      // socket congestionn�e
      // on initialise une fifo pour accumuler les donn�es
      client->ifb = IvyFifoNew ();
      IvyFifoWrite (client->ifb, &(buffer[reallySent]), len-reallySent);
      // on ajoute un fdset pour que le select appelle une callback pour vider
      // le buffer quand la socket sera � nouveau libre
      IvyChannelAddWritableEvent (client->channel);
      state = SendStateChangeToCongestion;
    }
  }

#ifdef DEBUG
  // DBG BEGIN DEBUG
  /* SendOk, SendStillCongestion, SendStateChangeToCongestion,
          SendStateChangeToDecongestion, SendStateFifoFull, SendError,
	  SendParamError
  */
  {
    static SendState DBG_state = SendOk;
    char *litState="";
    if (state != DBG_state) {
      switch (state) {
      case SendOk : litState = "SendOk";
	break;
      case  SendStillCongestion: litState = "SendStillCongestion";
	break;
      case SendStateChangeToCongestion : litState = "SendStateChangeToCongestion";
	break;
      case  SendStateChangeToDecongestion: litState = "SendStateChangeToDecongestion";
	break;
      case  SendStateFifoFull: litState = "SendStateFifoFull";
	break;
      case  SendError: litState = "SendError";
	break;
      case  SendParamError: litState = "SendParamError";
	break;
      }
      printf ("DBG>> BufferizedSocketSendRaw, state changed to '%s'\n", litState);
      DBG_state = state;
    }
  }
  // DBG END DEBUG
#endif

  return (state);
}



SendState SocketSendRawWithId( const Client client, const char *id, const char *buffer, const int len )
{
  SendState s1, s2;
  
#ifdef OPENMP
  omp_set_lock (&(client->fdLock));
#endif
  
  s1 = BufferizedSocketSendRaw (client, id, strlen (id));

  s2 = BufferizedSocketSendRaw (client, buffer, len);
  
#ifdef OPENMP
  omp_unset_lock (&(client->fdLock));
#endif
  
  if (s1 == SendStateChangeToCongestion) {
    // si le passage en congestion s'est fait sur l'envoi de l'id
    s2 = s1;
  }

  return (s2);
}


void SocketSetData (Client client, const void *data )
{
  if (client) {
    client->data = data;
  }
}

SendState SocketSend (Client client, char *fmt, ... )
{
  SendState state;
  static IvyBuffer buffer = {NULL, 0, 0 }; /* Use static mem to eliminate multiple call to malloc /free */
#ifdef OPENMP
#pragma omp threadprivate (buffer)
#endif

  va_list ap;
  int len;
  va_start (ap, fmt );
  buffer.offset = 0;
  len = make_message (&buffer, fmt, ap );
  state = SocketSendRaw (client, buffer.data, len );
  va_end (ap );
  return state;
}

const void *SocketGetData (Client client )
{
	return client ? client->data : 0;
}

void SocketBroadcast ( char *fmt, ... )
{
	Client client;
	static IvyBuffer buffer = {NULL, 0, 0 }; /* Use static mem to eliminate 
						    multiple call to malloc /free */
#ifdef OPENMP
#pragma omp threadprivate (buffer)
#endif
	va_list ap;
	int len;
	
	va_start (ap, fmt );
	len = make_message (&buffer, fmt, ap );
	va_end (ap );
	IVY_LIST_EACH (clients_list, client )
		{
		SocketSendRaw (client, buffer.data, len );
		}
}

/*
Ouverture d'un canal TCP/IP en mode client
*/
Client SocketConnect (char * host, unsigned short port, 
			void *data, 
			SocketInterpretation interpretation,
		        void (*handle_delete)(Client client, const void *data),
		        void(*handle_decongestion)(Client client, const void *data)
			)
{
	struct hostent *rhost;

	if ((rhost = gethostbyname (host )) == NULL) {
		fprintf(stderr, "Erreur %s Calculateur inconnu !\n",host);
		 return NULL;
	}
	return SocketConnectAddr ((struct in_addr*)(rhost->h_addr), port, data, 
				  interpretation, handle_delete, handle_decongestion);
}

Client SocketConnectAddr (struct in_addr * addr, unsigned short port, 
			  void *data, 
			  SocketInterpretation interpretation,
			  void (*handle_delete)(Client client, const void *data),
		          void(*handle_decongestion)(Client client, const void *data)
			  )
{
	HANDLE handle;
	Client client;
	struct sockaddr_in remote;
#ifdef WIN32
	u_long iMode = 1; /* non blocking Mode */
#else
	long   socketFlag;
#endif

	remote.sin_family = AF_INET;
	remote.sin_addr = *addr;
	remote.sin_port = htons (port);

	if ((handle = socket (AF_INET, SOCK_STREAM, 0)) < 0){
		perror ("*** client socket ***");
		return NULL;
	};

	if (connect (handle, (struct sockaddr *)&remote, sizeof(remote) ) < 0){
		perror ("*** client connect ***");
		return NULL;
	};
#ifdef WIN32
	if ( ioctlsocket(handle,FIONBIO, &iMode ) )
		fprintf(stderr,"Warning : Setting socket in nonblock mode FAILED\n");
#else
	socketFlag = fcntl (handle, F_GETFL);
	if (fcntl (handle, F_SETFL, socketFlag|O_NONBLOCK)) {
	  fprintf(stderr,"Warning : Setting socket in nonblock mode FAILED\n");
	}
#endif
	IVY_LIST_ADD_START(clients_list, client );
	
	client->buffer_size = IVY_BUFFER_SIZE;
	client->buffer = malloc( client->buffer_size );
	if (!client->buffer )
		{
		fprintf(stderr,"HandleSocket Buffer Memory Alloc Error\n");
		exit(0);
		}
		client->terminator = '\n';
	client->fd = handle;
	client->channel = IvyChannelAdd (handle, client,  DeleteSocket, 
					 HandleSocket, HandleCongestionWrite );
	client->interpretation = interpretation;
	client->ptr = client->buffer;
	client->data = data;
	client->handle_delete = handle_delete;
	client->handle_decongestion = handle_decongestion;
	client->from.sin_family = AF_INET;
	client->from.sin_addr = *addr;
	client->from.sin_port = htons (port);
	client->ifb = NULL;
	strcpy (client->app_uuid, "init by SocketConnectAddr");


#ifdef OPENMP
	omp_init_lock (&(client->fdLock));
#endif
	IVY_LIST_ADD_END(clients_list, client );
	

	return client;
}
/* TODO factoriser avec HandleRead !!!! */
int SocketWaitForReply (Client client, char *buffer, int size, int delai)
{
	fd_set rdset;
	struct timeval timeout;
	struct timeval *timeoutptr = &timeout;
	int ready;
	char *ptr;
	char *ptr_nl;
	long nb_to_read = 0;
	long nb;
	HANDLE fd;

	fd = client->fd;
	ptr = buffer;
	timeout.tv_sec = delai;
	timeout.tv_usec = 0;
   	do {
		/* limitation taille buffer */
		nb_to_read = size - (ptr - buffer );
		if (nb_to_read == 0 )
			{
			fprintf(stderr, "Erreur message trop long sans LF\n");
			ptr  = buffer;
			return -1;
			}
		FD_ZERO (&rdset );
		FD_SET (fd, &rdset );
		ready = select(fd+1, &rdset, 0,  0, timeoutptr);
		if (ready < 0 )
			{
			perror("select");
			return -1;
			}
		if (ready == 0 )
			{
			return -2;
			}
		if ((nb = recv (fd , ptr, nb_to_read, 0 )) < 0)
			{
			perror(" Read Socket ");
			return -1;
			}
		if (nb == 0 )
			return 0;

		ptr += nb;
		*ptr = '\0';
		ptr_nl = strchr (buffer, client->terminator );
	} while (!ptr_nl );
	*ptr_nl = '\0';
	return (ptr_nl - buffer);
}

/* Socket UDP */

Client SocketBroadcastCreate (unsigned short port, 
				void *data, 
				SocketInterpretation interpretation
			)
{
	HANDLE handle;
	Client client;
	struct sockaddr_in local;
	int on = 1;

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons (port);

	if ((handle = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
		perror ("*** dgram socket ***");
		return NULL;
	};

	/* wee need to used multiple client on the same host */
	if (setsockopt (handle, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on)) < 0)
		{
			perror ("*** set socket option REUSEADDR ***");
			return NULL;
		};
#ifdef SO_REUSEPORT

	if (setsockopt (handle, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof (on)) < 0)
		{
			perror ("*** set socket option REUSEPORT ***");
			return NULL;
		}
#endif
	/* wee need to broadcast */
	if (setsockopt (handle, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof (on)) < 0)
		{
			perror ("*** BROADCAST ***");
			return NULL;
		};

	if (bind(handle, (struct sockaddr *)&local, sizeof(local)) < 0)
		{
			perror ("*** test BIND ***");
			return NULL;
		};

	IVY_LIST_ADD_START(clients_list, client );
	
	client->buffer_size = IVY_BUFFER_SIZE;
	client->buffer = malloc( client->buffer_size );
	if (!client->buffer )
		{
		perror("HandleSocket Buffer Memory Alloc Error: ");
		exit(0);
		}
	client->terminator = '\n';
	client->fd = handle;
	client->channel = IvyChannelAdd (handle, client,  DeleteSocket, 
					 HandleSocket, HandleCongestionWrite);
	client->interpretation = interpretation;
	client->ptr = client->buffer;
	client->data = data;
	client->ifb = NULL;
	strcpy (client->app_uuid, "init by SocketBroadcastCreate");

#ifdef OPENMP
	omp_init_lock (&(client->fdLock));
#endif
	IVY_LIST_ADD_END(clients_list, client );
	
	return client;
}

void SocketSendBroadcast (Client client, unsigned long host, unsigned short port, char *fmt, ... )
{
	struct sockaddr_in remote;
	static IvyBuffer buffer = { NULL, 0, 0 }; /* Use satic mem to eliminate multiple call to malloc /free */
#ifdef OPENMP
#pragma omp threadprivate (buffer)
#endif
	va_list ap;
	int err,len;

	if (!client)
		return;

	va_start (ap, fmt );
	buffer.offset = 0;
	len = make_message (&buffer, fmt, ap );
	/* Send UDP packet to the dest */
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = htonl (host );
	remote.sin_port = htons(port);
	err = sendto (client->fd, 
			buffer.data, len,0,
			(struct sockaddr *)&remote,sizeof(remote));
	if (err != len) {
		perror ("*** send ***");
	}	va_end (ap );
}

/* Socket Multicast */

int SocketAddMember(Client client, unsigned long host )
{
	struct ip_mreq imr;
/*
Multicast datagrams with initial TTL 0 are restricted to the same host. 
Multicast datagrams with initial TTL 1 are restricted to the same subnet. 
Multicast datagrams with initial TTL 32 are restricted to the same site. 
Multicast datagrams with initial TTL 64 are restricted to the same region. 
Multicast datagrams with initial TTL 128 are restricted to the same continent. 
Multicast datagrams with initial TTL 255 are unrestricted in scope. 
*/
	unsigned char ttl = 64 ; /* Arbitrary TTL value. */
	/* wee need to broadcast */

	imr.imr_multiaddr.s_addr = htonl( host );
	imr.imr_interface.s_addr = INADDR_ANY; 
	if(setsockopt(client->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&imr,sizeof(struct ip_mreq)) == -1 )
		{
      	perror("setsockopt() Cannot join group");
      	fprintf(stderr, "Does your kernel support IP multicast extensions ?\n");
      	return 0;
    		}
				
  	if(setsockopt(client->fd, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) < 0 )
		{
      	perror("setsockopt() Cannot set TTL");
      	fprintf(stderr, "Does your kernel support IP multicast extensions ?\n");
      	return 0;
    		}

	return 1;
}

extern void SocketSetUuid (Client client, const char *uuid)
{
  strncpy (client->app_uuid, uuid, sizeof (client->app_uuid));
}

const char* SocketGetUuid (const Client client)
{
  return client->app_uuid;
}

extern int  SocketCmpUuid (const Client c1, const Client c2)
{
  return strncmp (c1->app_uuid, c2->app_uuid, sizeof (c1->app_uuid));
}


