#define LIST_ITER( list, p, cond ) \
	p = list; \
	while ( p && (cond) ) p = p->next 

#define LIST_REMOVE( list, p ) \
	{ \
	void  *toRemove; \
	if ( list == p ) \
		{ \
		list = p->next; \
		free(p);\
		} \
		else \
		{\
		toRemove = p;\
		LIST_ITER( list, p, ( p->next != toRemove ));\
		if ( p )\
			{\
			/* somme tricky swapping to use a untyped variable */\
			void *suiv; \
			void *prec = p;\
			p = toRemove;\
			suiv = p->next;\
			p = prec;\
			p->next = suiv;\
			free(toRemove);\
			}\
		} \
	}

#define LIST_ADD(list, p ) \
	if ((p = malloc( sizeof( *p ))))\
		{ \
		memset( p, 0 , sizeof( *p ));\
		p->next = list; \
		list = p; \
		} 

#define LIST_EACH( list, p ) \
	for ( p = list ; p ; p = p -> next )

#define LIST_EACH_SAFE( list, p, next )\
for ( p = list ; (next = p ? p->next: p ),p ; p = next )


#define LIST_EMPTY( list ) \
	{ \
	void *p; \
	while( list ) \
		{ \
		p = list;\
		list = list->next; \
		free(p);\
		} \
	}