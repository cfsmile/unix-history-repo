/*-
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_mkquery.c	6.17 (Berkeley) %G%";
static char rcsid[] = "$Id: res_mkquery.c,v 4.9.1.2 1993/05/17 10:00:01 vixie Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <stdio.h>
#include <string.h>

/*
 * Form all types of queries.
 * Returns the size of the result or -1.
 */
res_mkquery(op, dname, class, type, data, datalen, newrr_in, buf, buflen)
	int op;			/* opcode of query */
	const char *dname;		/* domain name */
	int class, type;	/* class and type of query */
	const char *data;		/* resource record data */
	int datalen;		/* length of data */
	const char *newrr_in;	/* new rr for modify or append */
	char *buf;		/* buffer to put query */
	int buflen;		/* size of buffer */
{
	register HEADER *hp;
	register char *cp;
	register int n;
	struct rrec *newrr = (struct rrec *) newrr_in;
	char *dnptrs[10], **dpp, **lastdnptr;

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf(";; res_mkquery(%d, %s, %d, %d)\n",
		       op, dname, class, type);
#endif
	/*
	 * Initialize header fields.
	 */
	if ((buf == NULL) || (buflen < sizeof(HEADER)))
		return(-1);
	bzero(buf, sizeof(HEADER));
	hp = (HEADER *) buf;
	hp->id = htons(++_res.id);
	hp->opcode = op;
	hp->pr = (_res.options & RES_PRIMARY) != 0;
	hp->rd = (_res.options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	cp = buf + sizeof(HEADER);
	buflen -= sizeof(HEADER);
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof(dnptrs)/sizeof(dnptrs[0]);
	/*
	 * perform opcode specific processing
	 */
	switch (op) {
	case QUERY:
		if ((buflen -= QFIXEDSZ) < 0)
			return(-1);
		if ((n = dn_comp((u_char *)dname, (u_char *)cp, buflen,
		    (u_char **)dnptrs, (u_char **)lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		__putshort(type, (u_char *)cp);
		cp += sizeof(u_int16_t);
		__putshort(class, (u_char *)cp);
		cp += sizeof(u_int16_t);
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		buflen -= RRFIXEDSZ;
		if ((n = dn_comp((u_char *)data, (u_char *)cp, buflen,
		    (u_char **)dnptrs, (u_char **)lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		__putshort(T_NULL, (u_char *)cp);
		cp += sizeof(u_int16_t);
		__putshort(class, (u_char *)cp);
		cp += sizeof(u_int16_t);
		__putlong(0, (u_char *)cp);
		cp += sizeof(u_int32_t);
		__putshort(0, (u_char *)cp);
		cp += sizeof(u_int16_t);
		hp->arcount = htons(1);
		break;

	case IQUERY:
		/*
		 * Initialize answer section
		 */
		if (buflen < 1 + RRFIXEDSZ + datalen)
			return (-1);
		*cp++ = '\0';	/* no domain name */
		__putshort(type, (u_char *)cp);
		cp += sizeof(u_int16_t);
		__putshort(class, (u_char *)cp);
		cp += sizeof(u_int16_t);
		__putlong(0, (u_char *)cp);
		cp += sizeof(u_int32_t);
		__putshort(datalen, (u_char *)cp);
		cp += sizeof(u_int16_t);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

#ifdef ALLOW_UPDATES
	/*
	 * For UPDATEM/UPDATEMA, do UPDATED/UPDATEDA followed by UPDATEA
	 * (Record to be modified is followed by its replacement in msg.)
	 */
	case UPDATEM:
	case UPDATEMA:

	case UPDATED:
		/*
		 * The res code for UPDATED and UPDATEDA is the same; user
		 * calls them differently: specifies data for UPDATED; server
		 * ignores data if specified for UPDATEDA.
		 */
	case UPDATEDA:
		buflen -= RRFIXEDSZ + datalen;
		if ((n = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		__putshort(type, cp);
                cp += sizeof(u_int16_t);
                __putshort(class, cp);
                cp += sizeof(u_int16_t);
		__putlong(0, cp);
		cp += sizeof(u_int32_t);
		__putshort(datalen, cp);
                cp += sizeof(u_int16_t);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		if ( (op == UPDATED) || (op == UPDATEDA) ) {
			hp->ancount = htons(0);
			break;
		}
		/* Else UPDATEM/UPDATEMA, so drop into code for UPDATEA */

	case UPDATEA:	/* Add new resource record */
		buflen -= RRFIXEDSZ + datalen;
		if ((n = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		__putshort(newrr->r_type, cp);
                cp += sizeof(u_int16_t);
                __putshort(newrr->r_class, cp);
                cp += sizeof(u_int16_t);
		__putlong(0, cp);
		cp += sizeof(u_int32_t);
		__putshort(newrr->r_size, cp);
                cp += sizeof(u_int16_t);
		if (newrr->r_size) {
			bcopy(newrr->r_data, cp, newrr->r_size);
			cp += newrr->r_size;
		}
		hp->ancount = htons(0);
		break;

#endif /* ALLOW_UPDATES */
	}
	return (cp - buf);
}
