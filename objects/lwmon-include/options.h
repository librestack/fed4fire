/* SPDX-License-Identifier: MIT
 *
 * functions to parse command-line options and configuration files
 * for lwmon and awooga
 *
 * this file is part of LWMON
 *
 * Copyright (c) 2010-2022 Claudio Calvelli <clc@librecast.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __LWMON_OPTIONS_H__
#define __LWMON_OPTIONS_H__ 1

/* type of option argument */
typedef enum {
    LWMON_OPT_END,      /* end of option array */
    LWMON_OPT_TITLE,    /* not an option, printed when providing help */
    LWMON_OPT_INT,      /* integer */
    LWMON_OPT_STRING,   /* string */
    LWMON_OPT_IO,       /* absolute path, or "-" for stdin/out */
    LWMON_OPT_FILE,     /* absolute path, no stdin/out */
    LWMON_OPT_DIR,      /* absolute path, points to directory */
    LWMON_OPT_FUNC,     /* call function to parse option, which must
			 * return negative on error, and the number of
			 * extra arguments it consumed if OK (normally
			 * 0, meaning it just used the string passed to
			 * it, but it could be positive) */
    LWMON_OPT_NONE      /* no option, set integer argument to 1 */
} lwmon_opttype_t;

/* option data, element selected depends on type */
typedef union {
    int * i;            /* integer argument for INT, NONE (, FUNC) */
    const char ** s;    /* string argument for STRING, IO, FILE, DIR (, FUNC) */
    void * v;           /* available to FUNC unless it uses .i or .s */
} lwmon_optdata_t;

/* protocols for generic lwmon_check_port function; also used by os-dependent
 * network lookup functions */
typedef enum {
    LWMON_PROTO_TCP4 = 1,
    LWMON_PROTO_TCP6 = 2,
    LWMON_PROTO_TCP  = 3,
    LWMON_PROTO_UDP4 = 4,
    LWMON_PROTO_UDP6 = 8,
    LWMON_PROTO_UDP  = 12
} lwmon_protocol_t;

/* element of an option array */
typedef struct {
    char optname;
    const char * argname;
    lwmon_opttype_t argtype;
    int (*func)(char, const char *, int, char *[], lwmon_optdata_t);
    lwmon_optdata_t data;
    const char * helptext;
} lwmon_opts_t;

/* type of help text */
typedef enum {
    LWMON_OH_END,       /* end of help array */
    LWMON_OH_SPACE,     /* produce a blank line in help output */
    LWMON_OH_PARA,      /* format a paragraph */
    LWMON_OH_INDENT     /* indented paragraph */
} lwmon_opthelpmode_t;

/* element of option help array */
typedef struct {
    lwmon_opthelpmode_t mode;
    const char * line;
} lwmon_opthelp_t;

/* structure containing parsed configuration data; which elements are
 * filled depends on the configuration */
typedef struct {
    char * sp;
    int iv;
    void * vp; /* reserved for custom checks, not used by library */
} lwmon_parsedata_t;

/* functions to parse command-line options */

/* lwmon_initops must be called before any other functions, providing
 * the argc/argv argument from main() */
void lwmon_initops(int, char *[]);

/* parse options and store arguments as appropriate; returns 1 after
 * parsing all options, 0 if there is an error */
int lwmon_opts(lwmon_opts_t*[]);

/* after running lwmon_opts there may be non-option arguments;
 * lwmon_argcount returns the number of such arguments;
 * lwmon_firstarg returns the first argument (or NULL if there are none)
 * lwmon_nextarg returns the next argument (NULL if there are no more arguments)
 * lwmon_firstarg must be called before calling lwmon_nextarg the first time */
int lwmon_argcount(void);
char * lwmon_firstarg(void);
char * lwmon_nextarg(void);

/* functions to print short and long usage; the first argument is normally
 * stderr or stdout and the second argument is the one-line description
 * of non-option arguments (description for option arguments is derived
 * from the options array) */
void lwmon_shortusage(FILE *, const char *);
void lwmon_usage(FILE *, const char *, lwmon_opts_t *const[],
		 lwmon_opthelp_t *const[]);

/* functions to parse a configuration file */

/* a configuration file line is first split into fields, a keyword which
 * determines how to parse the rest of the line, and zero or more arguments;
 * each argument is parsed by calling a "check" function to fill an array
 * of data, and at the end a "store" function */

/* type for the "check" function */
typedef int (*lwmon_check_t)(const char *, int,
			     const char *, lwmon_parsedata_t *);

/* "check" functions for common types of data: they all return a positive
 * number if OK or 0 on error */

/* data is a string, with no special syntax; it will be copied to the "sp"
 * argument of the result with the length in "iv"; it may produce errors if
 * it runs out of memory to allocate buffers */
int lwmon_check_string(const char *, int, const char *, lwmon_parsedata_t *);

/* data is a string but must point to an absolute path */
int lwmon_check_file(const char *, int, const char *, lwmon_parsedata_t *);

/* same as lwmon_check_file but allows a "-" to mean standard output */
int lwmon_check_file_or_stdout(const char *, int, const char *, lwmon_parsedata_t *);

/* data is a string but must point to an absolute path resolving to
 * an executable object */
int lwmon_check_program(const char *, int, const char *, lwmon_parsedata_t *);

/* data is a number which will be stored in the "iv" element of the result */
int lwmon_check_int(const char *, int, const char *, lwmon_parsedata_t *);

/* data is a number at least 1, indicating a check frequency as the number
 * of seconds between consecutive checks */
int lwmon_check_freq(const char *, int, const char *, lwmon_parsedata_t *);

/* protocol name: tcp, tcp4, tcp6, udp, udp4, udp6; stores a lwmon_protocol_t
 * value in the "iv" element */
int lwmon_check_proto(const char *, int, const char *, lwmon_parsedata_t *);

/* port number of service name; this function cannot be used directly in a
 * configuration file specification (it's the wrong type) but it's provided
 * as a helper to build one: the argument it checks needs to either follow
 * one parsed by lwmon_check_proto or have an implicit protocol */
int lwmon_check_port(lwmon_protocol_t, const char *, lwmon_parsedata_t *);

/* functions to handle a conditional within a configuration file */

/* configuration files can include conditional sections which are parsed
 * but will only result in configuration being stored if the condition is
 * true; conditions can be nested */

/* type for a "condition" function, which will be called from the "store"
 * function of a conditional; it will be passed the number of parsed elements,
 * an array of such elements, and an integer argument from the configutation
 * specification; returns 1 for true and 0 for false */
typedef int (*lwmon_cond_check_t)(int, const lwmon_parsedata_t *, int);

/* defines a particular condition function and the integer argument passed
 * to it when called; this allows the same condition function to be used for
 * different conditionals if most of the code is the same */
typedef struct {
    const char * name;
    lwmon_cond_check_t cond;
    int flags;
} lwmon_condition_t;

/* opaque type to store the result of evaluating conditions, including
 * nesting etc */
typedef struct lwmon_condstore_s lwmon_condstore_t;

/* takes an array of condition functions and a name, and finds a function
 * with the name given; returns the number of the element in the array, or
 * -1 if not found; the array needs to be terminated by an element whose name
 *  is NULL */
int lwmon_cond_find(const lwmon_condition_t[], const char *);

/* determines if a conditional evaluated to true; usually a "store" function
 * will start with code like:
 * if (! lwmon_if(cond)) return 1;
 * to avoid storing data inside a false conditional; however if the "store"
 * function does any extra parsing or checking, it can do that before checking
 * the conditional */
int lwmon_if(const lwmon_condstore_t *);

/* helper functions to build the "store" function for conditionals; the
 * "if" and "elsif" functions need to first determine the condition
 * function they need, and pass it as first argument; the remaining
 * arguments are the same as provided to a normal "store" function,
 * so the "else" and "endif" functions can be used directly as "store" */
int lwmon_cond_if(const lwmon_condition_t *, const char *, int,
		  lwmon_parsedata_t *, lwmon_condstore_t *);
int lwmon_cond_elsif(const lwmon_condition_t *, const char *, int,
		     lwmon_parsedata_t *, lwmon_condstore_t *);
int lwmon_cond_else(const char *, int,
		    lwmon_parsedata_t *, lwmon_condstore_t *);
int lwmon_cond_endif(const char *, int,
		     lwmon_parsedata_t *, lwmon_condstore_t *);

/* checks if there is a file matching any of the patterns specified in the
 * "sp" field of any of the data; if found, return 1 + the index of the
 * pattern which matched; if none found, return 0;
 * if the third argument is nonzero, empty files are ignored;
 * the two functions differ only in the way the patterns are provided */
int lwmon_has_file(int, const lwmon_parsedata_t *, int);
int lwmon_has_file1(int, const char *[], int);

/* checks if the local hostname matches any of the patterns specified in
 * the "sp" field of any of the data, returns 1 if so, 0 if none match */
int lwmon_host_is(int, const lwmon_parsedata_t *, int);

/* maximum number of arguments which have their own separate "check" function */
#define MAX_CHECKS 10

/* structure to hold the definition for a single configuration item */
typedef struct {
    /* keyword - this is matched against the first field in the line */
    const char * kw;
    /* minimum number of arguments; if the line has fewer than this,
     * it will produce an error */
    int minargs;
    /* maximum number of arguments which have their own separate check
     * function: maxargs must be less than or equal to MAX_CHECKS;
     * if the repeat_check field is NULL, this is also the maximum number
     * of arguments, and it will be an error if there are more; if
     * repeat_check is not NULL it is not an error to have maxargs
     * less than minargs */
    int maxargs;
    /* store function, taking all the parsed arguments and doing the
     * appropriate action */
    int (*store)(const char *, int, lwmon_parsedata_t *, lwmon_condstore_t *);
    /* function to check and parse arguments after the maxargs-th one */
    lwmon_check_t repeat_check;
    /* functions to check and parse the first maxargs arguments */
    lwmon_check_t check[MAX_CHECKS];
} lwmon_parse_t;

/* functions to parse a configuration file line, a whole file, or a
 * directory containing configuration files; they all return 1 if OK
 * or 0 on error; they all take a NULL-terminated list of lwmon_parse_t
 * arrays, to make it easier for modules to add their own configuration */
int lwmon_parse_line(const char *, const lwmon_parse_t *[]);
int lwmon_parse_file(const char *, const lwmon_parse_t *[]);
int lwmon_parse_dir(const char *, const lwmon_parse_t *[]);

#endif /* __LWMON_OPTIONS_H__ */
