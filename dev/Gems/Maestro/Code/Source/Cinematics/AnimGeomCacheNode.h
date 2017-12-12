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

// Description : CryMovie animation node for geometry caches


#ifndef CRYINCLUDE_CRYMOVIE_ANIMGEOMCACHENODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMGEOMCACHENODE_H
#pragma once

#if defined(USE_GEOM_CACHES)
#include "EntityNode.h"

class CAnimGeomCacheNode
    : public CAnimEntityNode
{
public:
    CAnimGeomCacheNode(const int id);
    ~CAnimGeomCacheNode();
    static void Initialize();

    virtual void Animate(SAnimContext& animContext);
    virtual void CreateDefaultTracks();
    virtual void OnReset();
    virtual void Activate(bool bActivate);
    virtual void PrecacheDynamic(float startTime) override;

    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

private:
    IGeomCacheRenderNode* GetGeomCacheRenderNode();

    CAnimGeomCacheNode(const CAnimGeomCacheNode&);
    CAnimGeomCacheNode& operator = (const CAnimGeomCacheNode&);

    bool m_bActive;
};

#endif
#endif // CRYINCLUDE_CRYMOVIE_ANIMGEOMCACHENODE_H
