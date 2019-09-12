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

#if defined(USE_MEM_ALLOCATOR)

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Component/ComponentApplicationBus.h>

static AZStd::unique_ptr<AZ::DynamicModuleHandle> gsSystem = 0;
static FNC_SHMalloc gspSHAllocFnc = 0;
static FNC_SHFreeSize gspSHFreeSizeFnc = 0;

//instantiate simple byte allocator
CSHAllocator<unsigned char> gsByteAllocator;

void LoadAllocatorModule(FNC_SHMalloc& rpfMalloc, FNC_SHFreeSize& rpfFreeSize)
{
    if (!gsSystem)
    {
        AZ::OSString fullLibraryName = AZ_DYNAMIC_LIBRARY_PREFIX "SHAllocator" AZ_DYNAMIC_LIBRARY_EXTENSION;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, fullLibraryName);

        gsSystem = AZ::DynamicModuleHandle::Create(fullLibraryName.c_str());
        if (gsSystem && gsSystem->Load(false))
        {
            gspSHAllocFnc = gsSystem->GetFunction<FNC_SHMalloc>("SHMalloc");
            gspSHFreeSizeFnc = gsSystem->GetFunction<FNC_SHFreeSize>("SHFreeSize");
            if (!gsSystem || !gspSHAllocFnc || !gspSHFreeSizeFnc)
            {
                AZ_Error("PRT/SHAllocator", false, "Could not access SHAllocator.dll (current path: %s)\n", fullLibraryName.c_str());
                exit(1);
            }
        }
    }
    rpfFreeSize = gspSHFreeSizeFnc;
    rpfMalloc = gspSHAllocFnc;
}
#endif
