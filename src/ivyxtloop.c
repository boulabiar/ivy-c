/*
 *	Ivy, C interface
 *
 *	Copyright (C) 1997-2000
 *	Centre d'�tudes de la Navigation A�rienne
 *
 * 	Main loop based on the X Toolkit
 *
 *	Authors: Fran�ois-R�gis Colin <fcolin@cena.fr>
 *		 St�phane Chatty <chatty@cena.fr>
 *
 *	$Id$
 * 
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 */

#ifdef WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef WIN32
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


#include <X11/Intrinsic.h>

#include "ivychannel.h"
#include "ivyxtloop.h"

struct _channel {
	XtInputId id_read;
	XtInputId id_delete;
	void *data;
	ChannelHandleDelete handle_delete;
	ChannelHandleRead handle_read;
	};


static int channel_initialized = 0;


static XtAppContext    app = NULL;


void IvyChannelInit(void)
{

	if ( channel_initialized ) return;

	/* pour eviter les plantages quand les autres applis font core-dump */
#ifndef WIN32
	signal( SIGPIPE, SIG_IGN);
#endif
	/* verifie si init correct */
	if ( !app )
	{
		fprintf( stderr, "You must call IvyXtChannelAppContext before XtMainLoop. Exiting.\n");
		exit(-1);
	}
	channel_initialized = 1;
}

void IvyChannelClose( Channel channel )
{

	if ( channel->handle_delete )
		(*channel->handle_delete)( channel->data );
	XtRemoveInput( channel->id_read );
	XtRemoveInput( channel->id_delete );
}
void IvyChannelStop()
{
}

static void IvyXtHandleChannelRead( XtPointer closure, int* source, XtInputId* id )
{
	Channel channel = (Channel)closure;
#ifdef DEBUG
	printf("Handle Channel read %d\n",*source );
#endif
	(*channel->handle_read)(channel,*source,channel->data);
}

static void IvyXtHandleChannelDelete( XtPointer closure, int* source, XtInputId* id )
{
	Channel channel = (Channel)closure;
#ifdef DEBUG
	printf("Handle Channel delete %d\n",*source );
#endif
	(*channel->handle_delete)(channel->data);
}


void IvyXtChannelAppContext( XtAppContext cntx )
{
	app = cntx;
}

Channel IvyChannelOpen(IVY_HANDLE fd, void *data,
				ChannelHandleDelete handle_delete,
				ChannelHandleRead handle_read
				)						
{
	Channel channel;

	channel = XtNew( struct _channel );
	if ( !channel )
		{
		fprintf(stderr,"NOK Memory Alloc Error\n");
		exit(0);
		}

	channel->handle_delete = handle_delete;
	channel->handle_read = handle_read;
	channel->data = data;

	channel->id_read = XtAppAddInput( app, fd, (XtPointer)XtInputReadMask, IvyXtHandleChannelRead, channel);
	channel->id_delete = XtAppAddInput( app, fd, (XtPointer)XtInputExceptMask, IvyXtHandleChannelDelete, channel);

	return channel;
}
void IvyMainLoop(void(*hook)(void))
{
	XtAppMainLoop (app);
}