/*
 *	Ivy, C interface
 *
 *	Copyright (C) 1997-2000
 *	Centre d'�tudes de la Navigation A�rienne
 *
 * 	Main loop handling around select
 *
 *	Authors: Fran�ois-R�gis Colin <fcolin@cena.dgac.fr>
 *		 St�phane Chatty <chatty@cena.dgac.fr>
 *
 *	$Id$
 * 
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 *
 */

#ifndef IVYLOOP_H
#define IVYLOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ivychannel.h"

/* general Handle */

#define ANYPORT	0

#ifdef WIN32
#include <windows.h>
#define HANDLE SOCKET
#else
#define HANDLE int
#endif



extern void IvyMainLoop(void(*hook)(void) );


extern void IvyChannelInit(void);
extern void IvyChannelClose( Channel channel );
extern Channel IvyChannelSetUp(
			HANDLE fd,
			void *data,
			ChannelHandleDelete handle_delete,
			ChannelHandleRead handle_read);



#ifdef __cplusplus
}
#endif

#endif

