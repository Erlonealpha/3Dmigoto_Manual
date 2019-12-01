#pragma once

#define INITGUID
#define NOMINMAX

// Windows Header Files
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include <fstream>
#include <set>
#include <unordered_map>
#include <stdint.h>

using namespace std;

#include <d3d9.h>
#include <nvapi.h>
#include "../PointerSet.h"
#include "DrawCallInfo.h"
#include <nvstereo.h>
#include "Globals.h"
#include "Overlay.h"

namespace D3D9Wrapper
{
#include "d3d9Wrapper.h"
}
void NvAPIOverride();
void NvAPIEnableStereoActiveTracking();
void NvAPIEnableConvergenceTracking();
void NvAPIEnableSeparationTracking();
void NvAPIEnableEyeSeparationTracking();
void NvAPIResetStereoActiveTracking();
void NvAPIResetConvergenceTracking();
void NvAPIResetSeparationTracking();
void NvAPIResetEyeSeparationTracking();

// FIXME: This ifdef UNICODE looks wrong. What we are compiled with has
// absolutely diddly squat to do with what the game was compiled with or which
// version it calls. Plus, why is the ANSI version using wide string types? -DSS
//
// Duplicated the extern "C" junk from the DX11 project for the sake of minimal
// diffs, but it's really not necessary since no other module will be trying to
// resolve our internal symbol names... especially since these aren't exported.
#ifdef UNICODE
extern "C" HMODULE (__stdcall *fnOrigLoadLibraryExW)(
	_In_       LPCWSTR lpLibFileName,
	_Reserved_ HANDLE  hFile,
	_In_       DWORD   dwFlags
	);
extern "C" HMODULE(__stdcall *fnOrigLoadLibraryW)(_In_ LPCWSTR lpLibFileName);

extern "C" HMODULE __stdcall Hooked_LoadLibraryExW(_In_ LPCWSTR lpLibFileName, _Reserved_ HANDLE hFile, _In_ DWORD dwFlags);
extern "C" HMODULE __stdcall Hooked_LoadLibraryW(_In_ LPCWSTR lpLibFileName);
#else
extern "C" HMODULE(__stdcall *fnOrigLoadLibraryExA)(_In_ LPCWSTR lpLibFileName,
	_Reserved_ HANDLE hFile, _In_ DWORD dwFlags);
extern "C" HMODULE(__stdcall *fnOrigLoadLibraryA)(_In_ LPCWSTR lpLibFileName);

extern "C" HMODULE __stdcall Hooked_LoadLibraryExA(_In_ LPCWSTR lpLibFileName, _Reserved_ HANDLE hFile, _In_ DWORD dwFlags);
extern "C" HMODULE __stdcall Hooked_LoadLibraryA(_In_ LPCWSTR lpLibFileName);
#endif
