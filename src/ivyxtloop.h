/*
 *	Ivy, C interface
 *
 *      Copyright (C) 1997-1999
 *      Centre d'�tudes de la Navigation A�rienne
 *
 * 	Main loop based on X Toolkit
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
#ifndef _IVYXTLOOP_H
#define _IVYXTLOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/Intrinsic.h>

/* general Handle */

#define ANYPORT	0

#ifdef WIN32
#include <windows.h>
#define HANDLE SOCKET
#else
#define HANDLE int
#endif

#include "ivychannel.h"

extern void IvyXtChannelInit(void);

extern Channel IvyXtChannelSetUp(
		HANDLE fd,
		void *data,
		ChannelHandleDelete handle_delete,
		ChannelHandleRead handle_read
);

extern void IvyXtChannelClose( Channel channel );

extern void IvyXtChannelAppContext( XtAppContext cntx );

#ifdef __cplusplus
}
#endif

#endif

