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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SURFACETYPES_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SURFACETYPES_H
#pragma once

#include <IScriptSystem.h>

struct ISurfaceType;

//////////////////////////////////////////////////////////////////////////
// SurfaceTypes loader.
//////////////////////////////////////////////////////////////////////////
class CScriptSurfaceTypesLoader
{
public:
    CScriptSurfaceTypesLoader();
    ~CScriptSurfaceTypesLoader();

    void ReloadSurfaceTypes();
    bool LoadSurfaceTypes(const char* sFolder, bool bReload);
    void UnloadSurfaceTypes();

    void UnregisterSurfaceType(ISurfaceType* sfType);

private:
    std::vector<ISurfaceType*> m_surfaceTypes;
    std::vector<string> m_folders;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SURFACETYPES_H
