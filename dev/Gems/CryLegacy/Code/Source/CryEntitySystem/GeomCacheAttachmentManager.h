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

// Description : Manages geometry cache attachments


#ifndef CRYINCLUDE_CRYENTITYSYSTEM_GEOMCACHEATTACHMENTMANAGER_H
#define CRYINCLUDE_CRYENTITYSYSTEM_GEOMCACHEATTACHMENTMANAGER_H
#pragma once

#if defined(USE_GEOM_CACHES)
struct IGeomCacheRenderNode;

#include <VectorMap.h>

class CGeomCacheAttachmentManager
{
public:
    void Update();

    void RegisterAttachment(IEntity* pChild, IEntity* pParent, const uint32 targetHash);
    void UnregisterAttachment(const IEntity* pChild, const IEntity* pParent);

    Matrix34 GetNodeWorldTM(const IEntity* pChild, const IEntity* pParent) const;
    bool IsAttachmentValid(const IEntity* pChild, const IEntity* pParent) const;

private:
    struct SBinding
    {
        IEntity* pChild;
        IEntity* pParent;

        bool operator<(const SBinding& rhs) const
        {
            return (pChild != rhs.pChild) ? (pChild < rhs.pChild) : (pParent < rhs.pParent);
        }
    };

    struct SAttachmentData
    {
        uint m_nodeIndex;
    };

    VectorMap<SBinding, SAttachmentData> m_attachments;
};

#endif
#endif // CRYINCLUDE_CRYENTITYSYSTEM_GEOMCACHEATTACHMENTMANAGER_H
