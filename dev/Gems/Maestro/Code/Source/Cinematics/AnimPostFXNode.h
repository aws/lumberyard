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

#ifndef CRYINCLUDE_CRYMOVIE_ANIMPOSTFXNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMPOSTFXNODE_H
#pragma once


#include "AnimNode.h"

class CFXNodeDescription;

//////////////////////////////////////////////////////////////////////////
class CAnimPostFXNode
    : public CAnimNode
{
public:
    //-----------------------------------------------------------------------------
    //!
    static void Initialize();

    //-----------------------------------------------------------------------------
    //!
    static CAnimNode* CreateNode(const int id, EAnimNodeType nodeType);

    //-----------------------------------------------------------------------------
    //!
    CAnimPostFXNode(const int id, CFXNodeDescription* pDesc);

    //-----------------------------------------------------------------------------
    //!
    virtual void SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    //-----------------------------------------------------------------------------
    //!
    virtual EAnimNodeType GetType() const;

    //-----------------------------------------------------------------------------
    //!
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

    //-----------------------------------------------------------------------------
    //!

    //-----------------------------------------------------------------------------
    //!
    virtual void CreateDefaultTracks();

    virtual void OnReset();

    //-----------------------------------------------------------------------------
    //!
    virtual void Animate(SAnimContext& ac);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    typedef std::map< EAnimNodeType, _smart_ptr<CFXNodeDescription> > FxNodeDescriptionMap;
    static FxNodeDescriptionMap s_fxNodeDescriptions;
    static bool s_initialized;

    CFXNodeDescription* m_pDescription;
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMPOSTFXNODE_H
