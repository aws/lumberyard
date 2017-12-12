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

#ifndef CRYINCLUDE_CRYMOVIE_EVENTNODE_H
#define CRYINCLUDE_CRYMOVIE_EVENTNODE_H
#pragma once


#include "AnimNode.h"

class CAnimEventNode
    : public CAnimNode
{
public:
    CAnimEventNode(const int id);

    virtual EAnimNodeType GetType() const { return eAnimNodeType_Event; }

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    //////////////////////////////////////////////////////////////////////////
    virtual void Animate(SAnimContext& ec);
    virtual void CreateDefaultTracks();
    virtual void OnReset();

    //////////////////////////////////////////////////////////////////////////
    // Supported tracks description.
    //////////////////////////////////////////////////////////////////////////
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        CAnimNode::GetMemoryUsage(pSizer);
    }
private:
    void SetScriptValue();

private:
    //! Last animated key in track.
    int m_lastEventKey;
};

#endif // CRYINCLUDE_CRYMOVIE_EVENTNODE_H
