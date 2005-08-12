
/*
 * \ingroup Ivy
 *	Ivy, C interface
 *
 *	Copyright (C) 1997-2000
 *	Centre d'�tudes de la Navigation A�rienne
 *
 * 	Main functions
 *
 *	\author Authors: Fran�ois-R�gis Colin <fcolin@cena.fr>
 *		 St�phane Chatty <chatty@cena.fr>
 *
 *	$Id$
 * 
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 * \todo 
 *	many things TODO
 * \bug 
 *  many introduced 
 */

#ifndef IVY_H
#define IVY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "IvyArgument.h"

/* numero par default du bus */

#define DEFAULT_BUS 2010
#define DEFAULT_PRIORITY 100;

typedef struct _clnt_lst *IvyClientPtr;

typedef enum { IvyApplicationConnected, IvyApplicationDisconnected } IvyApplicationEvent;
typedef enum { IvyAddBind, IvyRemoveBind } IvyBindEvent;

extern void IvyDefaultApplicationCallback( IvyClientPtr app, void *user_data, IvyApplicationEvent event ) ;
extern void IvyDefaultBindCallback( IvyClientPtr app, void *user_data, IvyBindEvent event, char* regexp ) ;


/* callback callback appele sur connexion deconnexion d'une appli */
typedef void (*IvyApplicationCallback)( IvyClientPtr app, void *user_data, IvyApplicationEvent event ) ;

/* callback callback appele sur ajout ou suppression d'un bind */
typedef void (*IvyBindCallback)( IvyClientPtr app, void *user_data, IvyBindEvent event, char* regexp ) ;

/* callback appele sur reception de die */
typedef void (*IvyDieCallback)( IvyClientPtr app, void *user_data, int id ) ;

/* callback appele sur reception de messages normaux */
typedef void (*MsgCallback)( IvyClientPtr app, void *user_data, IvyArgument args ) ;

/* callback appele sur reception de messages directs */
typedef void (*MsgDirectCallback)( IvyClientPtr app, void *user_data, int id, int len, void *msg ) ;

/* identifiant d'une expression reguliere ( Bind/Unbind ) */
typedef struct _msg_rcv *MsgRcvPtr;

/* filtrage des regexps */
void IvySetMyMessagesStart( int argc, const char **argv);

/**
 *
 * \param AppName 
 * \param ready 
 * \param callback 
 * \param data 
 * \param die_callback 
 * \param die_data 
 */
void IvyInit(
	 const char * AppName,		/* nom de l'application */
	 const char * ready,		/* ready Message peut etre NULL */
	 IvyApplicationCallback callback, /* callback appele sur connection deconnection d'une appli */
	 void * data,			/* user data passe au callback */
	 IvyDieCallback die_callback,	/* last change callback before die */
	 void * die_data 		/* user data */
	 );

/**
 *
 * \param priority prioritie de traitement des clients
 */
void IvySetApplicationPriority( int priority );
void IvySetBindCallback( IvyBindCallback bind_callback, void *bind_data ); 
/**
 *
 * \param bus 
 */
void IvyStart (const char* bus);
void IvyStop ();

/* query sur les applications connectees */
char *IvyGetApplicationName( IvyClientPtr app );
char *IvyGetApplicationHost( IvyClientPtr app );
char *IvyGetApplicationId( IvyClientPtr app );
IvyClientPtr IvyGetApplication( char *name );
char ** IvyGetApplicationList();
char **IvyGetApplicationMessages( IvyClientPtr app); /* demande de reception d'un message */

MsgRcvPtr IvyBindMsg( MsgCallback callback, void *user_data, const char *fmt_regexp, ... ); /* avec sprintf prealable */
void IvyUnbindMsg( MsgRcvPtr id );

MsgRcvPtr IvyBindSimpleMsg( MsgCallback callback, void *user_data, const char *fmt_regexp, ... ); /* avec sprintf prealable */

/* emission d'un message d'erreur */
void IvySendError( IvyClientPtr app, int id, const char *fmt, ... );

/* emmission d'un message die pour terminer l'application */
void IvySendDieMsg( IvyClientPtr app );

/* emission d'un message retourne le nb effectivement emis */

int IvySendMsg( const char *fmt_message, ... );		/* avec sprintf prealable */

/* Message Direct Inter-application */

void IvyBindDirectMsg( MsgDirectCallback callback, void *user_data);
void IvySendDirectMsg( IvyClientPtr app, int id, int len, void *msg );

/* boucle principale d'Ivy */
/* use of internal MainLoop or XtMainLoop, or other MainLoop integration */
extern void IvyMainLoop(void(*hook)(void));

#ifdef __cplusplus
}
#endif

#endif
