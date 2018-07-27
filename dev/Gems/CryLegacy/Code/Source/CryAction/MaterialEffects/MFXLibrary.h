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

// Description : A library which contains a set of MFXEffects


#ifndef _MFX_LIBRARY_H_
#define _MFX_LIBRARY_H_

#pragma once

#include "MFXContainer.h"

class CMFXLibrary
{
private:
    typedef std::map<TMFXNameId, TMFXContainerPtr, stl::less_stricmp<string> > TEffectContainersMap;

public:
    struct SLoadingEnvironment
    {
    public:

        SLoadingEnvironment(const TMFXNameId& _libraryName, const XmlNodeRef& _libraryParamsNode, std::vector<TMFXContainerPtr>& _allContainers)
            : libraryName(_libraryName)
            , libraryParamsNode(_libraryParamsNode)
            , allContainers(_allContainers)
        {
        }

        void  AddLibraryContainer(const TMFXNameId& _libraryName, TMFXContainerPtr _pContainer)
        {
            _pContainer->SetLibraryAndId(_libraryName, allContainers.size());
            allContainers.push_back(_pContainer);
        }

        const TMFXNameId& libraryName;
        const XmlNodeRef& libraryParamsNode;

    private:
        std::vector<TMFXContainerPtr>& allContainers;
    };

public:

    void LoadFromXml(SLoadingEnvironment& loadingEnvironment);
    TMFXContainerPtr FindEffectContainer(const char* effectName) const;
    void GetMemoryUsage(ICrySizer* pSizer) const;

private:

    bool AddContainer(const TMFXNameId& effectName, TMFXContainerPtr pContainer);

private:

    TEffectContainersMap m_effectContainers;
};

#endif // _MFX_LIBRARY_H_

