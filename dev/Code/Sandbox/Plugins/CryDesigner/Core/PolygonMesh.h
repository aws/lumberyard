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
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   PolygonMesh.h
//  Created:     April/16/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Polygon.h"

namespace CD
{
    class PolygonMesh
        : public CRefCountBase
    {
    public:

        PolygonMesh();
        ~PolygonMesh();

        void SetPolygon(PolygonPtr pPolygon, bool bForce, const Matrix34& worldTM = Matrix34::CreateIdentity(), int dwRndFlags = 0, float viewDistMultiplier = 1.f, int nMinSpec = 0, uint8 materialLayerMask = 0);
        void SetPolygons(const std::vector<PolygonPtr>& polygonList, bool bForce, const Matrix34& worldTM = Matrix34::CreateIdentity(), int dwRndFlags = 0, float viewDistMultiplier = 1.f, int nMinSpec = 0, uint8 materialLayerMask = 0);
        void SetWorldTM(const Matrix34& worldTM);
        void SetMaterialName(const string& name);
        void ReleaseResources();
        IRenderNode* GetRenderNode() const { return m_pRenderNode; }

    private:

        void ApplyMaterial();

        //void UpdateStatObjAndRenderNode( const SMeshInfo& mesh, const Matrix34& worldTM, int dwRndFlags, float viewDistanceMultiplier, int nMinSpec, uint8 materialLayerMask );
        //void UpdateStatObjAndRenderNode( const FlexibleMesh& mesh, const Matrix34& worldTM, int dwRndFlags, int nViewDistRatio, int nMinSpec, uint8 materialLayerMask );
        void UpdateStatObjAndRenderNode(const FlexibleMesh& mesh, const Matrix34& worldTM, int dwRndFlags, float viewDistanceMultiplier, int nMinSpec, uint8 materialLayerMask);
        void ReleaseRenderNode();
        void CreateRenderNode();

        std::vector<PolygonPtr> m_pPolygons;
        IStatObj* m_pStatObj;
        IRenderNode* m_pRenderNode;
        string m_MaterialName;
    };
};