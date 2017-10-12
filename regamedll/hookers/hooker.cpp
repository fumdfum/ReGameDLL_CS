/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

HIDDEN Module g_GameDLLModule = { NULL, NULL, NULL, NULL };
HIDDEN Module g_EngineModule = { NULL, NULL, NULL, NULL };

// Offset where module assumed be loaded to ajust hooks offsets. NULL for the Linux to trigger symbols searching.
#ifdef _WIN32
const size_t g_BaseOffset = 0x01D00000;
#else
const size_t g_BaseOffset = NULL;
#endif

VirtualTableRef *GetVirtualTableRefAddr(const char *szClassName)
{
	VirtualTableRef *refData = g_TableRefs;
	while (refData->symbolName != NULL)
	{
		if (!strcmp(refData->symbolName, szClassName))
		{
			if (refData->originalAddress != NULL)
			{
				return refData;
			}
		}
		refData++;
	}

	return NULL;
}

bool GetAddressUsingHook(size_t addr)
{
	for (FunctionHook *cfh = &g_FunctionHooks[0]; cfh->symbolName; cfh++)
	{
		if (addr == cfh->originalAddress)
		{
			return true;
		}
	}
	return false;
}

size_t OffsetToRebase(size_t addr)
{
	return (addr - g_GameDLLModule.base + g_BaseOffset);
}

void *GetFunctionEntity(const char *szClassName)
{
	return Sys_GetProcAddress((HMODULE)g_GameDLLModule.base, szClassName);
}

void printAddrRebase(size_t addr, const char *funcName)
{
	static int inum = 0;
	addr += g_BaseOffset - g_GameDLLModule.base;
	printf("#%d. %s - 0x%p\n", ++inum, funcName, (void *)addr);
}

FunctionHook *GetFunctionPtrByName(const char *funcName)
{
	for (FunctionHook *cfh = &g_FunctionHooks[0]; cfh->symbolName; cfh++)
	{
		if (!strcmp(cfh->symbolName, funcName))
			return cfh;
	}

	return NULL;
}

void *GetOriginalFuncAddrOrDie(const char *funcName)
{
	for (FunctionHook *cfh = &g_FunctionHooks[0]; cfh->symbolName; cfh++)
	{
		if (!strcmp(cfh->symbolName, funcName))
			return (void*) cfh->originalAddress;
	}

	Sys_Error("%s: Could not find function '%s'", __func__, funcName);
	return NULL;
}

void *GetOriginalFuncAddrOrDefault(const char *funcName, void *def)
{
	for (FunctionHook *cfh = &g_FunctionHooks[0]; cfh->symbolName; cfh++)
	{
		if (!strcmp(cfh->symbolName, funcName))
			return (void*)cfh->originalAddress;
	}

	return def;
}

void *GetFuncRefAddrOrDie(const char *funcName)
{
	for (AddressRef *cfh = &g_FunctionRefs[0]; cfh->symbolName; cfh++)
	{
		if (!strcmp(cfh->symbolName, funcName))
			return (void*)cfh->originalAddress;
	}

	Sys_Error("%s: Could not find function '%s'", __func__, funcName);
	return NULL;
}

void *GetFuncRefAddrOrDefault(const char *funcName, void *def)
{
	for (AddressRef *cfh = &g_FunctionRefs[0]; cfh->symbolName; cfh++)
	{
		if (!strcmp(cfh->symbolName, funcName))
			return (void*)cfh->originalAddress;
	}

	return def;
}

int HookGameDLL(size_t gameAddr, size_t engAddr)
{
	if (gameAddr == NULL
		|| !FindModuleByAddress(gameAddr, &g_GameDLLModule)
		|| !FindModuleByAddress(engAddr, &g_EngineModule))
	{
		return FALSE;
	}

	// Find all addresses
	bool success = true;

	AddressRef *refData = g_DataRefs;
	while (refData->symbolName != NULL)
	{
		if (!GetAddress(&g_GameDLLModule, (Address *)refData, g_BaseOffset))
		{
			UTIL_Log("%s: symbol not found \"%s\", symbol index: %i", __func__, refData->symbolName, refData->symbolIndex);
			success = false;
		}
		refData++;
	}

	AddressRef *refFunc = g_FunctionRefs;
	while (refFunc->symbolName != NULL)
	{
		if (!GetAddress(&g_GameDLLModule, (Address *)refFunc, g_BaseOffset))
		{
			UTIL_Log("%s: symbol not found \"%s\", symbol index: %i", __func__, refFunc->symbolName, refFunc->symbolIndex);
			success = false;
		}
		refFunc++;
	}

	FunctionHook *hookFunc = g_FunctionHooks;
	while (hookFunc->handlerFunc != NULL)
	{
		if (!GetAddress(&g_GameDLLModule, (Address*)hookFunc, g_BaseOffset))
		{
			UTIL_Log("%s: symbol not found \"%s\", symbol index: %i", __func__, hookFunc->symbolName, hookFunc->symbolIndex);
			success = false;
		}
		hookFunc++;
	}

	VirtualTableRef *refVtbl = g_TableRefs;
	while (refVtbl->symbolName != NULL)
	{
		if (!GetAddress(&g_GameDLLModule, (Address *)refVtbl, g_BaseOffset))
		{
			UTIL_Log("%s: symbol not found \"%s\"", __func__, refVtbl->symbolName);
			success = false;
		}
		refVtbl++;
	}

	if (!success)
	{
		UTIL_Log("%s: failed to hook gamedll!", __func__);
		return FALSE;
	}

	refData = g_DataRefs;
	while (refData->addressRef != NULL)
	{
		if (!FindDataRef(&g_GameDLLModule, refData))
			return FALSE;
		refData++;
	}

	refFunc = g_FunctionRefs;
	while (refFunc->addressRef != NULL)
	{
		if (!FindDataRef(&g_GameDLLModule, refFunc))
			return FALSE;
		refFunc++;
	}

	// Actually hook all things
	hookFunc = g_FunctionHooks;
	while (hookFunc->handlerFunc != NULL)
	{
		if (!HookFunction(&g_GameDLLModule, hookFunc))
			return FALSE;

		hookFunc++;
	}

	return TRUE;
}

#ifdef _WIN32

void *malloc_wrapper(size_t n)
{
	return malloc(n);
}

void *_nh_malloc_wrapper(size_t n)
{
	return malloc(n);
}

void *realloc_wrapper(void *memblock, size_t size)
{
	return realloc(memblock, size);
}

void free_wrapper(void *p)
{
	free(p);
}

void *calloc_wrapper(size_t n, size_t s)
{
	return calloc(n, s);
}

char *strdup_wrapper(const char *s)
{
	return _strdup(s);
}

int rand_wrapper()
{
	return rand();
}

#endif // _WIN32
