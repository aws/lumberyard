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

#ifndef CRYINCLUDE_CRYCOMMON_CRYMODULEDEFS_H
#define CRYINCLUDE_CRYCOMMON_CRYMODULEDEFS_H
#pragma once

#pragma message("CryModuleDefs.h is deprecated in Lumberyard 1.11, you do not need to define eCryModule")

enum ECryModule
{
    eCryM_Local = 0,
    eCryM_3DEngine,
    eCryM_Action,
    eCryM_AISystem,
    eCryM_Animation,
    eCryM_DynamicResponseSystem,
    eCryM_EntitySystem,
    eCryM_Font,
    eCryM_Input,
    eCryM_Movie,
    eCryM_Network,
    eCryM_Physics,
    eCryM_ScriptSystem,
    eCryM_SoundSystem,
    eCryM_System,
    eCryM_Game,
    eCryM_Render,
    eCryM_Launcher,
    eCryM_Editor,
    eCryM_UnitTests,
    eCryM_AudioImpl,
    eCryM_AudioUnitTests,
    eCryM_UI,
    eCryM_AWS,
    eCryM_Num,
};

#endif // CRYINCLUDE_CRYCOMMON_CRYMODULEDEFS_H
