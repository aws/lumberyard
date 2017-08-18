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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_MISSINGNODE_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_MISSINGNODE_H
#pragma once


#include "HyperGraphNode.h"

#define MISSING_NODE_CLASS QStringLiteral("MissingNode")

class CMissingNode
    : public CHyperNode
{
public:
    CMissingNode(const QString& sMissingClassName);
    virtual ~CMissingNode();

    virtual QString GetClassName() const { return MISSING_NODE_CLASS; };

    virtual void Init() {};
    virtual CHyperNode* Clone();

    virtual void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar = 0);
    virtual CHyperNodePort* FindPort(const char* portname, bool bInput);
    virtual bool IsEditorSpecialNode() { return true; }
    virtual bool IsFlowNode() { return false; }

private:
    QString m_sMissingClassName;
    QString m_sMissingName;
    QString m_sGraphEntity;
    GUID m_entityGuid;
    int m_iOrgFlags;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_MISSINGNODE_H
