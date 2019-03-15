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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_PHYSWORLD_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_PHYSWORLD_H
#pragma once


#include <primitives.h>
#include <physinterface.h>

#include "MathHelpers.h"
#include <CryLibrary.h>

#if defined(AZ_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   // HMODULE, LoadLibraryA()
#endif

class CPhysWorldLoader
{
public:
    CPhysWorldLoader()
        : m_hPhysics(0)
        , m_pPhysWorld(0)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        MathHelpers::AutoFloatingPointExceptions autoFpe(0);
#endif
        AZStd::string libName = GetLibName();

        // Need to use the library from the "rc" folder
#ifdef CGF_PHYSX_COMPILER
        libName = "rc/" + libName;
#endif

        const char* const pName = libName.c_str();

        RCLog("Loading %s...", pName);

        m_hPhysics = CryLoadLibrary(pName);

        if (!m_hPhysics)
        {
            const DWORD errCode = ::GetLastError();

            char messageBuffer[1024] = { '?', 0  };
#if defined(AZ_PLATFORM_WINDOWS)
            ::FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                errCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                messageBuffer,
                sizeof(messageBuffer) - 1,
                NULL);
#endif
            RCLogError("%s failed to load. Error code: 0x%x (%s)", pName, errCode, messageBuffer);
            return;
        }

        RCLog("  Loaded %s", pName);

        IPhysicalWorld*(* pfnCreatePhysicalWorld)(void* pSystem) =
            (IPhysicalWorld * (*)(void*))CryGetProcAddress(m_hPhysics, "CreatePhysicalWorld");

        if (!pfnCreatePhysicalWorld)
        {
            RCLogError("Physics is not initialized: failed to find CreatePhysicalWorld()");
            return;
        }

        m_pPhysWorld = pfnCreatePhysicalWorld(0);

        if (!m_pPhysWorld)
        {
            RCLogError("Physics is not initialized: a failure in CreatePhysicalWorld()");
            return;
        }

        m_pPhysWorld->Init();
    }

    ~CPhysWorldLoader()
    {
        if (m_pPhysWorld)
        {
            m_pPhysWorld->Release();
        }

        if (m_hPhysics)
        {
            const char* const pName = GetLibName();

            RCLog("Unloading %s...", pName);

            CryFreeLibrary(m_hPhysics);

            RCLog("  Unloaded %s", pName);
        }
    }

    IPhysicalWorld* GetWorldPtr()
    {
        return m_pPhysWorld;
    }

private:
    static const char* GetLibName()
    {
        return CryLibraryDefName("CryPhysicsRC");
    }

private:
    HMODULE m_hPhysics;
    IPhysicalWorld* m_pPhysWorld;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_PHYSWORLD_H
