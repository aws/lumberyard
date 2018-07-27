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


#include "CryLegacy_precompiled.h"
#if defined(USE_GEOM_CACHES)
#include "GeomCacheAttachmentManager.h"

void CGeomCacheAttachmentManager::Update()
{
    for (VectorMap<SBinding, SAttachmentData>::iterator iter = m_attachments.begin(); iter != m_attachments.end(); ++iter)
    {
        iter->first.pChild->InvalidateTM(ENTITY_XFORM_FROM_PARENT);
    }
}

void CGeomCacheAttachmentManager::RegisterAttachment(IEntity* pChild, IEntity* pParent, const uint32 targetHash)
{
    IGeomCacheRenderNode* pGeomCacheRenderNode = const_cast<IEntity*>(pParent)->GetGeomCacheRenderNode(0);

    if (pGeomCacheRenderNode)
    {
        const uint nodeCount = pGeomCacheRenderNode->GetNodeCount();

        for (uint i = 0; i < nodeCount; ++i)
        {
            const uint32 nodeNameHash = pGeomCacheRenderNode->GetNodeNameHash(i);
            if (nodeNameHash == targetHash)
            {
                SBinding binding;
                binding.pChild = pChild;
                binding.pParent = pParent;

                SAttachmentData attachment;
                attachment.m_nodeIndex = i;

                m_attachments[binding] = attachment;
                break;
            }
        }
    }
}

void CGeomCacheAttachmentManager::UnregisterAttachment(const IEntity* pChild, const IEntity* pParent)
{
    SBinding binding;
    binding.pChild = const_cast<IEntity*>(pChild);
    binding.pParent = const_cast<IEntity*>(pParent);

    m_attachments.erase(binding);
}

Matrix34 CGeomCacheAttachmentManager::GetNodeWorldTM(const IEntity* pChild, const IEntity* pParent) const
{
    SBinding binding;
    binding.pChild = const_cast<IEntity*>(pChild);
    binding.pParent = const_cast<IEntity*>(pParent);

    VectorMap<SBinding, SAttachmentData>::const_iterator findIter = m_attachments.find(binding);

    if (findIter != m_attachments.end())
    {
        const SAttachmentData data = findIter->second;
        IGeomCacheRenderNode* pGeomCacheRenderNode = const_cast<IEntity*>(binding.pParent)->GetGeomCacheRenderNode(0);
        if (pGeomCacheRenderNode)
        {
            Matrix34 nodeTransform = pGeomCacheRenderNode->GetNodeTransform(data.m_nodeIndex);
            nodeTransform.OrthonormalizeFast();
            return pParent->GetWorldTM() * nodeTransform;
        }
    }

    return pParent->GetWorldTM();
}

bool CGeomCacheAttachmentManager::IsAttachmentValid(const IEntity* pChild, const IEntity* pParent) const
{
    SBinding binding;
    binding.pChild = const_cast<IEntity*>(pChild);
    binding.pParent = const_cast<IEntity*>(pParent);

    VectorMap<SBinding, SAttachmentData>::const_iterator findIter = m_attachments.find(binding);

    if (findIter != m_attachments.end())
    {
        const SAttachmentData data = findIter->second;
        IGeomCacheRenderNode* pGeomCacheRenderNode = const_cast<IEntity*>(binding.pParent)->GetGeomCacheRenderNode(0);
        return pGeomCacheRenderNode->IsNodeDataValid(data.m_nodeIndex);
    }

    return false;
}
#endif