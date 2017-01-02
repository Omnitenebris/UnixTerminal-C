/*
 * parse.c - feeble command parsing for the Feeble SHell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "parse.h"
#include "error.h"


#define MAXARGV 1000

enum token {
    identifier, directin, directout, doubledirectout,
    /* everything >= semicolon ends an individual "struct pipeline" */
    semicolon,
    ampersand,
    verticalbar, doubleampersand, doublebar, doublepipe,
    eol
};
static enum token gettoken(char **s, char **argp);
static char *ptok(enum token tok);


struct parsed_line *parse(char *s)
{
    struct parsed_line *retval;  /* remains freeparse()able at all times */
    struct parsed_line *curline;
    struct pipeline **plp;  /* where to append for '|' and '|||' */
    char *argv[MAXARGV];
    enum token tok;
    int argc = 0;
    int isdouble = 0;

    retval = curline = emalloc(sizeof(struct parsed_line));
    curline->conntype = CONN_SEQ;  /* i.e. always do this first command */
    curline->inputfile = curline->outputfile = NULL;
    curline->output_is_double = 0;
    curline->isbg = 0;
    curline->pl = NULL;
    curline->next = NULL;
    plp = &(curline->pl);

    do {
        if (argc >= MAXARGV)
            fatal("argv limit exceeded");
        while ((tok = gettoken(&s, &argv[argc])) < semicolon) {
	    switch ((int)tok) {  /* cast prevents stupid warning message about
				  * not handling all enum token values */
	    case identifier:
		argc++;  /* it's already in argv[argc];
		          * increment to represent a save */
		break;
	    case directin:
		if (curline->inputfile) {
		    fprintf(stderr,
			    "syntax error: multiple input redirections\n");
		    freeparse(curline);
		    return(NULL);
		}
		if (gettoken(&s, &curline->inputfile) != identifier) {
		    fprintf(stderr, "syntax error in input redirection\n");
		    freeparse(curline);
		    return(NULL);
		}
		break;
	    case doubledirectout:
		curline->output_is_double = 1;
		/* fall through */
	    case directout:
		if (curline->outputfile) {
		    fprintf(stderr,
			    "syntax error: multiple output redirections\n");
		    freeparse(curline);
		    return(NULL);
		}
		if (gettoken(&s, &curline->outputfile) != identifier) {
		    fprintf(stderr, "syntax error in output redirection\n");
		    freeparse(curline);
		    return(NULL);
		}
		break;
	    }
	}

	/* cons up just-parsed pipeline component */
	if (argc) {
	    *plp = emalloc(sizeof(struct pipeline));
	    (*plp)->next = NULL;
	    (*plp)->argv = eargvsave(argv, argc);
	    (*plp)->isdouble = isdouble;
	    plp = &((*plp)->next);
	    isdouble = 0;
	    argc = 0;
	} else if (tok != eol) {
	    fprintf(stderr, "syntax error: null command before `%s'\n",
		    ptok(tok));
	    freeparse(curline);
	    return(NULL);
	}

	/* ampersanded? */
	if (tok == ampersand)
	    curline->isbg = 1;

	/* is this a funny kind of pipe (to the right)? */
	if (tok == doublepipe)
	    isdouble = 1;

	/* does this start a new struct parsed_line? */
	if (tok == semicolon || tok == ampersand || tok == doubleampersand || tok == doublebar) {
	    curline->next = emalloc(sizeof(struct parsed_line));
	    curline = curline->next;

	    curline->conntype =
		(tok == semicolon || tok == ampersand) ? CONN_SEQ
		: (tok == doubleampersand) ? CONN_AND
		: CONN_OR;
	    curline->inputfile = curline->outputfile = NULL;
	    curline->output_is_double = 0;
	    curline->isbg = 0;
	    curline->pl = NULL;
	    curline->next = NULL;
	    plp = &(curline->pl);
	}

    } while (tok != eol);
    return(retval);
}


/* (*s) is advanced as we scan; *argp is set iff retval == identifier */
static enum token gettoken(char **s, char **argp)
{
    char *p;

    while (**s && isascii(**s) && isspace(**s))
        (*s)++;
    switch (**s) {
    case '\0':
        return(eol);
    case '<':
        (*s)++;
        return(directin);
    case '>':
        (*s)++;
	if (**s == '&') {
	    (*s)++;
	    return(doubledirectout);
	}
        return(directout);
    case ';':
        (*s)++;
        return(semicolon);
    case '|':
	if ((*s)[1] == '|') {
	    *s += 2;
	    return(doublebar);
	}
        (*s)++;
	if (**s == '&') {
	    (*s)++;
	    return(doublepipe);
	}
        return(verticalbar);
    case '&':
	if ((*s)[1] == '&') {
	    *s += 2;
	    return(doubleampersand);
	} else {
	    (*s)++;
	    return(ampersand);
	}
    /* else identifier */
    }

    /* it's an identifier */
    /* find the beginning and end of the identifier */
    p = *s;
    while (**s && isascii(**s) && !isspace(**s) && !strchr("<>;&|", **s))
        (*s)++;
    *argp = estrsavelen(p, *s - p);
    return(identifier);
}


static char *ptok(enum token tok)
{
    switch (tok) {
    case directin:
	return("<");
    case directout:
	return(">");
    case semicolon:
	return(";");
    case verticalbar:
	return("|");
    case ampersand:
	return("&");
    case doubleampersand:
	return("&&");
    case doublebar:
	return("||");
    case doubledirectout:
	return(">&");
    case doublepipe:
	return("|&");
    case eol:
	return("end of line");
    default:
	return(NULL);
    }
}


static void freepipeline(struct pipeline *pl)
{
    if (pl) {
	char **p;
        for (p = pl->argv; *p; p++)
            free(*p);
        free(pl->argv);
        freepipeline(pl->next);
        free(pl);
    }
}


void freeparse(struct parsed_line *p)
{
    if (p) {
	freeparse(p->next);
	if (p->inputfile)
	    free(p->inputfile);
	if (p->outputfile)
	    free(p->outputfile);
	freepipeline(p->pl);
	free(p);
    }
}
