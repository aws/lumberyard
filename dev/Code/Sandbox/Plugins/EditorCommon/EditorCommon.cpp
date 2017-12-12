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
#include "stdafx.h"
#include "EditorCommon.h"

#include "EditorCommonAPI.h"
#include "platform_impl.h"

#ifdef KDAB_MAC_PORT
// needed for AfxEnableMemoryLeakDump
#include <afx.h>
#endif

CEditorCommonApp::CEditorCommonApp()
{
}


void EDITOR_COMMON_API InitializeEditorCommon(IEditor* editor)
{
}

void EDITOR_COMMON_API UninitializeEditorCommon()
{
}

void EDITOR_COMMON_API InitializeEditorCommonISystem(ISystem* pSystem)
{
    ModuleInitISystem(pSystem, "EditorCommon");
}

void EDITOR_COMMON_API UninitializeEditorCommonISystem(ISystem* pSystem)
{
    // Turning off MFC memory dump.
    // It's not useful for any reason.
    // 
    // Basically, when anything links with MFC, MFC installs its own memory allocators which take over all memory allocations.That's fine if your entire application is MFC, but right now, since we're trying to remove MFC, our main application is not an MFC application, but some of our plugins are still using MFC.
    // 
    // So our Editor application loads up, allocates a bunch of stuff, then loads the plugins, after which, MFC takes over memory management.Then, when we unload the plugins, MFC gets unloaded and does it's memory dump there. Because it tracks everything, even things that shouldn't have been freed.
    // 
    // Yes, we have memory leaks.And yes, this would have been an issue before MFC was removed from the main editor application... Except that we do TerminateProcess before the leaks get reported. (That's another thing to be fixed, but not at this exact moment). So the memory leaks being reported only became an issue since MFC was removed in the Editor, because the leak dump is happening before TerminateProcess instead of after now.
    // 
    // I'm putting the thing to turn off MFC leaks into a semi-random plugin that links against MFC. This might come back if EditorCommon stops using MFC and other plugins still do. Hopefully when we get rid of MFC for EditorCommon, we get rid of it for everything.
#ifdef KDAB_MAC_PORT
#if defined(_DEBUG)
    AfxEnableMemoryLeakDump(FALSE);
#endif
#endif

    ModuleShutdownISystem(pSystem);
}
