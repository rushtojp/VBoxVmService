#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <winsvc.h>
#include <process.h>

const int nBufferSize = 500;
char pInitFile[nBufferSize+1];

void show_usage()
{
    printf("VBoxVmSerice control utility\n");
    printf("usage: VmServiceControl [options]\n");
    printf("       -i        Install VBoxVmService service\n");
    printf("       -u        Uninstall VBoxVmService service\n");
    printf("       -s        Start VBoxVmService service\n");
    printf("       -k        Stop VBoxVmService service\n");
    printf("       -b        Restart VBoxVmService service\n");
    // this option is hidden because it's not very usefull
    //printf("       -b n      Restart VM with index n\n");
    printf("       -su n     Startup VM with index n\n");
    printf("       -sd n     Shutdown VM with index n\n");
    printf("\n");
}

BOOL KillService(char* pName) 
{ 
    // kill service with given name
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if (schSCManager==0) 
    {
        long nError = GetLastError();
        fprintf_s(stderr, "OpenSCManager failed, error code = %d\n", nError);
    }
    else
    {
        // open the service
        SC_HANDLE schService = OpenService( schSCManager, pName, SERVICE_ALL_ACCESS);
        if (schService==0) 
        {
            long nError = GetLastError();
            fprintf_s(stderr, "OpenService failed, error code = %d\n", nError);
        }
        else
        {
            // call ControlService to kill the given service
            SERVICE_STATUS status;
            if(ControlService(schService,SERVICE_CONTROL_STOP,&status))
            {
                fprintf_s(stdout, "Service stopped successfully\n");
                CloseServiceHandle(schService); 
                CloseServiceHandle(schSCManager); 
                return TRUE;
            }
            else
            {
                long nError = GetLastError();
                fprintf_s(stderr, "ControlService failed, error code = %d\n", nError);
            }
            CloseServiceHandle(schService); 
        }
        CloseServiceHandle(schSCManager); 
    }
    return FALSE;
}


BOOL RunService(char* pName, int nArg, char** pArg) 
{ 
    // run service with given name
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if (schSCManager==0) 
    {
        long nError = GetLastError();
        fprintf_s(stderr, "OpenSCManager failed, error code = %d\n", nError);
    }
    else
    {
        // open the service
        SC_HANDLE schService = OpenService( schSCManager, pName, SERVICE_ALL_ACCESS);
        if (schService==0) 
        {
            long nError = GetLastError();
            fprintf_s(stderr, "OpenService failed, error code = %d\n", nError);
        }
        else
        {
            // call StartService to run the service
            if(StartService(schService,nArg,(const char**)pArg))
            {
                fprintf_s(stdout, "Service started successfully\n");
                CloseServiceHandle(schService); 
                CloseServiceHandle(schSCManager); 
                return TRUE;
            }
            else
            {
                long nError = GetLastError();
                fprintf_s(stderr, "StartService failed, error code = %d\n", nError);
            }
            CloseServiceHandle(schService); 
        }
        CloseServiceHandle(schSCManager); 
    }
    return FALSE;
}

BOOL BounceProcess(char* pName, int nIndex) 
{ 
    // bounce the process with given index
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if (schSCManager==0) 
    {
        long nError = GetLastError();
        fprintf_s(stderr, "OpenSCManager failed, error code = %d\n", nError);
    }
    else
    {
        // open the service
        SC_HANDLE schService = OpenService( schSCManager, pName, SERVICE_ALL_ACCESS);
        if (schService==0) 
        {
            long nError = GetLastError();
            fprintf_s(stderr, "OpenService failed, error code = %d\n", nError); 
        }
        else
        {
            // call ControlService to invoke handler
            SERVICE_STATUS status;
            if(nIndex>=0&&nIndex<128)
            {
                if(ControlService(schService,(nIndex|0x80),&status))
                {
                    CloseServiceHandle(schService); 
                    CloseServiceHandle(schSCManager); 
                    return TRUE;
                }
                long nError = GetLastError();
                fprintf_s(stderr, "ControlService failed, error code = %d\n", nError); 
            }
            else
            {
                fprintf_s(stderr, "Invalid argument to BounceProcess: %d\n", nIndex); 
            }
            CloseServiceHandle(schService); 
        }
        CloseServiceHandle(schSCManager); 
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////// 
//
// Uninstall
//
VOID UnInstall(char* pName)
{
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if (schSCManager==0) 
    {
        long nError = GetLastError();
        fprintf_s(stderr, "OpenSCManager failed, error code = %d\n", nError);
    }
    else
    {
        SC_HANDLE schService = OpenService( schSCManager, pName, SERVICE_ALL_ACCESS);
        if (schService==0) 
        {
            long nError = GetLastError();
            fprintf_s(stderr, "OpenService failed, error code = %d\n", nError);
        }
        else
        {
            if(!DeleteService(schService)) 
            {
                fprintf_s(stderr, "Failed to delete service %s\n", pName);
            }
            else 
            {
                fprintf_s(stdout, "Service %s removed\n",pName);
            }
            CloseServiceHandle(schService); 
        }
        CloseServiceHandle(schSCManager);   
    }
}

////////////////////////////////////////////////////////////////////// 
//
// Install
//
VOID Install(char* pPath, char* pName) 
{  
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE); 
    if (schSCManager==0) 
    {
        long nError = GetLastError();
        fprintf_s(stderr, "OpenSCManager failed, error code = %d\n", nError);
    }
    else
    {
        SC_HANDLE schService = CreateService( 
                schSCManager,         /* SCManager database      */ 
                pName,                /* name of service         */ 
                pName,                /* service name to display */ 
                SERVICE_ALL_ACCESS,   /* desired access          */ 
                SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS ,    /* service type            */ 
                SERVICE_AUTO_START,   /* start type              */ 
                SERVICE_ERROR_NORMAL, /* error control type      */ 
                pPath,                /* service's binary        */ 
                NULL,                 /* no load ordering group  */ 
                NULL,                 /* no tag identifier       */ 
                NULL,                 /* no dependencies         */ 
                NULL,                 /* LocalSystem account     */ 
                NULL                  /* no password             */
                );                      
        if (schService==0) 
        {
            long nError =  GetLastError();
            fprintf_s(stderr, "Failed to create service %s, error code = %d\n", pName, nError);
        }
        else
        {
            // Set system wide VBOX_USER_HOME environment variable
            char pVboxUserHome[nBufferSize+1];
            GetPrivateProfileString("Settings","VBOX_USER_HOME","",pVboxUserHome,nBufferSize,pInitFile);
            HKEY hKey;
            DWORD dwDisp;
            LONG lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
            if (lRet == ERROR_SUCCESS)
            {
                lRet = RegSetValueEx(hKey, "VBOX_USER_HOME", 0, REG_SZ, (const BYTE *)pVboxUserHome, strlen(pVboxUserHome) + 1);
                RegCloseKey(hKey);
            }
            if (lRet != ERROR_SUCCESS)
            {
                fprintf_s(stderr, "Failed to set VBOX_USER_HOME environment\n");
            }

            // also set environment variable for current process, so that
            // no reboot is required after installation.
            SetEnvironmentVariable("VBOX_USER_HOME", pVboxUserHome);

            fprintf_s(stdout, "Service %s installed\n", pName);
            CloseServiceHandle(schService); 
        }
        CloseServiceHandle(schSCManager);
    }   
}

BOOL SendCommandToService(char * message)
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;
    while (true) 
    { 
        hPipe = ::CreateFile((LPSTR)"\\\\.\\pipe\\VBoxVmService", 
                GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        dwError = GetLastError();
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            break;
        }

        // If any error except the ERROR_PIPE_BUSY has occurred,
        // we should return FALSE. 
        if (dwError != ERROR_PIPE_BUSY) 
        {
            return FALSE;
        }
        // The named pipe is busy. Let��s wait for 2 seconds. 
        if (!WaitNamedPipe((LPSTR)"\\\\.\\pipe\\VBoxVmService", 2000)) 
        { 
            dwError = GetLastError();
            return FALSE;
        } 
    } 
    DWORD dwRead = 0;
    if (!(WriteFile(hPipe, (LPVOID)message, strlen(message), &dwRead, 0)))
    {
        CloseHandle(hPipe);
        return FALSE;
    }
    CloseHandle(hPipe);
    return TRUE;
}

////////////////////////////////////////////////////////////////////// 
//
// Standard C Main
//
void main(int argc, char *argv[] )
{
    char pModuleFile[nBufferSize+1];
    char pExeFile[nBufferSize+1];
    char pServiceName[nBufferSize+1];

    DWORD dwSize = GetModuleFileName(NULL,pModuleFile,nBufferSize);
    pModuleFile[dwSize] = 0;
    *(strrchr(pModuleFile, '\\')) = 0;
    sprintf_s(pExeFile,"%s\\VBoxVmService.exe",pModuleFile);
    sprintf_s(pInitFile,"%s\\VBoxVmService.ini",pModuleFile);
    GetPrivateProfileString("Settings","ServiceName","VBoxVmService",pServiceName,nBufferSize,pInitFile);

    // uninstall service if switch is "-u"
    if(argc==2&&_stricmp("-u",argv[1])==0)
    {
        UnInstall(pServiceName);
    }
    // install service if switch is "-i"
    else if(argc==2&&_stricmp("-i",argv[1])==0)
    {           
        Install(pExeFile, pServiceName);
    }
    // start service if switch is "-s"
    else if(argc==2&&_stricmp("-s",argv[1])==0)
    {           
        RunService(pServiceName,0,NULL);
    }
    // stop service if switch is "-k"
    else if(argc==2&&_stricmp("-k",argv[1])==0)
    {           
        KillService(pServiceName);
    }
    // bounce service if switch is "-b"
    else if(argc==2&&_stricmp("-b",argv[1])==0)
    {           
        KillService(pServiceName);
        RunService(pServiceName,0,NULL);
    }
    // bounce a specifc VM if the index is supplied
    else if(argc==3&&_stricmp("-b",argv[1])==0)
    {
        int nIndex = atoi(argv[2]);
        if(BounceProcess(pServiceName, nIndex))
            fprintf_s(stdout, "Bounced VM %d\n", nIndex);
        else
            fprintf_s(stderr, "Failed to bounce VM %d\n", nIndex);
    }
    // STARTUP SWITCH
    // 
    // exit with error if switch is "-su" and no VmId is supplied
    else if(argc==2&&_stricmp("-su",argv[1])==0)
    {
        fprintf_s(stderr, "Startup option needs to be followed by a VM index.\n");
    }
    // start a specific vm (if the index is supplied)
    else if(argc==3&&_stricmp("-su",argv[1])==0)
    {
        int nIndex = atoi(argv[2]);
        char pCommand[80];
        sprintf_s(pCommand, "start %u", nIndex);
        if(SendCommandToService(pCommand))
            fprintf_s(stdout, "Started your virtual machine, VM%d\n", nIndex);
        else
            fprintf_s(stderr, "Failed to send command to service.\n");
    }
    // SHUTDOWN SWITCH
    // 
    // exit with error if switch is "-sd" and no VmId is supplied
    else if(argc==2&&_stricmp("-sd",argv[1])==0)
    {
        fprintf_s(stderr, "Shutdown option needs to be followed by a VM index.\n");
    }
    // shutdown a specifc vm (if the index is supplied)
    else if(argc==3&&_stricmp("-sd",argv[1])==0)
    {
        int nIndex = atoi(argv[2]);
        char pCommand[80];
        sprintf_s(pCommand, "stop %u", nIndex);
        if(SendCommandToService(pCommand))
            fprintf_s(stdout, "Shutdown your virtual machine, VM%d\n", nIndex);
        else
            fprintf_s(stderr, "Failed to send command to service.\n");
    }
    else 
        show_usage();
}
