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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBINDING_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBINDING_H
#pragma once

class CScriptableBase;

//////////////////////////////////////////////////////////////////////////
// **************************************************************************************
// DEPRECATED: This class is currently deprecated and will be removed in release 1.13
// **************************************************************************************
class CScriptBindings
{
public:
    CScriptBindings();
    virtual ~CScriptBindings();

    void Init(ISystem* pSystem, IScriptSystem* pSS);
    void Done();

    void LoadScriptedSurfaceTypes(const char* sFolder, bool bReload);
    virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

private:
    std::vector<std::unique_ptr<CScriptableBase> > m_binds;

    class CScriptSurfaceTypesLoader* m_pScriptSurfaceTypes;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBINDING_H

