/*
 *	Ivy, C interface
 *
 *	Copyright (C) 1997-2000
 *	Centre d'�tudes de la Navigation A�rienne
 *
 *	Bind syntax for extracting message comtent 
 *  using regexp or other 
 *
 *	Authors: Fran�ois-R�gis Colin <fcolin@cena.fr>
 *
 *	$Id$
 * 
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 */
#include "ivyargument.h"

/* Module de gestion de la syntaxe des messages Ivy */

typedef struct _binding *IvyBinding;

typedef enum  { IvyBindRegexp, IvyBindSimple } IvyBindingType;
void IvyBindingParseMessage( const char *msg );
void IvyBindingSetFilter( int argc, const char ** argv );
int IvyBindingFilter( IvyBindingType typ, int len, const char *exp );
IvyBinding IvyBindingCompile( IvyBindingType typ, const char * expression );
void IvyBindingGetCompileError( int *erroffset, const char **errmessage );
void IvyBindingFree( IvyBinding bind );
int IvyBindingExec( IvyBinding bind, const char * message );
IvyArgument IvyBindingMatch( IvyBinding bind, const char *message );
