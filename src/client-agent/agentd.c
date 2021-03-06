/* @(#) $Id$ */

/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

/* Part of the OSSEC HIDS
 * Available at http://www.ossec.net/hids/
 */


#include "shared.h"
#include "agentd.h"

#include "os_net/os_net.h"



/* AgentdStart v0.2, 2005/11/09
 * Starts the agent daemon.
 */
void AgentdStart(char *dir, int uid, int gid, char *user, char *group)
{
    int rc = 0;
    int pid = 0;
    int maxfd = 0;   

    fd_set fdset;
    
    struct timeval fdtimeout;

    
    /* Going daemon */
    pid = getpid();
    available_server = 0;
    nowDaemon();
    goDaemon();

    
    /* Setting group ID */
    if(Privsep_SetGroup(gid) < 0)
        ErrorExit(SETGID_ERROR, ARGV0, group);

    
    /* chrooting */
    if(Privsep_Chroot(dir) < 0)
        ErrorExit(CHROOT_ERROR, ARGV0, dir);

    
    nowChroot();


    if(Privsep_SetUser(uid) < 0)
        ErrorExit(SETUID_ERROR, ARGV0, user);


    /* Create the queue. In this case we are going to create
     * and read from it
     * Exit if fails.
     */
    if((logr->m_queue = StartMQ(DEFAULTQUEUE, READ)) < 0)
        ErrorExit(QUEUE_ERROR, ARGV0, DEFAULTQUEUE, strerror(errno));

    maxfd = logr->m_queue;
    logr->sock = -1;
    


    /* Creating PID file */	
    if(CreatePID(ARGV0, getpid()) < 0)
        merror(PID_ERROR,ARGV0);


    /* Reading the private keys  */
    verbose(ENC_READ, ARGV0);
        
    OS_ReadKeys(&keys);
    OS_StartCounter(&keys);
    os_write_agent_info(keys.keyentries[0]->name, NULL, keys.keyentries[0]->id);


    /* Start up message */
    verbose(STARTUP_MSG, ARGV0, (int)getpid());

    
    /* Initial random numbers */
    #ifdef __OpenBSD__
    srandomdev();
    #else
    srandom( time(0) + getpid()+ pid + getppid());
    #endif
                    
    random();


    /* Connecting UDP */
    rc = 0;
    while(rc < logr->rip_id)
    {
        verbose("%s: INFO: Server IP Address: %s", ARGV0, logr->rip[rc]);
        rc++;
    }


    /* Trying to connect to the server */
    if(!connect_server(0))
    {
        ErrorExit(UNABLE_CONN, ARGV0);
    }
    

    /* Setting max fd for select */
    if(logr->sock > maxfd)
    {
        maxfd = logr->sock;
    }


    /* Connecting to the execd queue */
    if(logr->execdq == 0)
    {
        if((logr->execdq = StartMQ(EXECQUEUE, WRITE)) < 0)
        {
            merror("%s: INFO: Unable to connect to the active response "
                   "queue (disabled).", ARGV0);
            logr->execdq = -1;
        }
    }



    /* Trying to connect to server */
    os_setwait();

    start_agent(1);
    
    os_delwait();


    /* Sending integrity message for agent configs */
    intcheck_file(OSSECCONF, dir);
    intcheck_file(OSSEC_DEFINES, dir);

   
    /* Sending first notification */
    run_notify();
    
     
    /* Maxfd must be higher socket +1 */
    maxfd++;
    
    
    /* monitor loop */
    while(1)
    {
        /* Monitoring all available sockets from here */
        FD_ZERO(&fdset);
        FD_SET(logr->sock, &fdset);
        FD_SET(logr->m_queue, &fdset);

        fdtimeout.tv_sec = 120;
        fdtimeout.tv_usec = 0;

        
        /* Wait for 120 seconds at a maximum for any descriptor */
        rc = select(maxfd, &fdset, NULL, NULL, &fdtimeout);
        if(rc == -1)
        {
            ErrorExit(SELECT_ERROR, ARGV0);
        }
       
        
        else if(rc == 0)
        {
            continue;
        }    

        
        /* For the receiver */
        if(FD_ISSET(logr->sock, &fdset))
        {
            receive_msg();
        }

        
        /* For the forwarder */
        if(FD_ISSET(logr->m_queue, &fdset))
        {
            EventForward();
        }
    }
}



/* EOF */
