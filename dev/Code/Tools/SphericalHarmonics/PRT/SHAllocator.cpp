/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "stdafx.h"
#include <PRT/SHAllocator.h>
#include <PRT/ISHLog.h> //top be able to log
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


#undef min
#undef max


extern NSH::ISHLog& GetSHLog();

#ifdef _WIN32
#if defined(USE_MEM_ALLOCATOR)
static HMODULE gsSystem = 0;
static FNC_SHMalloc gspSHAllocFnc = 0;
static FNC_SHFreeSize gspSHFreeSizeFnc = 0;
#endif

#if defined(USE_MEM_ALLOCATOR)
//instantiate simple byte allocator
CSHAllocator<unsigned char> gsByteAllocator;

void LoadAllocatorModule(FNC_SHMalloc& rpfMalloc, FNC_SHFreeSize& rpfFreeSize)
{
    if (!gsSystem)
    {
        CRITICAL_SECTION cs;
        InitializeCriticalSection(&cs);
        EnterCriticalSection(&cs);
#if defined(_WIN64)
        gsSystem = LoadLibrary("SHAllocator.dll");
#else
        gsSystem = LoadLibrary("SHAllocator.dll");
#endif
        if (!gsSystem)
        {
            //now assume current dir is one level below
#if defined(_WIN64)
    #if (_MSC_VER >= 1910)
            gsSystem = LoadLibrary("Bin64vc141/SHAllocator.dll");
    #elif (_MSC_VER >= 1900)
            gsSystem = LoadLibrary("Bin64vc140/SHAllocator.dll");
    #else
            gsSystem = LoadLibrary("Bin64vc120/SHAllocator.dll");
    #endif
#else
            gsSystem = LoadLibrary("bin32/SHAllocator.dll");
#endif
        }
        if (!gsSystem)
        {
            //support rc, is one level above
#if defined(_WIN64)
            gsSystem = LoadLibrary("../SHAllocator_x64.dll");
#else
            gsSystem = LoadLibrary("../SHAllocator.dll");
#endif
        }
        gspSHAllocFnc           = (FNC_SHMalloc)GetProcAddress((HINSTANCE)gsSystem, "SHMalloc");
        gspSHFreeSizeFnc    = (FNC_SHFreeSize)GetProcAddress((HINSTANCE)gsSystem, "SHFreeSize");
        if (!gsSystem || !gspSHAllocFnc || !gspSHFreeSizeFnc)
        {
            char szMasterCDDir[256];
            GetCurrentDirectory(256, szMasterCDDir);
            GetSHLog().LogError("Could not access SHAllocator.dll (check working dir, currently: %s)\n", szMasterCDDir);
            std::string buf("Could not access SHAllocator.dll (check working dir, currently: ");
            buf += szMasterCDDir;
            buf += ")";
            MessageBox(NULL, buf.c_str(), "PRT Memory Manager", MB_OK);
            if (gsSystem)
            {
                ::FreeLibrary(gsSystem);
            }
            exit(1);
        }
        ;
        LeaveCriticalSection(&cs);
    }
    rpfFreeSize = gspSHFreeSizeFnc;
    rpfMalloc = gspSHAllocFnc;
}
#endif
#endif
