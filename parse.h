enum connenum {
    CONN_SEQ,  /* sequential commands, i.e. separated by a semicolon */
    CONN_AND,  /* commands joined by '&&' */
    CONN_OR    /* commands joined by '||' */
};

struct pipeline {  /* list of '|'-connected commands */
    char **argv;  /* array ending with NULL */
    struct pipeline *next;  /* NULL if this doesn't pipe into anything */
    int isdouble; /* 1 if we have '|&' i.e. should dup onto 2 */
};

struct parsed_line { /* list of ';' or '&&' or '||' -connected struct pipelines */
    enum connenum conntype;  /* will be CONN_SEQ if this is the first item */
    char *inputfile, *outputfile;  /* NULL for no redirection */
    int output_is_double;  /* output redirection is '>&' rather than '>' */
    struct pipeline *pl;  /* the command(s) */
    int isbg;  /* non-zero iff there is a '&' after this command */
    struct parsed_line *next;   /* connected as specified by next->conntype */
};


extern struct parsed_line *parse(char *s);
extern void freeparse(struct parsed_line *p);
