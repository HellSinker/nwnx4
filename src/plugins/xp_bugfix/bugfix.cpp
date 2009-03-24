/***************************************************************************
    NWNX BugFix - NWN2Server bugfixes and patches plugin
    Copyright (C) 2008 Skywing (skywing@valhallalegends.com)

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

#include "bugfix.h"
#include "..\..\misc\Patch.h"
#include "StackTracer.h"
#include "wx/fileconf.h"

#define BUGFIX_VERSION "1.0.1"
#define __NWN2_VERSION_STR(X) #X
#define _NWN2_VERSION_STR(X) __NWN2_VERSION_STR(X)
#define NWN2_VERSION _NWN2_VERSION_STR(NWN2SERVER_VERSION)

/***************************************************************************
    NWNX and DLL specific functions
***************************************************************************/

BugFix* plugin;

Patch _patches[] =
{
	Patch( OFFS_CalcPositionLoop0, "\xe9", 1 ),
	Patch( OFFS_CalcPositionLoop0+1, (relativefunc)BugFix::CalcPositionsLoop0Fix ),
	Patch( OFFS_CalcPositionLoop1, "\xe9", 1 ),
	Patch( OFFS_CalcPositionLoop1+1, (relativefunc)BugFix::CalcPositionsLoop1Fix ),
#if NWN2SERVER_VERSION < 0x01211549
	Patch( OFFS_NullDerefCrash0, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash0+1, (relativefunc)BugFix::NullDerefCrash0Fix ),
	Patch( OFFS_NullDerefCrash1, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash1+1, (relativefunc)BugFix::NullDerefCrash1Fix ),
#endif
	Patch( OFFS_NullDerefCrash2, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash2+1, (relativefunc)BugFix::NullDerefCrash2Fix ),
#if NWN2SERVER_VERSION < 0x01211549
	Patch( OFFS_NullDerefCrash3, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash3+1, (relativefunc)BugFix::NullDerefCrash3Fix ),
#endif
	Patch( OFFS_NullDerefCrash4, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash4+1, (relativefunc)BugFix::NullDerefCrash4Fix ),
	Patch( OFFS_Crash5, "\xe9", 1 ),
	Patch( OFFS_Crash5+1, (relativefunc)BugFix::Crash5Fix ),
	Patch( OFFS_NullDerefCrash6, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash6+1, (relativefunc)BugFix::NullDerefCrash6Fix ),
	Patch( OFFS_NullDerefCrash7, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash7+1, (relativefunc)BugFix::NullDerefCrash7Fix ),
	Patch( OFFS_NullDerefCrash8, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash8+1, (relativefunc)BugFix::NullDerefCrash8Fix ),
	Patch( OFFS_CheckUncompress0, "\xe9", 1 ),
	Patch( OFFS_CheckUncompress0+1, (relativefunc)BugFix::Uncompress0Fix ),
	Patch( OFFS_CheckUncompress1, "\xe9", 1 ),
	Patch( OFFS_CheckUncompress1+1, (relativefunc)BugFix::Uncompress1Fix ),
	Patch( OFFS_NullDerefCrash9, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash9+1, (relativefunc)BugFix::NullDerefCrash9Fix ),
#if NWN2SERVER_VERSION >= 0x01211549
	Patch( OFFS_NullDerefCrash10, "\xe9", 1 ),
	Patch( OFFS_NullDerefCrash10+1, (relativefunc)BugFix::NullDerefCrash10Fix ),
#endif
#if NWN2SERVER_VERSION == 0x01211549 && defined(XP_BUGFIX_USE_SYMBOLS)
	Patch( OFFS_CGameEffectDtor, "\xe9", 1 ),
	Patch( OFFS_CGameEffectDtor+1, (relativefunc)BugFix::CGameEffectDtorLogger ),
#endif

	Patch()
};

Patch *patches = _patches;

typedef void (__cdecl * NWN2Heap_Deallocate_Proc)(void *p);

NWN2Heap_Deallocate_Proc NWN2Heap_Deallocate;

DLLEXPORT Plugin* GetPluginPointerV2()
{
	return plugin;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		plugin = new BugFix();

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
    Implementation of BugFix Plugin
***************************************************************************/

BugFix::BugFix()
{
	header = 
		_T("NWNX BugFix Plugin V") _T( BUGFIX_VERSION ) _T("\n") \
		_T("(c) 2008 by Skywing \n") \
		_T("Visit NWNX at: http://www.nwnx.org\n") \
		_T("Built for NWN2 version ") _T( NWN2_VERSION ) _T("\n");

	description = _T(
		"This plugin fixes some known bugs in nwn2server.");

	subClass = _T("BUGFIX");
	version  = _T(BUGFIX_VERSION);

	lastlog  = GetTickCount();
}

BugFix::~BugFix()
{
	wxLogMessage(wxT("* Plugin unloaded."));
}

bool BugFix::Init(TCHAR* nwnxhome)
{
	assert(GetPluginFileName());

	/* Log file */
	wxString logfile(nwnxhome); 
	logfile.append(wxT("\\"));
	logfile.append(GetPluginFileName());
	logfile.append(wxT(".txt"));
	logger = new wxLogNWNX(logfile, wxString(header.c_str()), true, true);

	if (!Check())
	{
		wxLogMessage( wxT( "* Wrong nwn2server version, patches not active." ) );
		return true;
	}


	nwn2mm   = GetModuleHandle( _T( "NWN2_MemoryMgr_amdxp.dll" ) );

	if (nwn2mm)
		NWN2Heap_Deallocate = (NWN2Heap_Deallocate_Proc)GetProcAddress( nwn2mm, "?Deallocate@NWN2_Heap@@SAXPAX@Z" );

	if (!NWN2Heap_Deallocate)
		wxLogMessage( wxT( "* WARNING: Failed to locate NWN2_Heap::Deallocate." ) );

	int i = 0;
	while(patches[i].Apply()) {
		i++;
	}

	wxLogMessage(wxT("* Plugin initialized."));

	/* Ini file */
	wxString inifile(nwnxhome); 
	inifile.append(wxT("\\"));
	inifile.append(GetPluginFileName());
	inifile.append(wxT(".ini"));

	wxLogMessage(wxT("* Reading inifile %s"), inifile);

	wxFileConfig config(wxEmptyString, wxEmptyString, 
		inifile, wxEmptyString,
		wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_NO_ESCAPE_CHARACTERS
		);


#ifdef XP_BUGFIX_USE_SYMBOLS

	tracer = new StackTracer();

	wxString TraceLogFileName;

	if (config.Read( "StackTraceLogFile", &TraceLogFileName ))
	{
		int TraceCount = 0;

		wxLogMessage(wxT("* Trace log file: %s"),
			TraceLogFileName);

		if (!config.Read( "StackTraceCount", &TraceCount ))
			TraceCount = 1024;

		if (!tracer->Initialize(
			(size_t)TraceCount,
			TraceLogFileName.wc_str(wxConvLibc).data()))
		{
			wxLogMessage(wxT("* Failed to initialize stack tracing for '%s' (%lu traces)."),
				TraceLogFileName, TraceCount);

			delete tracer;

			tracer = NULL;
		}
		else
		{
			wxLogMessage(wxT("* Initialized stack tracing to '%s' for %lu traces."),
				TraceLogFileName, TraceCount);
		}
	}

#endif

	return true;
}

void BugFix::GetFunctionClass(TCHAR* fClass)
{
	_tcsncpy_s(fClass, 128, wxT("BUGFIX"), 4); 
}

void BugFix::SetString(char* sFunction, char* sParam1, int nParam2, char* sValue)
{
	wxLogTrace(TRACE_VERBOSE, wxT("* Plugin SetString(0x%x, %s, %d, %s)"), 0x0, sParam1, nParam2, sValue);
}

char* BugFix::GetString(char* sFunction, char* sParam1, int nParam2)
{
	wxLogTrace(TRACE_VERBOSE, wxT("* Plugin GetString(0x%x, %s, %d)"), 0x0, sParam1, nParam2);
	return NULL;
}

bool BugFix::Check()
{
	//
	// Let's make sure that the nwn2server version matches what we are compiled
	// against.
	//

	__try
	{
		if (*reinterpret_cast< unsigned long * >( OFFS_ProcessServerMessageHook ) != CHECK_ProcessServerMessageHook)
		{
			return false;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}

	return true;
}

void __stdcall BugFix::FreeNwn2Heap(void *p)
{
	if (!p)
		return;

	if (NWN2Heap_Deallocate)
		NWN2Heap_Deallocate( p );
}

#ifdef XP_BUGFIX_USE_SYMBOLS

void __stdcall BugFix::LogStackTrace(
	__in const CONTEXT * Context,
	__in ULONG_PTR TraceContext
	)
{
	//
	// Call the class method.
	//

	if (!plugin->tracer)
		return;

	plugin->tracer->LogTrace( Context, TraceContext );
}

#endif

void __stdcall BugFix::SafeInitPositionList(NWN2::somestruc *struc)
{
	NWN2::position_list  *prev = struc->tail;
	NWN2::position_list  *cur  = prev->next;
	std::set< ULONG_PTR > visited;
	ULONG_PTR             key;
//	bool                  first = true;
//	float                 posY = 0.0f;
	bool                  loop = false;
	bool                  in_free_pool = false;

	try
	{
		for (;;)
		{
			key = reinterpret_cast< ULONG_PTR >( cur );

			//
			// If we see ``deadbeef'', then we're actually working on free list
			// nodes (bad).  The deadbeef marker signifies the end of the free
			// list for the NWN2_MemPool< NWN2_JStar::Node > slab allocator.
			//
			// Because the free list looks sufficiently like the
			// NWN2_JStar::Node::m_pParent linked list, we may traverse into it
			// if we get a node in the free list handed to us.  In this case we
			// should break out.  Note that scribbling over [+04h] in the free
			// list is not typically fatal, as it's 0x24 in size and only the
			// value at [+00h], which we only read and don't write, matters.
			//
			// However, this may interfere with other JStar nodes that have a
			// pointer to a freed node checked out, so it's not ideal.  We'll
			// try this though as it's better than crashing entirely.
			//
			// Eventually, after I have identified all the inlined calls to
			// NWN2_MemPool< NWN2_JStar::Node >::Free, I'll scrap this and just
			// instance the slab allocator per-NWN2_JStar object.
			//

			if (key == 0xDEADBEEF)
			{
				in_free_pool = true;
				loop         = true;
				break;
			}

//			wxLogMessage(wxT("%g %g\n"), cur->pos.y, posY);

			if (!key                                   ||
			    (visited.find( key ) != visited.end()))
			{
//				wxLogMessage( wxT( "* %g < %g, or loop at %p" ), cur->pos.y, posY, cur );
//				wxLogMessage("%d %d\n", cur->pos.y < posY, cur->pos.y > posY);
//				prev->next        = struc->head;
//				struc->head->prev = prev;
				loop              = true;
				break;
			}
			else
			{
				visited.insert( key );
			}

			cur->prev = prev;
			prev      = cur;
//			posY      = cur->pos.y;
//			first     = false;

			if (prev == struc->head)
				break;

			cur       = cur->next;
		}
	}
	catch (std::bad_alloc)
	{
//		prev->next        = struc->head;
//		struc->head->prev = prev;
		loop              = true;
	}

	if (loop)
	{
		ULONG now = GetTickCount();

		if (now - plugin->lastlog > 1000)
		{
			plugin->lastlog = now;

			wxLogMessage( wxT(
				"* SafeInitPositionList: Fixed broken list links (%p, loop detected at @ %p).%s" ),
				struc,
				prev,
				in_free_pool ? " (WARNING: Node list extended into JStar slab allocator free pool!)" : ""
				);
		}
	}
}

void __stdcall BugFix::LogNullDerefCrash0()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash0: Avoided null deference crash #0 (expired frame data?)." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash1()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash1: Avoided null deference crash #1 (party inviter invalid)." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash2()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash2: Avoided null deference crash #2 (no collider data - respawn while polymorphed race?)." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash3()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash3: Avoided null deference crash #3 (DM client toggle plot object)." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash4()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash4: Avoided null deference crash #4 (failed to insert player into game object array)." ) );
	}
/*mov     edx, [eax]
mov     ecx, eax
mov     eax, [edx+68h]
*/
}

void __stdcall BugFix::LogCrash5()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogCrash5: Avoided deference crash #5 (unpacketize expired frame data?)." ) );
	}
/*mov     edx, [eax]
mov     ecx, eax
mov     eax, [edx+68h]
*/
}

void __stdcall BugFix::LogNullDerefCrash6()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash6: Avoided null deference crash #6 (Inventory.UnequipItem while player not zoned in)." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash7()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash7: Avoided null deference crash #7 (ActionExchangeItem script function called on wrong object type)." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash8()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash8: Avoided null deference crash #8 (NULL CItemRepository in item acquisition)." ) );
	}
}

void __stdcall BugFix::LogUncompress0Fix()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogUncompress0Fix: Avoided crash due to invalid compressed data." ) );
	}
}

void __stdcall BugFix::LogUncompress1Fix()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogUncompress1Fix: Avoided crash due to invalid compressed data." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash9()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash9: Avoided null deference crash #9 (Nonexistant object in item repository)." ) );
	}
}

void __stdcall BugFix::LogNullDerefCrash10()
{
	ULONG now = GetTickCount();

	if (now - plugin->lastlog > 1000)
	{
		plugin->lastlog = now;

		wxLogMessage( wxT( "LogNullDerefCrash10: Avoided null dereference crash #10 (Bogus feats during level-up processing)." ) );
	}
}



unsigned long CalcPositionLoop0Ret      = OFFS_CalcPositionLoop0Ret;
unsigned long CalcPositionLoop1Ret      = OFFS_CalcPositionLoop1Ret;
#if NWN2SERVER_VERSION < 0x01211549
unsigned long NullDerefCrash0NormalRet  = OFFS_NullDerefCrash0RetNormal;
unsigned long NullDerefCrash0SkipRet    = OFFS_NullDerefCrash0RetSkip;
unsigned long NullDerefCrash1NormalRet  = OFFS_NullDerefCrash1RetNormal;
unsigned long NullDerefCrash1SkipRet    = OFFS_NullDerefCrash1RetSkip;
#endif
unsigned long NullDerefCrash2NormalRet  = OFFS_NullDerefCrash2RetNormal;
unsigned long NullDerefCrash2SkipRet    = OFFS_NullDerefCrash2RetSkip;
#if NWN2SERVER_VERSION < 0x01211549
unsigned long NullDerefCrash3NormalRet  = OFFS_NullDerefCrash3RetNormal;
unsigned long NullDerefCrash3SkipRet    = OFFS_NullDerefCrash3RetSkip;
#endif
unsigned long NullDerefCrash4NormalRet  = OFFS_NullDerefCrash4RetNormal;
unsigned long NullDerefCrash4SkipRet    = OFFS_NullDerefCrash4RetSkip;
unsigned long Crash5NormalRet           = OFFS_Crash5RetNormal;
unsigned long Crash5SkipRet             = OFFS_Crash5RetSkip;
unsigned long NullDerefCrash6NormalRet  = OFFS_NullDerefCrash6RetNormal;
unsigned long NullDerefCrash6SkipRet    = OFFS_NullDerefCrash6RetSkip;
unsigned long NullDerefCrash7NormalRet  = OFFS_NullDerefCrash7RetNormal;
unsigned long NullDerefCrash7SkipRet    = OFFS_NullDerefCrash7RetSkip;
unsigned long NullDerefCrash8NormalRet  = OFFS_NullDerefCrash8RetNormal;
unsigned long NullDerefCrash8SkipRet    = OFFS_NullDerefCrash8RetSkip;
unsigned long CheckUncompress0NormalRet = OFFS_CheckUncompress0RetNormal;
unsigned long CheckUncompress0SkipRet   = OFFS_CheckUncompress0RetSkip;
unsigned long CheckUncompress1NormalRet = OFFS_CheckUncompress1RetNormal;
unsigned long CheckUncompress1SkipRet   = OFFS_CheckUncompress1RetSkip;
unsigned long UncompressMessage         = OFFS_UncompressMessage;
unsigned long NullDerefCrash9NormalRet  = OFFS_NullDerefCrash9RetNormal;
unsigned long NullDerefCrash9SkipRet    = OFFS_NullDerefCrash9RetSkip;
#if NWN2SERVER_VERSION >= 0x01211549
unsigned long NullDerefCrash10NormalRet = OFFS_NullDerefCrash10RetNormal;
unsigned long NullDerefCrash10SkipRet   = OFFS_NullDerefCrash10RetSkip;
#endif
#if NWN2SERVER_VERSION == 0x01211549
unsigned long CGameEffectDtorRet        = OFFS_CGameEffectDtorRet;
#endif

/*
 * NWN2_JStar::SearchStep
 *
 * - There is a bug here somewhere where we return a NWN2_JStar::Node to the
 *   node allocator pool while it is still in use.  The way this typically
 *   fails is that we get a loop in the list because a node is inserted into
 *   the list twice after being freed and then checked out again for the same
 *   NWN2_JStar instance (failure mode I).  Other failure modes are possible
 *   (bizzare pathing behavior due to alias a "hot" NWN2_JStar::Node from
 *   another active NWN2_JStar instance (failure mode II), or taking a node in
 *   the free list and walking the free list ``Next'' pointers as
 *   NWN2_JStar::Node::m_pParent pointers until we reach the end of pool marker
 *   at 0xDEADBEEF (failure mode III).
 *
 * - N.B. I am considering instancing the allocator pool on a per-NWN2_JStar
 *   instance basis, disabling ``free'' calls, and then just throwing away the
 *   entire list of allocations made by that NWN2_JStar instance in the context
 *   of NWN2_JStar::~NWN2_JStar.  This should fix the problem for good at the
 *   cost of some minor additional memory usage while an NWN2_JStar instance is
 *   performing pathing stepping.  However, given the maximum pathing step
 *   length (300), this seems like it shouldn't be too much of an issue.
 *
 * - To find the actual problem, I recommend replacing the standard
 *   NWN2_MemPool< NWN2_JStar::Node > with a page heap style allocator that
 *   places each node on it's own page and reprotects that page to noaccess
 *   after it is freed to the pool.  This will catch the first use of a freed
 *   NWN2_JStar::Node object.  It may also be useful to log a stack trace with
 *   each allocation.  To use the system page heap to accomplish these tasks,
 *   simply replace NWN2_MemPool< NWN2_JStar::Node >::Free with HeapFree, and
 *   NWN2_MemPool< NWN2_JStar::Node >::Alloc with HeapAlloc, and enable page
 *   heap, which will also conveniently log stack traces as well NWN2 is
 *   rebuilt with /Oy- (so that EBP-based stack traces work).  Note that as
 *   Microsoft builds Windows with /Oy-, I believe that it would be a
 *   reasonable change to make for release builds for all modules, as it
 *   significantly improves debuggability of these issues in conjuction with
 *   tools such as page heap.
 */
__declspec(naked) void BugFix::CalcPositionsLoop0Fix()
{
	__asm
	{
		push    esi
		call    SafeInitPositionList

		jmp     dword ptr [CalcPositionLoop0Ret]
	}
}

/*
 * NWN2_JStar::SearchStep
 *
 * - There is a bug here somewhere where we return a NWN2_JStar::Node to the
 *   node allocator pool while it is still in use.  The way this typically
 *   fails is that we get a loop in the list because a node is inserted into
 *   the list twice after being freed and then checked out again for the same
 *   NWN2_JStar instance (failure mode I).  Other failure modes are possible
 *   (bizzare pathing behavior due to alias a "hot" NWN2_JStar::Node from
 *   another active NWN2_JStar instance (failure mode II), or taking a node in
 *   the free list and walking the free list ``Next'' pointers as
 *   NWN2_JStar::Node::m_pParent pointers until we reach the end of pool marker
 *   at 0xDEADBEEF (failure mode III).
 *
 * - N.B. I am considering instancing the allocator pool on a per-NWN2_JStar
 *   instance basis, disabling ``free'' calls, and then just throwing away the
 *   entire list of allocations made by that NWN2_JStar instance in the context
 *   of NWN2_JStar::~NWN2_JStar.  This should fix the problem for good at the
 *   cost of some minor additional memory usage while an NWN2_JStar instance is
 *   performing pathing stepping.  However, given the maximum pathing step
 *   length (300), this seems like it shouldn't be too much of an issue.
 *
 * - To find the actual problem, I recommend replacing the standard
 *   NWN2_MemPool< NWN2_JStar::Node > with a page heap style allocator that
 *   places each node on it's own page and reprotects that page to noaccess
 *   after it is freed to the pool.  This will catch the first use of a freed
 *   NWN2_JStar::Node object.  It may also be useful to log a stack trace with
 *   each allocation.  To use the system page heap to accomplish these tasks,
 *   simply replace NWN2_MemPool< NWN2_JStar::Node >::Free with HeapFree, and
 *   NWN2_MemPool< NWN2_JStar::Node >::Alloc with HeapAlloc, and enable page
 *   heap, which will also conveniently log stack traces as well NWN2 is
 *   rebuilt with /Oy- (so that EBP-based stack traces work).  Note that as
 *   Microsoft builds Windows with /Oy-, I believe that it would be a
 *   reasonable change to make for release builds for all modules, as it
 *   significantly improves debuggability of these issues in conjuction with
 *   tools such as page heap.
 */
__declspec(naked) void BugFix::CalcPositionsLoop1Fix()
{
	__asm
	{
		push    esi
		call    SafeInitPositionList

		jmp     dword ptr [CalcPositionLoop1Ret]
	}
}

/*
 * CNetLayerWindow::FrameTimeout
 *
 * - We call CExoNetExtendableBuffer::GetFrameData, which fails due to an
 *   expired frame or some other as of yet undetermined condition.  This
 *   routine has an [out] pointer to the frame data.  When the routine
 *   returns false, the [out] pointer is uninitialized, but
 *   CNetLayerWindow::FrameTimeout does not check the return value and will
 *   use an uninitialized buffer pointer (stack based) in this case.
 *
 * - The actual fix for this problem should be:
 *
 *   if (!pBuffer->GetFrameData( ..., &pFrameData, .... ))
 *     return false; // Or perform other error handling, e.g. drop player
 */
__declspec(naked) void BugFix::NullDerefCrash0Fix()
{
#if NWN2SERVER_VERSION < 0x01211549
	__asm
	{
		test    al, al
		jz      Skip

		mov     eax, dword ptr [esp+20h]

;		cmp     eax, 0ffffh
;		jbe     Skip

		movzx   cx, byte ptr [eax+03h]

		jmp     dword ptr [NullDerefCrash0NormalRet]

Skip:
		call    LogNullDerefCrash0
		jmp     dword ptr [NullDerefCrash0SkipRet]
	}
#endif
}

/*
 * CNWSMessage::WriteGameObjUpdate_UpdateObject
 *
 * - We cast a CGameObject as a CNWSCreature and the creature either has no
 *   CNWSCreatureStatus (failure mode I), or the object wasn't a CNWSCreature
 *   and the cast failed (failure mode II).  The object we are referencing is
 *   the object referenced by a player's CNWSCreature::m_oidInvitedToPartyBy,
 *   and the failure condition occurs when the inviter has left the server.
 *
 * - This typically happens when a player invites another player that is in a
 *   zone transfer, then the inviting player quits immediately after.
 *
 * - The actual fix for this problem should be to iterate through all
 *   CNWSCreature objects from CExoServerAppInternal::RemovePCFromWorld, and:
 *
 *   if (pCreature->GetInvitedToParty() && pCreature->GetInvitedToPartyBy()) ==
 *     pExitingPlayer->GetId())
 *   {
 *      pCreature->SetInvitedToParty( false );
 *      pCreature->SetInvitedToPartyBy( INVALID_OBJECT_ID );
 *   }
 *
 *   There may need to be a notification sent to the client to tell it that the
 *   party invitation is expired in a cleaner way as well.
 */
#if NWN2SERVER_VERSION == 0x00131409
__declspec(naked) void BugFix::NullDerefCrash1Fix()
{
	__asm
	{
		test	ebp, ebp
		jz      Skip

		mov     ecx, dword ptr [ebp+01fc4h]
		cmp     ecx, 0ffffh
		jbe     Skip

		jmp     dword ptr [NullDerefCrash1NormalRet]

Skip:
		call    LogNullDerefCrash1
		jmp     dword ptr [NullDerefCrash1SkipRet]
	}
}
#elif NWN2SERVER_VERSION == 0x00121295
__declspec(naked) void BugFix::NullDerefCrash1Fix()
{
	__asm
	{
		test	ebp, ebp
		jz      Skip

		mov     ecx, dword ptr [ebp+1each]
		cmp     ecx, 0ffffh
		jbe     Skip

		jmp     dword ptr [NullDerefCrash1NormalRet]

Skip:
		call    LogNullDerefCrash1
		jmp     dword ptr [NullDerefCrash1SkipRet]
	}
}
#else
__declspec(naked) void BugFix::NullDerefCrash1Fix()
{
	__asm
	{
	}
}
#endif

/*
 * NWN2_Collider::UpdateCollider
 *
 * - We touch NWN2_Collider::m_AABBInfo->m_pAABBMgr->m_pAABBs when in fact the
 *   NWN2_AABBMgr pointer (m_pAABBMgr) is null.
 *
 * - This tends to happen when a creature dies while polymorphed.  I assume
 *   that it's a race condition where the creature is pathed over before we
 *   have loaded collision detection data for the new form when unpolymorphing,
 *   but that isn't proven yet.
 *
 * - Haven't converted stack trace from 1.12 to 1.13 beta to determine the
 *   caller yet.
 */
__declspec(naked) void BugFix::NullDerefCrash2Fix()
{
	__asm
	{
		pop     edi
		test    ecx, ecx
		jz      Skip

		add     eax, dword ptr [ecx+0ch]
		fstp    dword ptr [eax]

		jmp     dword ptr [NullDerefCrash2NormalRet]

Skip:
		call    LogNullDerefCrash2
		jmp     dword ptr [NullDerefCrash2SkipRet]
	}
}

/*
 * CNWSMessage::HandlePlayerToServerDungeonMasterMessage
 *
 * - In the handler for the toggle plot object DM command, we receive an
 *   OBJECT_ID from a DM client, which is a valid CGameObject reference, but
 *   points to an object not derived from CNWSObject (such as a CNWSArea or
 *   CNWSModule).  The code assumes that if CGameObjectArray::GetGameObject
 *   returns for an OBJECT_ID, that CGameObject::AsNWSObject() will always
 *   cast successfully to a CNWSObject.
 *
 * - The actual fix for this problem should be along the lines of the
 *   following:
 *
 *   CNWSObject *pServerObject = pObject->AsNWSObject();
 *
 *   if (!pServerObject)
 *     return false; // Or handle the error appropriately.
 */
__declspec(naked) void BugFix::NullDerefCrash3Fix()
{
#if NWN2SERVER_VERSION < 0x01211549
	__asm
	{
		test    eax, eax
		jz      Skip

		mov     eax, dword ptr [eax+180h]

		jmp     dword ptr [NullDerefCrash3NormalRet]

Skip:
		call    LogNullDerefCrash3
		jmp     dword ptr [NullDerefCrash3SkipRet]
	}
#endif
}

/*
 * CServerExoAppInternal::LoadCharacterStart
 *
 * - We call CNWSPlayer::GetGameObject() for a newly inserted CNWSPlayer
 *   object, but the player wasn't actually inserted into the game object array
 *   and so this the call returns null.  We then go on to reference the
 *   returned CGameObject pointer for a call to CGameObject::AsNWSCreature, for
 *   use in CServerExoAppInternal::ProcessBackwardsCompatibility, but as the
 *   CNWSPlayer had no CGameObject associated with it, we crash.
 *
 * - The real solution should be to handle the failure to insert the CNWSPlayer
 *   into the game object array and if we fail to insert the game object, send
 *   back a PlayerLogin_Fail message.  e.g.:
 *
 *   CGameObject *pGameObject = pPlayerObject->GetGameObject();
 *
 *   if (!pGameObject) // Suggested new check
 *     break; // Fallthrough handles via if (!pPlayerObject->GetGameObject())
 *
 *   CNWSCreature *pCreatureObject = pGameObject->AsNWSCreature();
 *
 *   if (!pCreatureObject) // Suggested new check
 *     break; // Fallthrough handles via if (!pPlayerObject->GetGameObject())
 *
 *   ProcessBackwardsCompatibility( pCreatureObject );
 *
 *   CNWSDungeonMaster *pDMObject = pPlayerObject->AsNWSDungeonMaster();
 *
 *   if (pDMObject)
 *   {
 *     // ... Additional processing ...
 *     break; // All done
 *   }
 *
 * - This may happen when we fail to process a .bic file.
 */
__declspec(naked) void BugFix::NullDerefCrash4Fix()
{
	__asm
	{
		test    eax, eax
		jz      Skip

		mov     edx, dword ptr [eax]
		mov     ecx, eax
		mov     eax, dword ptr [edx+68h]

		jmp     dword ptr [NullDerefCrash4NormalRet]

Skip:
		call    LogNullDerefCrash4

		;
		; Sends a login failure message and cleans up.
		;

		jmp     dword ptr [NullDerefCrash4SkipRet]
	}
}

/*
 * CNetLayerWindow::UnpacketizeFullMessages
 *
 * - We call CExoNetExtendableBuffer::GetFrameData, which fails due to an
 *   expired frame or some other as of yet undetermined condition.  This
 *   routine has an [out] pointer to the frame data.  When the routine
 *   returns false, the [out] pointer is uninitialized, but
 *   CNetLayerWindow::UnpacketizeFullMessages (incorrectly) assumes that
 *   CExoNetExtendableBuffer::GetFrameData signals a failure condition by
 *   setting the [out] pointer to NULL.  Thus, we use an uninitialized
 *   buffer pointer (stack based) in this case.
 *
 * - The actual fix for this problem should be:
 *
 *   if (!pBuffer->GetFrameData( ..., &pFrameData, .... ))
 *     return false; // Or perform other error handling, e.g. drop player
 */
__declspec(naked) void BugFix::Crash5Fix()
{
	__asm
	{
		test    al, al
		jz      Skip

		mov     ecx, dword ptr [esp+14h]

		;
		; This is redundant as the test al, al above will catch it.  However,
		; just to be clear, redoing it.
		;

		test    ecx, ecx

		jmp     dword ptr [Crash5NormalRet]

Skip:
		call    LogCrash5

		;
		; Set the output pointer to null so that the check for failure will
		; work as it was intended to.
		;

		xor     ecx, ecx
		mov     dword ptr [esp+14h], ecx

		test    ecx, ecx

		jmp     dword ptr [Crash5SkipRet]
	}
}


/*
 * CNWSMessage::HandlePlayerToServerInventoryMessage
 *
 * - We call CNWSPlayer::GetGameObject, but fail to handle the contingency
 *   where this returns NULL.
 *
 * - The actual fix to this problem should be:
 *
 *   CGameObject *pGameObject = pPlayer->GetGameObject();
 *
 *   if (!pGameObject) break; // Suggested new check
 *
 */
__declspec(naked) void BugFix::NullDerefCrash6Fix()
{
	__asm
	{
		test    eax, eax
		jz      Skip

		mov     edx, dword ptr [eax]
		mov     ecx, eax
		mov     eax, dword ptr [edx+068h]

		jmp     dword ptr [NullDerefCrash6NormalRet]

Skip:
		call    LogNullDerefCrash6

		;
		; Send an unequip failure message to the client.
		;

		jmp     dword ptr [NullDerefCrash6SkipRet]
	}
}


/*
 * CNWVirtualMachineCommands::ActionExchangeItem
 *
 * - We call CGameObject::AsNWSObject(), but do not handle the case where the
 *   cast to CNWSObject fails (e.g. an environmental object was provided).
 *
 * - The actual fix to this problem should be (note that there are two if
 *   branches that must be altered, or the AsNWSObject() cast should be moved
 *   before the if fork):
 *
 *   CNWSObject *pNWSObject = pGameObject->AsNWSObject();
 *
 *   if (!pNWSObject) break; // Suggested new check
 *
 */
__declspec(naked) void BugFix::NullDerefCrash7Fix()
{
	__asm
	{
		;
		; Must preserve ecx/eax across the call, edx is not read before it is
		; rewritten beyond this point.
		;

		push    ecx
		push    eax

		;
		; Cast to CNWSObject with a check for failure.  We will still run the
		; unchecked casts afterwards, but this pre-check is sufficient as we do
		; not support concurrency in the scripting environment.
		;

		mov     ecx, esi
		mov     edx, dword ptr [esi]
		call    dword ptr [edx+03ch]
		test    eax, eax
		pop     eax
		pop     ecx

		jz      Skip

		mov     cl, byte ptr [eax+03e0h]

		jmp     dword ptr [NullDerefCrash7NormalRet]

Skip:

		call    LogNullDerefCrash7

		;
		; Fail the script VM call.
		;

		jmp     dword ptr [NullDerefCrash7SkipRet]
	}
}


/*
 * CNWSItem::AcquireItem
 *
 * - We do not check that the object's CItemRepository exists.
 *
 * - The actual fix to this problem should be:
 *
 *   CItemRepository *pItemRepository = ...;
 *
 *   if (!pItemRepository) break; // Suggested new check
 *
 *   pItemRepository->FindBlankPosition( ... );
 *
 */
__declspec(naked) void BugFix::NullDerefCrash8Fix()
{
	__asm
	{
		test    ebp, ebp
		jz      Skip

		mov     ecx, dword ptr [edi]
		push    01h
		push    00h

		jmp     dword ptr [NullDerefCrash8NormalRet]

Skip:
		call    LogNullDerefCrash8

		;
		; Fail the item acquisition.
		;

		jmp     dword ptr [NullDerefCrash8SkipRet]
	}
}

/*
 * CNetLayerWindow::UnpacketizeFullMessages
 *
 * - We don't check that the frame chunk length is within bounds.
 * - We don't check that zlib inflate succeeds.
 *
 * - The actual fix should check that the frame chunk length is within the
 *   length of the remaining packet buffer, and that the uncompress call
 *   succeeds.
 */
__declspec(naked) void BugFix::Uncompress0Fix()
{
	__asm
	{
		cmp     edi, ebx
		jnbe    Skip

		mov     ecx, dword ptr [esi+04h]
		push    edi
		push    eax
		mov     eax, dword ptr [esi+0ch]
		push    eax
		call    dword ptr [UncompressMessage]

		test    eax, eax
		jz      Skip

		jmp     dword ptr [CheckUncompress0NormalRet]

Skip:
		call    LogUncompress0Fix

		;
		; Drop the message.
		;

		jmp     dword ptr [CheckUncompress0SkipRet]
	}
}

/*
 * CNetLayerWindow::UnpacketizeFullMessages
 *
 * - We don't check that zlib inflate succeeds.
 *
 * - The actual fix should check that the uncompress call succeeds.
 */
__declspec(naked) void BugFix::Uncompress1Fix()
{
	__asm
	{
		mov     edx, dword ptr [esi+0ch]
		mov     ecx, dword ptr [esi+04h]
		push    ebx
		push    ebp
		push    edx
		call    dword ptr [UncompressMessage]

		test    eax, eax
		jz      Skip

		jmp     dword ptr [CheckUncompress1NormalRet]

Skip:
		push    ebp
		call    FreeNwn2Heap

		call    LogUncompress1Fix

		;
		; Drop the message.
		;

		jmp     dword ptr [CheckUncompress1SkipRet]
	}
}

/*
 * CItemRepository::GetItemPtrInRepository
 *
 * - We do not check that GetItemByGameObjectID returns a non-NULL CNWSItem*.
 *
 * - The actual fix to this problem should be:
 *
 *   CNWSItem *pItem = pServer->GetItemByGameObjectID( ObjectID );
 *
 *   if (!pItem) continue; // Suggested new check
 *
 * - Additionally, why the CItemRepository had an invalid object id in it
 *   should be looked into as well.  This is suspicious, perhaps there needs to
 *   be a check to remove an item from all item repositories when a CNWSItem is
 *   deleted.
 *
 */
__declspec(naked) void BugFix::NullDerefCrash9Fix()
{
	__asm
	{
		test    eax, eax
		jz      Skip

		mov     edi, eax
		mov     eax, dword ptr [edi+04h]

		jmp     dword ptr [NullDerefCrash9NormalRet]

Skip:
		call    LogNullDerefCrash9

		;
		; Continue on to the next item.
		;

		jmp     dword ptr [NullDerefCrash9SkipRet]
	}
}

/*
 * CNWSCreatureStats::ValidateLevelUp
 *
 * - We don't handle the case where CNWSFeat::GetFeat returns NULL.  This
 *   results in a crash during level-up processing (inside
 *   CNWSRules::IgnoreValidation) if a client specifies a bogus feat identifier
 *   in their level-up packet.
 *
 * - The actual fix to this problem should be:
 *
 *    CNWSFeat *pFeat = g_pRules->GetFeat( FeatId );
 *
 *    if (!pFeat) return LEVELUP_FAIL_BAD_FEAT;
 *
 */
__declspec(naked) void BugFix::NullDerefCrash10Fix()
{
	__asm
	{
		test    eax, eax
		jz      Skip

		cmp     dword ptr [esp+028h], ebx
		mov     ebp, eax

		jmp     dword ptr [NullDerefCrash10NormalRet]

Skip:
		call    LogNullDerefCrash10

		;
		; Fail the level-up.
		;

		jmp     dword ptr [NullDerefCrash10SkipRet]
	}
}

/*
 * CGameEffect::~CGameEffect
 *
 * - Log stack traces to catch who deletes a still-live CGameEffect.
 *
 */
__declspec(naked) void BugFix::CGameEffectDtorLogger()
{
#if NWN2SERVER_VERSION == 0x01211549
	__asm
	{
		sub     esp, SIZE CONTEXT

		mov     dword ptr [esp]CONTEXT.ContextFlags, CONTEXT_CONTROL | CONTEXT_INTEGER
		mov     word ptr [esp]CONTEXT.SegSs, ss
		mov     word ptr [esp]CONTEXT.SegCs, cs
		mov     dword ptr [esp]CONTEXT.Esp, esp
		add     dword ptr [esp]CONTEXT.Esp, SIZE CONTEXT
		mov     dword ptr [esp]CONTEXT.Ebp, ebp
		mov     dword ptr [esp]CONTEXT.Eax, eax
		mov     dword ptr [esp]CONTEXT.Ebx, ebx
		mov     dword ptr [esp]CONTEXT.Ecx, ecx
		mov     dword ptr [esp]CONTEXT.Edx, edx
		mov     dword ptr [esp]CONTEXT.Edi, edi
		mov     dword ptr [esp]CONTEXT.Esi, esi
		mov     dword ptr [esp]CONTEXT.Eip, OFFS_CGameEffectDtor
		pushfd
		pop     eax
		mov     dword ptr [esp]CONTEXT.EFlags, eax

		lea     eax, dword ptr [esp]
		push    ecx
		push    eax
		call    LogStackTrace

		mov     eax, OFFS_ms_iGameEffectCount
		dec     dword ptr [eax]

		mov     eax, dword ptr [esp]CONTEXT.Eax
		mov     ecx, dword ptr [esp]CONTEXT.Ecx

		add     esp, SIZE CONTEXT

		push    ecx

		jmp     dword ptr [CGameEffectDtorRet]
	}
#endif
}