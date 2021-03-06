/***************************************************************************
    NWNX Mysql - Database plugin for MySQL
    Copyright (C) 2007 Ingmar Stieger (Papillon, papillon@nwnx.org)

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

#include "mysql.h"
#include "nwn2heap.h"

/***************************************************************************
    NWNX and DLL specific functions
***************************************************************************/

MySQL* plugin;

DLLEXPORT Plugin* GetPluginPointerV2()
{
	return plugin;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		plugin = new MySQL();

		TCHAR szPath[MAX_PATH];
		GetModuleFileName(hModule, szPath, MAX_PATH);
		plugin->SetPluginFullPath(szPath);
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		delete plugin;
	}
    return TRUE;
}


/***************************************************************************
    Implementation of MySQL Plugin
***************************************************************************/

MySQL::MySQL()
{
	header = _T(
		"NWNX MySQL Plugin V.1.1.0-dev\n" \
		"(c) 2007 by Ingmar Stieger (Papillon)\n" \
		"(c) 2008 by virusman\n" \
		"visit us at http://www.nwnx.org\n" \
		"(built using mysql-5.0.27 source)\n");

	description = _T(
		"This plugin provides database storage. It uses " \
	    "MySQL 4 or 5 as database server.");

	subClass = _T("MySQL");
	version = _T("1.1.0-dev");

	result = NULL;
	row = NULL;
}

MySQL::~MySQL()
{
	Disconnect();
}

bool MySQL::Init(TCHAR* nwnxhome)
{
	SetupLogAndIniFile(nwnxhome);
	if(HookSCORCO())
		wxLogMessage(wxT("* Hooking successful"));
	else
		wxLogMessage(wxT("* Hooking failed"));

	if (config->Read(wxT("server"), &server) )
	{
		wxLogMessage(wxT("* Connecting to server %s"), server);
	}
	else
	{
		wxLogMessage(wxT("* MySQL database server not found in ini file"));
		server = wxT("localhost");
		wxLogMessage(wxT("* Using default server %s"), server);
	}

	if (!config->Read(wxT("user"), &user) )
	{
		wxLogMessage(wxT("* MySQL user account not found in ini file"));
		user = wxT("");
		wxLogMessage(wxT("* Using default user '%s'"), user);
	}

	if (!config->Read(wxT("password"), &password) )
	{
		wxLogMessage(wxT("* MySQL password not found in ini file"));
		password = wxT("");
		wxLogMessage(wxT("* Using default password '%s'"), password);
	}

	if (!config->Read(wxT("schema"), &schema) )
	{
		wxLogMessage(wxT("* MySQL schema not found in ini file"));
		user = wxT("nwn2");
		wxLogMessage(wxT("* Using default schema '%s'"), schema);
	}
	if (!config->Read(wxT("port"), &port) )
	{
		wxLogMessage(wxT("* MySQL port not found in ini file"));
		port = 0;
		wxLogMessage(wxT("* Using default port '%d'"), port);
	}

	if (!Connect())
	{
		wxLogMessage(wxT("* Connection to MySQL server failed:\n  %s"), mysql_error(&mysql));
	}

	wxLogMessage(wxT("* Plugin initialized."));

	return true;
}

bool MySQL::Connect()
{
	// initialize the mysql structure
	if (!mysql_init(&mysql))
		return FALSE;

	// try to connect to the mysql server
	connection = mysql_real_connect(&mysql, server, user, password, schema, port, NULL, CLIENT_MULTI_STATEMENTS);
	if (connection == NULL)
	{
		mysql_close(&mysql);
		return FALSE;
    }

	return TRUE;
}

void MySQL::Disconnect()
{
	// close the connection
	mysql_close(&mysql);
}

bool MySQL::Reconnect()
{
	wxLogMessage(wxT("* Reconnecting to MySQL server..."));
	Disconnect();
	if (!Connect())
	{
		wxLogMessage(wxT("* Connection to MySQL server failed:\n  %s"), mysql_error(&mysql));
		return false;
	}
	else
	{
		wxLogMessage(wxT("* Connection to MySQL server succeeded."));
		return true;
	}
}

bool MySQL::Execute(char* query)
{
	MYSQL_RES *newResult;

	if (!connection)
	{
		if (!Reconnect())
		{
			wxLogMessage(wxT("! Error: Not connected."));
			return FALSE;
		}
	}

	// eat any leftover resultsets so mysql does not get out of sync
	while (mysql_more_results(&mysql))
	{
		mysql_next_result(&mysql);
		newResult = mysql_store_result(&mysql);
		mysql_free_result(newResult);
	}

	// execute the query
	if (logLevel == 2)
		wxLogMessage(wxT("* Executing: %s"), query);
	if (mysql_query(connection, (const char *)query) != 0)
	{
		unsigned int error_no = mysql_errno(&mysql);
		if (logLevel > 0) { 
			wxLogMessage(wxT("! SQL Error: %s (%lu)."), mysql_error(&mysql), error_no);

			// log the query that caused the error, too,  if we haven't already. 
			if (logLevel != 2) { 
				wxLogMessage(wxT(" -> QUERY: %s."), query);
			}
		}

		// throw away last resultset if a SELECT statement failed
		if (_strnicmp(query, wxT("SELECT"), 6) == 0)
		{
			mysql_free_result(result);
			result = NULL;
			row = NULL;
			num_fields = 0;
		}

		if ((error_no == CR_SERVER_GONE_ERROR) && (Reconnect()))
		{
			// Retry query with new connection
			if (mysql_query(connection, (const char *)query) != 0)
				return FALSE;
		}
		else
		{
			return FALSE;
		}
	}

	// store the resultset in local memory
	newResult = mysql_store_result(&mysql);
	if (newResult == NULL)
	{
		if (mysql_field_count(&mysql) != 0)
		{
			// SELECT with an empty result set
			wxLogTrace(TRACE_VERBOSE, wxT("* Retrieved an empty resultset (mysql_query)"));
			mysql_free_result(result);
			result = NULL;
			row = NULL;
			num_fields = 0;
		}
		else
		{
			// NOT a SELECT like command
			wxLogTrace(TRACE_VERBOSE, wxT("* Retrieved an invalid resultset (NO SELECT) (mysql_query)"));

			// Try to advance to a non-empty resultset, if there is one.
			// This allows for calls to SQLFetch() to succeed automatically, 
			// even if the first n resultsets are empty or not valid.
			newResult = AdvanceToNextValidResultset();
			if (newResult)
			{
				mysql_free_result(result);
				result = newResult;
				row = NULL;
				num_fields = mysql_num_fields(result);
			}
		}

		if (mysql_errno(&mysql) != 0)
		{
			if (logLevel > 0)
				wxLogMessage(wxT("! Error (mysql_store_result): %s"), mysql_error(&mysql));
			return FALSE;	
		}
	}
	else
	{
		// successfully retrieved the resultset
		mysql_free_result(result);
		result = newResult;
		row = NULL;
		num_fields = mysql_num_fields(result);
	}
	
	return TRUE;
}

int MySQL::Fetch(char* buffer)
{
	if (!connection)
	{
		wxLogMessage(wxT("! Error (Fetch): Not connected."));
		return -1;
	}

	// If the parameter is NEXT, try to load the next
	// resultset from the last multi-statement query. 
	// If it is	empty, try to fetch the next row
	// from the current resultset. 
	if (strcmp(buffer,wxT("NEXT")) == 0)
	{
		wxLogTrace(TRACE_VERBOSE, wxT("* Trying to fetch the next resultset"));

		mysql_free_result(result);
		result = NULL;
		num_fields = 0;

		result = AdvanceToNextValidResultset();
		if (result)
		{
			num_fields = mysql_num_fields(result);
		}
	} 

	if (result)
	{
		row = mysql_fetch_row(result);
		if (row)
		{
			wxLogTrace(TRACE_VERBOSE, wxT("* Fetch returns a row."));
			return 1;
		}
	}

	row = NULL;
	wxLogTrace(TRACE_VERBOSE, wxT("* Fetch returns no row."));
	nwnxcpy(buffer, wxT(""));
	return 0;
}

int MySQL::GetData(int iCol, char* buffer)
{
	if (!row)
	{
		wxLogTrace(TRACE_VERBOSE, wxT("! GetData: No valid row in resultset."));
		nwnxcpy(buffer, wxT(""));
		return -1;
	}

	wxLogTrace(TRACE_VERBOSE, wxT("* GetData: Get column %d, buffer size %d bytes"), iCol, MAX_BUFFER);

	if ((iCol < (int)num_fields) && row[iCol])
	{
		nwnxcpy(buffer, row[iCol]);
		if (logLevel == 2)
			wxLogMessage(wxT("* Returning: %s (column %d)"), buffer, iCol);
		return 0;
	}
	else
	{
		nwnxcpy(buffer, wxT(""));
		if (logLevel == 2)
			wxLogMessage(wxT("* Returning: (empty) (column %d)"), iCol);
		return -1;
	}
}

MYSQL_RES* MySQL::AdvanceToNextValidResultset()
{
	MYSQL_RES *newResult;

	while (mysql_more_results(&mysql))
	{
		wxLogTrace(TRACE_VERBOSE, wxT("* Got a resultset"));
		mysql_next_result(&mysql);
		newResult = mysql_store_result(&mysql);
		if (newResult == NULL)
		{
			wxLogTrace(TRACE_VERBOSE, wxT("* Empty resultset"));
			if (mysql_field_count(&mysql) != 0)
			{
				// SELECT with an empty result set
				wxLogTrace(TRACE_VERBOSE, wxT("* Retrieved an empty resultset"));
				return NULL;
			}
			else
			{
				// NOT a SELECT like command, advance to next resultset
				wxLogTrace(TRACE_VERBOSE, wxT("* Retrieved an invalid resultset (NO SELECT)"));
			}
		}
		else
		{
			// SELECT with a non-empty resultset
			wxLogTrace(TRACE_VERBOSE, wxT("* Retrieved a non-empty resultset"));
			return newResult;
		}
	}

	// no non-empty resultset found
	return NULL;
}

int MySQL::GetAffectedRows()
{
	return mysql_affected_rows(&mysql);
}

void MySQL::GetEscapeString(char* str, char* buffer)
{
	if (*str == 0x0)
	{
		nwnxcpy(buffer, wxT(""));
		return;
	}

	size_t len, to_len;
	len = strlen(str);
	
	char* to = (char*)malloc(len*2+1);
	to_len = mysql_real_escape_string(&mysql, to, str, (unsigned long)len);
	nwnxcpy(buffer, to, to_len);
	free(to);
}

int MySQL::GetErrno()
{
	return mysql_errno(&mysql);
}

const char *MySQL::GetErrorMessage()
{
	return mysql_error(&mysql);
}

BOOL MySQL::WriteScorcoData(BYTE* pData, int Length)
{
	if (logLevel == 2)
		wxLogMessage(wxT("* SCO query: %s"), scorcoSQL);
	wxLogTrace(TRACE_VERBOSE, wxT("WriteScorcoData"));
	int res;
	unsigned long len;
	char* Data = new char[Length * 2 + 1 + 2];
	char* pSQL = new char[MAXSQL + Length * 2 + 1];

	len = mysql_real_escape_string (&mysql, Data + 1, (const char*)pData, Length);
	Data[0] = Data[len + 1] = 39; //'
	Data[len + 2] = 0x0; 
	sprintf(pSQL, scorcoSQL, Data);

	MYSQL_RES *result = mysql_store_result (&mysql);
	res = mysql_query(&mysql, (const char *) pSQL);

	mysql_free_result(result);
	delete[] pSQL;
	delete[] Data;

	if (res == 0)
		return true;
	else
		return false;
}

BYTE* MySQL::ReadScorcoData(char *param, int *size)
{
	if (logLevel == 2)
		wxLogMessage(wxT("* RCO query: %s"), scorcoSQL);
	wxLogTrace(TRACE_VERBOSE, wxT("ReadScorcoData"));
	BOOL pSqlError;
	MYSQL_RES *rcoresult;
	if (strcmp(param, "FETCHMODE") != 0)
	{	
		if (mysql_query(&mysql, (const char *) scorcoSQL) != 0)
		{
			pSqlError = true;
			return NULL;
		}

		/*if (result)
		{
		mysql_free_result(result);
		result = NULL;
		}*/
		rcoresult = mysql_store_result (&mysql);
		if (!rcoresult)
		{
			pSqlError = true;
			return NULL;
		}
	}
	else rcoresult=result;

	MYSQL_ROW row;
	pSqlError = false;
	row = mysql_fetch_row(rcoresult);
	if (row)
	{
		unsigned long* length = mysql_fetch_lengths(rcoresult);
		// allocate buf for result!
		//char* buf = new char[*length];
		NWN2_HeapMgr *pHeapMgr = NWN2_HeapMgr::Instance();
		NWN2_Heap *pHeap = pHeapMgr->GetDefaultHeap();
		char* buf = (char *) pHeap->Allocate(*length);

		if (!buf) return NULL;

		memcpy(buf, row[0], length[0]);
		*size = length[0];
		mysql_free_result(rcoresult);
		return (BYTE*)buf;
	}
	else
	{
		if (logLevel == 2)
			wxLogMessage(wxT("* Empty RCO resultset"));
		mysql_free_result(rcoresult);
		return NULL;
	}
}
