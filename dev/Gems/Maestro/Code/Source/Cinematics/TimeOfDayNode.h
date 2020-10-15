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

#pragma once


#include "AnimNode.h"

class CTODNodeDescription;

//////////////////////////////////////////////////////////////////////////
class CAnimTODNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimTODNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimTODNode, "{589BD27C-0C44-4802-B040-6D063E60D3FB}", CAnimNode);

    //-----------------------------------------------------------------------------
    //!
    static void Initialize();
    static CTODNodeDescription* GetTODNodeDescription(AnimNodeType nodeType);
    static CAnimNode* CreateNode(const int id, AnimNodeType nodeType);

    //-----------------------------------------------------------------------------
    //!
    CAnimTODNode();
    CAnimTODNode(const int id, AnimNodeType nodeType, CTODNodeDescription* desc);

    //-----------------------------------------------------------------------------
    //!
    virtual void SerializeAnims(XmlNodeRef& xmlNode, bool loading, bool loadEmptyTracks);

    //-----------------------------------------------------------------------------
    //!
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int index) const;

    //-----------------------------------------------------------------------------
    //!
    virtual void CreateDefaultTracks();
    virtual void Activate(bool activate);
    virtual void OnReset();

    //-----------------------------------------------------------------------------
    //!
    virtual void Animate(SAnimContext& ac);

    void InitPostLoad(IAnimSequence* sequence) override;

    static void Reflect(AZ::SerializeContext* serializeContext);
protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    typedef std::map< AnimNodeType, _smart_ptr<CTODNodeDescription> > TODNodeDescriptionMap;
    static StaticInstance<TODNodeDescriptionMap> s_TODNodeDescriptions;
    static bool s_initialized;
    CTODNodeDescription* m_description;

    XmlNodeRef m_xmlData;
};
