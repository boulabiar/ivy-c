/*
 *	Ivy, C interface
 *
 *	Copyright (C) 1997-2006
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
/* Module de gestion de la syntaxe des messages Ivy */

typedef struct _binding *IvyBinding;

/* Mise en place des Filtrages */
void IvyBindingSetFilter( int argc, const char ** argv );
int IvyBindingFilter( const char *expression );

/* Creation, Compilation */
IvyBinding IvyBindingCompile( const char *expression, int *erroffset, const char **errmessage );
void IvyBindingFree( IvyBinding bind );

/* Execution , extraction */
int IvyBindingExec( IvyBinding bind, const char * message );
void IvyBindingMatch( IvyBinding bind, const char *message, int argnum, int *arglen, const char **arg );
