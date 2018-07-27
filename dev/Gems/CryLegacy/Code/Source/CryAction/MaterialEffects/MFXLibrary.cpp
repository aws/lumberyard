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

#include "CryLegacy_precompiled.h"
#include "MFXLibrary.h"

namespace MaterialEffectsUtils
{
    CMFXContainer* CreateContainer()
    {
        if (gEnv->IsDedicated())
        {
            return new CMFXDummyContainer();
        }
        else
        {
            return new CMFXContainer();
        }
    }
}

void CMFXLibrary::LoadFromXml(SLoadingEnvironment& loadingEnvironment)
{
    CryLogAlways("[MFX] Loading FXLib '%s' ...", loadingEnvironment.libraryName.c_str());

    INDENT_LOG_DURING_SCOPE();

    for (int i = 0; i < loadingEnvironment.libraryParamsNode->getChildCount(); ++i)
    {
        XmlNodeRef currentEffectNode = loadingEnvironment.libraryParamsNode->getChild(i);
        if (!currentEffectNode)
        {
            continue;
        }

        TMFXContainerPtr pContainer = MaterialEffectsUtils::CreateContainer();
        pContainer->BuildFromXML(currentEffectNode);

        const TMFXNameId& effectName = pContainer->GetParams().name;
        const bool effectAdded = AddContainer(effectName, pContainer);
        if (effectAdded)
        {
            loadingEnvironment.AddLibraryContainer(loadingEnvironment.libraryName, pContainer);
        }
        else
        {
            GameWarning("[MFX] Effect (at line %d) '%s:%s' already present, skipping redefinition!", currentEffectNode->getLine(), loadingEnvironment.libraryName.c_str(), effectName.c_str());
        }
    }
}

bool CMFXLibrary::AddContainer(const TMFXNameId& effectName, TMFXContainerPtr pContainer)
{
    TEffectContainersMap::const_iterator it = m_effectContainers.find(effectName);
    if (it != m_effectContainers.end())
    {
        return false;
    }

    m_effectContainers.insert(TEffectContainersMap::value_type(effectName, pContainer));
    return true;
}

TMFXContainerPtr CMFXLibrary::FindEffectContainer(const char* effectName) const
{
    TEffectContainersMap::const_iterator it = m_effectContainers.find(CONST_TEMP_STRING(effectName));

    return (it != m_effectContainers.end()) ? it->second : TMFXContainerPtr(NULL);
}

void CMFXLibrary::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_effectContainers);
}