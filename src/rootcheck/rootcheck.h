/*   $OSSEC, syscheck.h, v0.1, 2005/07/29, Daniel B. Cid$   */

/* Copyright (C) 2003,2004,2005 Daniel B. Cid <dcid@ossec.net>
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */
       

#ifndef __ROOTCHECK_H

#define __ROOTCHECK_H

#include <stdio.h>

typedef struct _config
{
    char *workdir;
    char *rootkit_files;
    char *rootkit_trojans;

    FILE *fp;
    int daemon;
    int notify; /* QUEUE or SYSLOG */

    int queue;
}config;


config rootcheck;

#define QUEUE   101
#define SYSLOG  102

#define MAX_DIR_SIZE    64


/** Prototypes **/

/* common is_file: Check if a file exist (using stat and fopen) */
int is_file(char *file_name);

/* Check if regex is present on the file.
 * Similar to `strings fiile | grep -r regex`
 */ 
int os_string(char *file, char *regex);


/* run_rk_check: checks the integrity of the files against the
 * saved database
 */
void run_rk_check();

/* start_rk_daemon: Runs run_rk_check periodically.
 */
void start_rk_daemon();


/*** Plugins prototypes ***/
void check_rc_files(char *basedir, FILE *fp);

void check_rc_trojans(char *basedir, FILE *fp);

void check_rc_dev(char *basedir);

void check_rc_sys(char *basedir);

void check_rc_pids();

void check_rc_ports();

#endif

/* EOF */