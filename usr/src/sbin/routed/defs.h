/*	defs.h	4.15	84/08/29	*/

/*
 * Internal data structure definitions for
 * user routing process.  Based on Xerox NS
 * protocol specs with mods relevant to more
 * general addressing scheme.
 */
#include <sys/types.h>
#include <sys/socket.h>

#include <net/route.h>
#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>

#include "protocol.h"
#include "trace.h"
#include "interface.h"
#include "table.h"
#include "af.h"

/* #define	COMPAT	1		/* stay compatible with 4.1a daemons */

/*
 * When we find any interfaces marked down we rescan the
 * kernel every CHECK_INTERVAL seconds to see if they've
 * come up.
 */
#define	CHECK_INTERVAL	(1*60)

#define	LOOPBACKNET	0177
#define equal(a1, a2) \
	(bcmp((caddr_t)(a1), (caddr_t)(a2), sizeof (struct sockaddr)) == 0)
#define	min(a,b)	((a)>(b)?(b):(a))

struct	sockaddr_in addr;	/* address of daemon's socket */

int	s;			/* source and sink of all data */
int	kmem;
int	supplier;		/* process should supply updates */
int	install;		/* if 1 call kernel */
int	lookforinterfaces;	/* if 1 probe kernel for new up interfaces */
int	performnlist;		/* if 1 check if /vmunix has changed */
int	externalinterfaces;	/* # of remote and local interfaces */
int	timeval;		/* local idea of time */

char	packet[MAXPACKETSIZE+1];
struct	rip *msg;

char	**argv0;
struct	servent *sp;

extern	char *sys_errlist[];
extern	int errno;

struct	in_addr inet_makeaddr();
int	inet_addr();
char	*malloc();
int	exit();
int	sendmsg();
int	supply();
int	timer();
int	cleanup();
