/***************************************************************************
    NWNX Controller - Windows Service handling functions
    Copyright (C) 2006 Ingmar Stieger (Papillon, papillon@nwnx.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***************************************************************************/

#include "stdwx.h"
#include "service.h"

SERVICE_STATUS_HANDLE NWNXServiceStatusHandle; 
SERVICE_STATUS NWNXServiceStatus;

SC_HANDLE getSCManager()
{
	// Return a handle to the SC Manager database. 
 	SC_HANDLE schSCManager = OpenSCManager( 
    NULL,                    // local machine 
    NULL,                    // ServicesActive database 
    SC_MANAGER_ALL_ACCESS);  // full access rights 
 
	if (schSCManager == NULL) 
		wxLogError(wxT("* Failed to connect to service manager (%d)"), GetLastError());

	return schSCManager;
}

BOOL installservice(int serviceNo)
{
	SC_HANDLE schSCManager, schService;
	SERVICE_DESCRIPTION sdBuf;

    TCHAR szPath[MAX_PATH], cmdLine[MAX_PATH]; 
	TCHAR serviceName[64];
	TCHAR displayName[64];

	wxLogMessage(wxT("* Installing NWNX Service %d..."), serviceNo);

	schSCManager = getSCManager();
	if (NULL == schSCManager) 
		return FALSE;

    if(!GetModuleFileName(NULL, szPath, MAX_PATH ) )
    {
		wxLogError(wxT("* GetModuleFileName failed (%d)"), GetLastError()); 
        return FALSE;
    }

	_stprintf_s(serviceName, 64, _T("NWNX4-%d"), serviceNo);
	_stprintf_s(displayName, 64, _T("NWNX4 Service %d"), serviceNo);
	_stprintf_s(cmdLine, MAX_PATH, _T("%s -serviceno %d -runservice"), szPath, serviceNo);
	sdBuf.lpDescription = _T("Neverwinter Nights Extender 4 service instance");

    schService = CreateService( 
        schSCManager,              // SCManager database 
        serviceName,		       // name of service 
        displayName,               // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        cmdLine,                   // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 
 
    if (schService == NULL) 
    {
		wxLogError(wxT("* CreateService failed (%d)"), GetLastError()); 
        return FALSE;
    }

    if(!ChangeServiceConfig2(
        schService,                 // handle to service
        SERVICE_CONFIG_DESCRIPTION, // change: description
        &sdBuf) )                   // value: new description
    {
        wxLogError(wxT("ChangeServiceConfig2 failed"));
        return FALSE;
    }

    CloseServiceHandle(schService); 
    return TRUE;

}

BOOL uninstallservice(int serviceNo)
{
	SC_HANDLE schSCManager, schService;
	TCHAR serviceName[64];

	_stprintf_s(serviceName, 64, wxT("NWNX4-%d"), serviceNo);
	wxLogMessage(wxT("* Uninstalling NWNX Service %d..."), serviceNo);

	schSCManager = getSCManager();
	if (NULL == schSCManager) 
		return FALSE;

    schService = OpenService( 
        schSCManager,       // SCManager database 
        serviceName,        // name of service 
        DELETE);            // only need DELETE access 
 
    if (schService == NULL)
    { 
        wxLogError(wxT("* OpenService failed (%d)"), GetLastError()); 
        return FALSE;
    }
 
    if (!DeleteService(schService)) 
    {
		wxLogError(wxT("* DeleteService failed (%d)"), GetLastError()); 
        return FALSE;
    }
    else 
		wxLogMessage(wxT("* DeleteService succeeded")); 
 
    CloseServiceHandle(schService); 
    return TRUE;
}

void WINAPI NWNXServiceStart(DWORD argc, LPTSTR *argv) 
{ 
    DWORD status; 
    DWORD specificError; 
	TCHAR serviceName[64];
 
    NWNXServiceStatus.dwServiceType = SERVICE_WIN32; 
    NWNXServiceStatus.dwCurrentState = SERVICE_START_PENDING; 
    NWNXServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP; 
    NWNXServiceStatus.dwWin32ExitCode = 0; 
    NWNXServiceStatus.dwServiceSpecificExitCode = 0; 
    NWNXServiceStatus.dwCheckPoint = 0; 
    NWNXServiceStatus.dwWaitHint = 0; 
 
	_stprintf_s(serviceName, 64, wxT("NWNX4-%d"), serviceNo);
    NWNXServiceStatusHandle = RegisterServiceCtrlHandler(serviceName, NWNXServiceCtrlHandler); 
 
    if (NWNXServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) 
    { 
		wxLogError(wxT("* RegisterServiceCtrlHandler failed %d"), GetLastError()); 
        return; 
    } 
 
    // Initialization code goes here. 
    status = NWNXServiceInitialization(argc,argv, &specificError); 
 
    // Handle error condition 
    if (status != NO_ERROR) 
    { 
        NWNXServiceStatus.dwCurrentState = SERVICE_STOPPED; 
        NWNXServiceStatus.dwCheckPoint = 0; 
        NWNXServiceStatus.dwWaitHint = 0; 
        NWNXServiceStatus.dwWin32ExitCode = status; 
        NWNXServiceStatus.dwServiceSpecificExitCode = specificError; 
 
        SetServiceStatus(NWNXServiceStatusHandle, &NWNXServiceStatus); 
        return; 
    } 
 
    // Initialization complete - report running status. 
    NWNXServiceStatus.dwCurrentState = SERVICE_RUNNING; 
    NWNXServiceStatus.dwCheckPoint = 0; 
    NWNXServiceStatus.dwWaitHint = 0; 
 
    if (!SetServiceStatus(NWNXServiceStatusHandle, &NWNXServiceStatus)) 
    { 
        status = GetLastError(); 
		wxLogError(wxT("* SetServiceStatus error %ld"), status); 
    } 

    // This is where the service does its work. 
	start_worker();
 
    return; 
} 
 
// Stub initialization function. 
DWORD NWNXServiceInitialization(DWORD argc, LPTSTR *argv, DWORD *specificError) 
{ 
    argv; 
    argc; 
    specificError; 
    return(0); 
}

VOID WINAPI NWNXServiceCtrlHandler(DWORD Opcode) 
{ 
	DWORD status; 

	switch(Opcode) 
	{ 
		case SERVICE_CONTROL_STOP: 

			//
			// Notify the SCM immediately that we received the request and are
			// working on stopping.
			//

			NWNXServiceStatus.dwWin32ExitCode = 0; 
			NWNXServiceStatus.dwCurrentState  = SERVICE_STOP_PENDING; 
			NWNXServiceStatus.dwCheckPoint    = 0; 
			NWNXServiceStatus.dwWaitHint      = controller->getGracefulShutdownTimeout() + 6000;

			if (!SetServiceStatus(NWNXServiceStatusHandle, &NWNXServiceStatus))
			{ 
				status = GetLastError(); 
				wxLogError(wxT("* SetServiceStatus error %ld"), status); 
			}

			//
			// Cleanly shutdown the server process.
			//

			controller->notifyServiceShutdown();
			controller->killServerProcess();

			NWNXServiceStatus.dwWin32ExitCode = 0; 
			NWNXServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
			NWNXServiceStatus.dwCheckPoint    = 0; 
			NWNXServiceStatus.dwWaitHint      = 0; 

			if (!SetServiceStatus(NWNXServiceStatusHandle, &NWNXServiceStatus))
			{ 
				status = GetLastError(); 
				wxLogError(wxT("* SetServiceStatus error %ld"), status); 
			}

			wxLogMessage(wxT("* Service successfully stopped.")); 
			return; 
 
			case SERVICE_CONTROL_INTERROGATE: 
				// Fall through to send current status. 
				break; 
 
			default: ;
			wxLogError(wxT("* Unrecognized opcode %ld"), Opcode); 
	} 

	// Send current status. 
	if (!SetServiceStatus (NWNXServiceStatusHandle,  &NWNXServiceStatus)) 
	{ 
		status = GetLastError(); 
		wxLogError(wxT("* SetServiceStatus error %ld"), status); 
	} 
	return; 
}

BOOL StartNWNXService(int serviceNo) 
{ 
    SC_HANDLE schService;
	SC_HANDLE schSCManager;
    SERVICE_STATUS_PROCESS ssStatus; 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;
	TCHAR serviceName[64];
 
	schSCManager = getSCManager();
	if (NULL == schSCManager) 
		return FALSE;

	_stprintf_s(serviceName, 64, wxT("NWNX4-%d"), serviceNo);
    schService = OpenService( 
        schSCManager,           // SCManager database 
        serviceName,            // name of service 
        SERVICE_ALL_ACCESS);    

    if (schService == NULL)
    { 
		wxLogError(wxT("* OpenService failed (%d)"), GetLastError()); 
        return FALSE;
    }
 
    if (!StartService(
            schService,  // handle to service 
            0,           // number of arguments 
            NULL) )      // no arguments 
    {
        return 0; 
    }
    else 
    {
        wxLogMessage(wxT("* Starting NWNX service %d..."), serviceNo);
    }

    // Check the status until the service is no longer start pending. 
 
    if (!QueryServiceStatusEx( 
            schService,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            (LPBYTE)&ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
    {
        return 0; 
    }
 
    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        // Do not wait longer than the wait hint. A good interval is 
        // one tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 
 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status again. 
 
        if (!QueryServiceStatusEx( 
            schService,			            // handle to service 
            SC_STATUS_PROCESS_INFO,         // info level
            (LPBYTE)&ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
            break; 
 
        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // The service is making progress.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint
                break;
            }
        }
    } 

    CloseServiceHandle(schService); 

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
		wxLogMessage(wxT("* Service successfully started.")); 
        return 1;
    }
    else 
    { 
        wxLogError(wxT("! Service not started."));
		wxLogError(wxT("! Current State: %d"), ssStatus.dwCurrentState); 
		wxLogError(wxT("! Exit Code: %d"), ssStatus.dwWin32ExitCode); 
		wxLogError(wxT("! Service Specific Exit Code: %d"), ssStatus.dwServiceSpecificExitCode); 
		wxLogError(wxT("! Check Point: %d"), ssStatus.dwCheckPoint); 
		wxLogError(wxT("! Wait Hint: %d"), ssStatus.dwWaitHint); 
        return 0;
    } 
}

// This function attempts to stop a service. It allows the caller to 
// specify whether dependent services should also be stopped. It also 
// accepts a timeout value, to prevent a scenario in which a service 
// shutdown hangs, then the application stopping the service hangs.
// 
// If the operation is successful, returns ERROR_SUCCESS. Otherwise, 
// returns a system error code.

DWORD StopNWNXService(int serviceNo) 
{
    SC_HANDLE schService;
	SC_HANDLE schSCManager;
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
	DWORD dwTimeout = 10000; //msec
	TCHAR serviceName[64];

	_stprintf_s(serviceName, 64, wxT("NWNX4-%d"), serviceNo);

	schSCManager = getSCManager();
	if (NULL == schSCManager) 
		return FALSE;

    schService = OpenService( 
        schSCManager,       // SCManager database 
        serviceName,
        SERVICE_ALL_ACCESS);

    if (schService == NULL)
    { 
		wxLogError(wxT("* OpenService failed (%d)"), GetLastError()); 
        return FALSE;
    }

	wxLogMessage(wxT("* Stopping NWNX service %d..."), serviceNo); 

    // Make sure the service is not already stopped
    if (!QueryServiceStatusEx( 
            schService, 
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssStatus, 
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
        return GetLastError();

    if (ssStatus.dwCurrentState == SERVICE_STOPPED) 
       return ERROR_SUCCESS;

    // If a stop is pending, just wait for it
    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) 
    {
        Sleep(ssStatus.dwWaitHint);
        if (!QueryServiceStatusEx( 
                schService, 
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssStatus, 
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded ) )
            return GetLastError();

        if (ssStatus.dwCurrentState == SERVICE_STOPPED)
            return ERROR_SUCCESS;

        if (GetTickCount() - dwStartTime > dwTimeout)
            return ERROR_TIMEOUT;
    }

    // Send a stop code to the main service
	SERVICE_STATUS ss;
    if (!ControlService(schService, SERVICE_CONTROL_STOP, &ss ))
        return GetLastError();

   // Wait for the service to stop
   while (ss.dwCurrentState != SERVICE_STOPPED) 
   {
       Sleep(ss.dwWaitHint);
       if (!QueryServiceStatusEx( 
               schService, 
               SC_STATUS_PROCESS_INFO,
               (LPBYTE)&ss, 
               sizeof(SERVICE_STATUS),
               &dwBytesNeeded))
           return GetLastError();

       if (ss.dwCurrentState == SERVICE_STOPPED)
           break;

        if (GetTickCount() - dwStartTime > dwTimeout)
           return ERROR_TIMEOUT;
    }

    // Return success
    return ERROR_SUCCESS;
}


