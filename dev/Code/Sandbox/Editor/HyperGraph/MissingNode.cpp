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
#include "MissingNode.h"

#include "HyperGraph.h"
#include "Objects/EntityObject.h"

CMissingNode::CMissingNode(const QString& sMissingClassName)
    : m_sMissingClassName(sMissingClassName)
{
    ZeroStruct(m_entityGuid);
    if (!gEnv->IsEditor())
    {
        assert(false);
    }
}

CMissingNode::~CMissingNode()
{
}

CHyperNode* CMissingNode::Clone()
{
    CMissingNode* pNode = new CMissingNode(m_sMissingClassName);
    pNode->CopyFrom(*this);

    pNode->m_sMissingName = m_sMissingName;
    pNode->m_sGraphEntity = m_sGraphEntity;
    pNode->m_entityGuid = m_entityGuid;
    pNode->m_iOrgFlags = m_iOrgFlags;

    return pNode;
}

CHyperNodePort* CMissingNode::FindPort(const char* portname, bool bInput)
{
    CHyperNodePort* res = CHyperNode::FindPort(portname, bInput);

    if (!res)
    {
        res = CreateMissingPort(portname, bInput);
    }

    return res;
}

void CMissingNode::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    if (bLoading)
    {
        node->getAttr("Name", m_sMissingName);
        node->getAttr("GraphEntity", m_sGraphEntity);
        node->getAttr("flags", m_iOrgFlags);

        if (node->getAttr("EntityGUID", m_entityGuid) && ar)
        {
            m_entityGuid = ar->ResolveID(m_entityGuid);
        }

        XmlNodeRef portsNode = node->findChild("Inputs");
        if (portsNode)
        {
            for (int i = 0; i < portsNode->getNumAttributes(); ++i)
            {
                cstr sKey, sVal;
                portsNode->getAttributeByIndex(i, &sKey, &sVal);
                CreateMissingPort(sKey, true, true);
            }
        }
    }
    else
    {
        SetName(m_sMissingName);
    }

    CHyperNode::Serialize(node, bLoading, ar);

    if (!m_sGraphEntity.isEmpty())
    {
        node->setAttr("GraphEntity", m_sGraphEntity.toUtf8().data());
    }

    if (!GuidUtil::IsEmpty(m_entityGuid))
    {
        node->setAttr("EntityGUID", m_entityGuid);
        EntityGUID entid64 = ToEntityGuid(m_entityGuid);
        if (entid64)
        {
            node->setAttr("EntityGUID_64", entid64);
        }
    }
    node->setAttr("flags", m_iOrgFlags);

    SetName("MISSING");
}