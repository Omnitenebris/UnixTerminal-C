/*
 * error.c - feeble error handling for a feeble shell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"


/*
 * This is extremely cheesy, but it simplifies the program considerably in some
 * cases, along the lines of things which are not the point right now.
 *
 * DO NOT USE THIS FOR code which should correctly handle user error,
 * e.g. command not found.  This is used for out-of-memory errors and similar
 * things which we don't expect to happen in this toy environment.
 */
void fatal(char *err)
{
    fflush(stdout);
    fprintf(stderr, "fatal error: %s\n", err);
    exit(1);
}


/* malloc() which never returns NULL */
void *emalloc(unsigned size)
{
    void *p = malloc(size);
    if (p == NULL)
        fatal("out of memory!");  /* not very likely; and see "cheesy", above */
    return(p);
}


/* strsave by length, and never fail */
char *estrsavelen(char *s, int len)
{
    char *p = emalloc(len + 1);
    memcpy(p, s, len);
    p[len] = '\0';
    return(p);
}


/* duplicate an argv, adding NULL, and never fail */
char **eargvsave(char **argv, int argc)
{
    char **p = emalloc(sizeof(char *) * (argc + 1));
    int i;
    for (i = 0; i < argc; i++)
        p[i] = argv[i];
    p[i] = NULL;
    return(p);
}


/*
 * Put together a directory name and a file base name, with error checking.
 * Note that the return value is only valid until the next call to
 * efilenamecons().
 */
char *efilenamecons(char *s, char *t)
{
    static char buf[1000];
    if (strlen(s) + 1 + strlen(t) + 1 > sizeof buf)
        fatal("buffer exceeded in efilenamecons()");
    sprintf(buf, "%s/%s", s, t);
    return(buf);
}
