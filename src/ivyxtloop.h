/*
 *	Ivy, C interface
 *
 *	Copyright (C) 1997-2000
 *	Centre d'�tudes de la Navigation A�rienne
 *
 * 	Main loop based on the X Toolkit
 *
 *	Authors: Fran�ois-R�gis Colin <colin@cena.fr>
 *		 St�phane Chatty <chatty@cena.fr>
 *
 *	$Id$
 * 
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 */

#ifndef IVYXTLOOP_H
#define IVYXTLOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/Intrinsic.h>

#include "ivychannel.h"



extern void IvyXtChannelAppContext( XtAppContext cntx );

#ifdef __cplusplus
}
#endif

#endif

