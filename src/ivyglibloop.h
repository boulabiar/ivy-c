/*
*	Ivy, C interface
*
*	Copyright (C) 1997-2000
*	Centre d'�tudes de la Navigation A�rienne
*
* 	Main loop based on the GTK Toolkit
*
*	Authors: Fran�ois-R�gis Colin <fcolin@cena.fr>
*
*	$Id$
* 
*	Please refer to file version.h for the
*	copyright notice regarding this software
*/

#ifndef IVYGLIBLOOP_H
#define IVYGLIBLOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#define ANYPORT	0

#ifdef WIN32
#include <windows.h>
#define HANDLE SOCKET
#else
#define HANDLE int
#endif

#include "ivychannel.h"

extern void IvyGlibChannelInit(void);

extern Channel IvyGlibChannelSetUp(
				   HANDLE fd,
				   void *data,
				   ChannelHandleDelete handle_delete,
				   ChannelHandleRead handle_read
				   );

extern void IvyGlibChannelClose( Channel channel );

#ifdef __cplusplus
}
#endif

#endif /* IVYGLIBLOOP_H */
