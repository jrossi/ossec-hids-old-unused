/* @(#) $Id$ */

/* Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 *
 * License details at the LICENSE file included with OSSEC or 
 * online at: http://www.ossec.net/en/licensing.html
 */



#include "os_win32ui.h"
#include <process.h>
#include "os_win.h"


/* Dialog -- About OSSEC */
BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, 
                           WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_CREATE:
        case WM_INITDIALOG:

            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case UI_ID_CLOSE:
                    EndDialog(hwnd, IDOK);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDOK);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}


/* Main Dialog */
BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    int ret_code = 0;

    
    switch(Message)
    {
        case WM_INITDIALOG:
        {
            int statwidths[] = {130, -1};
            HMENU hMenu, hSubMenu;

            UINT menuflags = MF_STRING;

            if(config_inst.admin_access == 0)
            {
                menuflags = MF_STRING|MF_GRAYED;
            }

            hMenu = CreateMenu();

            /* Creating management menu */
            hSubMenu = CreatePopupMenu();
            AppendMenu(hSubMenu, menuflags, UI_MENU_MANAGE_START,"&Start OSSEC");
            AppendMenu(hSubMenu, menuflags, UI_MENU_MANAGE_STOP,"&Stop OSSEC");
            AppendMenu(hSubMenu, MF_SEPARATOR, UI_MENU_NONE,"");
            AppendMenu(hSubMenu, menuflags, UI_MENU_MANAGE_RESTART,"&Restart");
            AppendMenu(hSubMenu, menuflags, UI_MENU_MANAGE_STATUS,"&Status");
            AppendMenu(hSubMenu, MF_SEPARATOR, UI_MENU_NONE,"");
            AppendMenu(hSubMenu, MF_STRING,UI_MENU_MANAGE_EXIT,"&Exit");
            AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu,"&Manage");

            /* Create view menu */
            hSubMenu = CreatePopupMenu();
            AppendMenu(hSubMenu, MF_STRING, UI_MENU_VIEW_LOGS, "&View Logs");
            AppendMenu(hSubMenu, MF_STRING, UI_MENU_VIEW_CONFIG,"V&iew Config");
            AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu,"&View");

            hSubMenu = CreatePopupMenu();
            AppendMenu(hSubMenu, MF_STRING, UI_MENU_HELP_ABOUT, "A&bout");
            AppendMenu(hSubMenu, MF_STRING, UI_MENU_HELP_HELP, "Help");
            AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&Help");


            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            SetMenu(hwnd, hMenu);


            hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
                    WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP, 
                    0, 0, 0, 0,
                    hwnd, (HMENU)IDC_MAIN_STATUS, 
                    GetModuleHandle(NULL), NULL);

            SendMessage(hStatus, SB_SETPARTS, 
                    sizeof(statwidths)/sizeof(int), 
                    (LPARAM)statwidths);
            SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"http://www.ossec.net");

	    

            /* Initializing config */
            config_read(hwnd);
            gen_server_info(hwnd);


            /* Setting the icons */
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, 
                    (LPARAM)LoadIcon(GetModuleHandle(NULL), 
                                     MAKEINTRESOURCE(IDI_OSSECICON)));
            SendMessage(hwnd, WM_SETICON, ICON_BIG, 
                    (LPARAM)LoadIcon(GetModuleHandle(NULL), 
                                     MAKEINTRESOURCE(IDI_OSSECICON)));

            if(config_inst.admin_access == 0)
            {
                MessageBox(hwnd, "Admin access required. Some features may not work properly. \n\n"
                        "**If on Vista (or Server 2008), choose the \"Run as administrator\" option.",
                        "Admin access required.", MB_OK);
                break;
            }
                                                                                            
        }
        break;

        case WM_COMMAND:
        switch(LOWORD(wParam))
        {
            /* In case of SAVE */
            case IDC_ADD:
            {
                int chd = 0;
                int len;


                if(config_inst.admin_access == 0)
                {
                    MessageBox(hwnd, "Unable to edit configuration. "
                                     "Admin access required.",
                                     "Error Saving.", MB_OK);
                    break;
                }

                /** Getting values from the user (if chosen save) 
                 * We should probably create another function for it...
                 **/
                
                /* Getting server ip */
                len = GetWindowTextLength(GetDlgItem(hwnd, UI_SERVER_TEXT));
                if(len > 0)
                {
                    char *buf;


                    /* Allocating buffer */
                    buf = (char*)GlobalAlloc(GPTR, len + 1);
                    if(!buf)
                    {
                        exit(-1);
                    }
                    
                    GetDlgItemText(hwnd, UI_SERVER_TEXT, buf, len + 1);

                    /* If auth key changed, set it */
                    if(strcmp(buf, config_inst.server) != 0)
                    {
                        if(set_ossec_server(buf, hwnd))
                        {
                            chd = 1;
                        }
                    }
                    else
                    {
                        GlobalFree(buf);
                    }
                }
                
                
                /* Getting auth key */
                len = GetWindowTextLength(GetDlgItem(hwnd, UI_SERVER_AUTH));
                if(len > 0)
                {
                    char *buf;

                    /* Allocating buffer */
                    buf = (char*)GlobalAlloc(GPTR, len + 1);
                    if(!buf)
                    {
                        exit(-1);
                    }

                    GetDlgItemText(hwnd, UI_SERVER_AUTH, buf, len + 1);

                    /* If auth key changed, set it */
                    if(strcmp(buf, config_inst.key) != 0)
                    {
                        int ret;
                        char *tmp_str;
                        char *decd_buf = NULL;
                        char *decd_to_write = NULL;
                        char *id = NULL;
                        char *name = NULL;
                        char *ip = NULL;


                        /* Getting new fields */
                        decd_buf = decode_base64(buf);
                        if(decd_buf)
                        {
                            decd_to_write = strdup(decd_buf);

                            /* Getting id, name and ip */
                            id = decd_buf;
                            name = strchr(id, ' ');
                            if(name)
                            { 
                                *name = '\0'; 
                                name++;

                                ip = strchr(name, ' ');
                                if(ip)
                                {
                                    *ip = '\0';
                                    ip++;

                                    tmp_str = strchr(ip, ' ');
                                    if(tmp_str)
                                    {
                                        *tmp_str = '\0';
                                    }
                                }
                            }
                        }

                        /* If ip isn't set, it is because we have an invalid
                         * auth key.
                         */
                        if(!ip)
                        {
                            MessageBox(hwnd, "Unable to import "
                                             "authentication key. Invalid.", 
                                             "Error Saving.", MB_OK);
                        }
                        else
                        {
                            char mbox_msg[1024 +1];
                            mbox_msg[1024] = '\0';
                            
                            snprintf(mbox_msg, 1024, "Adding key for:\r\n\r\n"
                                               "Agent ID: %s\r\n" 
                                               "Agent Name: %s\r\n" 
                                               "IP Address: %s\r\n",
                                               id, name, ip);
                             
                            ret = MessageBox(hwnd, mbox_msg, 
                                         "Confirm Importing Key", MB_OKCANCEL);
                            if(ret == IDOK)
                            {
                                FILE *fp;
                                fp = fopen(AUTH_FILE, "w");
                                if(fp)
                                {
                                    chd+=2;
                                    fprintf(fp, "%s", decd_to_write);
                                    fclose(fp);
                                }
                            }

                            
                        }

                        /* Free used memory */
                        if(decd_buf)
                        {
                            free(decd_to_write);
                            free(decd_buf);
                        }
                    }
                    else
                    {
                        GlobalFree(buf);
                    }

                } /* Finished adding AUTH KEY */

                /* Re-printing messages */
                if(chd)
                {
                    config_read(hwnd);

                    /* Set status to restart */
                    if(strcmp(config_inst.status,ST_RUNNING) == 0)
                    {
                        config_inst.status = ST_RUNNING_RESTART;
                    }

                    gen_server_info(hwnd);

                    if(chd == 1)
                    {
                        SendMessage(hStatus, SB_SETTEXT, 0,
                                (LPARAM)"Server IP Saved ..");
                    }
                    else if(chd == 2)
                    {
                        SendMessage(hStatus, SB_SETTEXT, 0,
                                (LPARAM)"Auth key imported ..");

                    }
                    else
                    {
                        SendMessage(hStatus, SB_SETTEXT, 0,
                                (LPARAM)"Auth key and server ip saved ..");

                    }
                }         
            }
            break;
            
            case UI_MENU_MANAGE_EXIT:
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;

            case UI_MENU_VIEW_LOGS:
                _spawnlp( _P_NOWAIT, "notepad", "notepad " OSSECLOGS, NULL );
                break;
            case UI_MENU_VIEW_CONFIG:    
                _spawnlp( _P_NOWAIT, "notepad", "notepad " CONFIG, NULL );
                break;
            case UI_MENU_HELP_HELP:
                _spawnlp( _P_NOWAIT, "notepad", "notepad " HELPTXT, NULL );
                break;
            case UI_MENU_HELP_ABOUT:
                {
                    DialogBox(GetModuleHandle(NULL), 
                            MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
                }
                break;
            case IDC_CANCEL:
                config_read(hwnd);    
                gen_server_info(hwnd);
                break;
                
            case UI_MENU_MANAGE_START:
            
                /* Starting OSSEC  -- must have a valid config before. */
                if((strcmp(config_inst.key, FL_NOKEY) != 0) &&
                   (strcmp(config_inst.server, FL_NOSERVER) != 0))
                {
                    ret_code = os_start_service();
                }
                else
                {
                    ret_code = 0;
                }
                
                if(ret_code == 0)
                {
                    MessageBox(hwnd, "Unable to start OSSEC (check config).",
                                     "Error -- Unable to start", MB_OK);
                }
                else if(ret_code == 1)
                {
                    config_read(hwnd);
                    gen_server_info(hwnd);

                    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"Started..");

                    MessageBox(hwnd, "OSSEC Agent Started.",
                                     "Started..", MB_OK);
                }
                else
                {
                    MessageBox(hwnd, "Agent already running (try restart).",
                                     "Already running..", MB_OK);
                }
                break;    
            case UI_MENU_MANAGE_STOP:
                
                /* Stopping OSSEC */
                ret_code = os_stop_service();
                if(ret_code == 1)
                {
                    config_read(hwnd);
                    gen_server_info(hwnd);

                    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"Stopped..");
                    MessageBox(hwnd, "OSSEC Agent Stopped.",
                                     "Stopped..", MB_OK);
                }
                else
                {
                    MessageBox(hwnd, "Agent already stopped.",
                                     "Already stopped..", MB_OK);
                }
                break;
            case UI_MENU_MANAGE_STATUS:
                if(CheckServiceRunning())
                {
                    MessageBox(hwnd, "OSSEC Agent running.",
                                     "Agent running..", MB_OK);

                }
                else
                {
                    MessageBox(hwnd, "OSSEC Agent stopped.",
                                     "Agent stopped.", MB_OK);
                }
                break;
            case UI_MENU_MANAGE_RESTART:
                
                if((strcmp(config_inst.key, FL_NOKEY) == 0) ||
                   (strcmp(config_inst.server, FL_NOSERVER) == 0))
                {
                    MessageBox(hwnd, "Unable to restart OSSEC (check config).",
                                     "Error -- Unable to restart", MB_OK);
                    break;
                    
                }
                                                                            
                ret_code = os_stop_service();
                
                /* Starting OSSEC */
                ret_code = os_start_service();
                if(ret_code == 0)
                {
                    MessageBox(hwnd, "Unable to restart OSSEC (check config).",
                                     "Error -- Unable to restart", MB_OK);
                }
                else
                {
                    config_read(hwnd);
                    gen_server_info(hwnd);

                    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"Restarted..");
                    MessageBox(hwnd, "OSSEC Agent Restarted.",
                                     "Restarted..", MB_OK);
                }
                break;        
        }
        break;
        
        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;
        
        default:
            return FALSE;
    }
    return TRUE;
}




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    int ret;
    WSADATA wsaData;


    /* Starting Winsock -- for name resolution. */
    WSAStartup(MAKEWORD(2, 0), &wsaData);


    /* Initializing config */
    init_config();

    /* Initializing controls */
    InitCommonControls();

    /* Creating main dialogbox */
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc);


    /* Check if service is running and try to start it */
    if((strcmp(config_inst.key, FL_NOKEY) != 0)&&
            (strcmp(config_inst.server, FL_NOSERVER) != 0) &&
            !CheckServiceRunning() &&
            (config_inst.admin_access != 0))
    {
        ret = MessageBox(NULL, "OSSEC Agent not running. "
                "Do you wish to start it?",
                "Wish to start the agent?", MB_OKCANCEL);
        if(ret == IDOK)
        {
            /* Starting the service */
            os_start_service();
        }
    }

    return(0);
}


/* EOF */
