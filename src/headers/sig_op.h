/* @(#) $Id$ */

/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


/* Functions to handle signal manipulation
 */

#ifndef __SIG_H

#define __SIG_H

void HandleSIG();
void HandleSIGPIPE();

/* Start signal manipulation */
void StartSIG(char *process_name);

/* Start signal manipulation -- function as an argument */
void StartSIG2(char *process_name, void (*func)(int));

#endif
