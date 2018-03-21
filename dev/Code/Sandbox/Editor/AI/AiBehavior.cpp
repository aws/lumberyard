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

#include "StdAfx.h"
#include "AiBehavior.h"
#include "../Util/FileUtil.h"
#include "IScriptSystem.h"

//////////////////////////////////////////////////////////////////////////
void CAIBehavior::ReloadScript()
{
    // Execute script file in script system.
    if (m_script.isEmpty())
    {
        return;
    }

    if (CFileUtil::CompileLuaFile(GetScript().toUtf8().data()))
    {
        IScriptSystem* scriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();
        // Script compiled succesfully.
        scriptSystem->ReloadScript(m_script.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIBehavior::Edit()
{
    CFileUtil::EditTextFile(GetScript().toUtf8().data());
}


//////////////////////////////////////////////////////////////////////////
void CAICharacter::ReloadScript()
{
    // Execute script file in script system.
    if (m_script.isEmpty())
    {
        return;
    }

    if (CFileUtil::CompileLuaFile(GetScript().toUtf8().data()))
    {
        IScriptSystem* scriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();
        // Script compiled succesfully.
        scriptSystem->ReloadScript(m_script.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CAICharacter::Edit()
{
    CFileUtil::EditTextFile(GetScript().toUtf8().data());
}
