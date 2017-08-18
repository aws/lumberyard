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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_BSPTREE3D_H
#define CRYINCLUDE_CRYENTITYSYSTEM_BSPTREE3D_H
#pragma once


#include "AreaUtil.h"

class CBSPTree3D
    : public IBSPTree3D
{
public:
    CBSPTree3D(const IBSPTree3D::FaceList& faceList);

    virtual bool IsInside(const Vec3& vPos) const;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    virtual size_t WriteToBuffer(void* pBuffer) const;
    virtual void ReadFromBuffer(const void* pBuffer);

private:

    struct BSPTreeNode;
    typedef DynArray<BSPTreeNode> NodeStorage;
    typedef uint NodeIndex;

    static const NodeIndex kInvalidNodeIndex = (NodeIndex) - 1;

    void BuildTree(const IBSPTree3D::FaceList& faceList, NodeStorage& treeNodes) const;
    static AreaUtil::ESplitResult SplitFaceByPlane(const AreaUtil::CPlaneBase& plane, const IBSPTree3D::CFace& inFace, IBSPTree3D::CFace& outPosFace, IBSPTree3D::CFace& outNegFace);

private:

    struct BSPTreeNode
    {
        AreaUtil::EPointPosEnum IsPointIn(const Vec3& vPos, const NodeStorage& pNodeStorage) const;
        void GetMemoryUsage(class ICrySizer* pSizer) const;

        AreaUtil::CPlaneBase m_Plane;
        NodeIndex m_PosChild;
        NodeIndex m_NegChild;
    };

    struct GenerateNodeTask
    {
        GenerateNodeTask()
            : m_TargetNode(kInvalidNodeIndex) {}

        NodeIndex m_TargetNode;
        FaceList m_FaceList;
    };

    NodeStorage m_BSPTree;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_BSPTREE3D_H
