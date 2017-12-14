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


//This interface is used by meshes and vegetation to handle the static but deformable 
//objects such as attached cloth or willow tree branches. It is tied to the merged mesh
//rendering logic.
class IDeformableNode
{
public:

    virtual ~IDeformableNode() {}

    virtual void SetStatObj(IStatObj* pStatObj) = 0;

    virtual void CreateDeformableSubObject(bool create, const Matrix34& worldTM, IGeneralMemoryHeap* pHeap) = 0;
    virtual void Render(const struct SRendParams& rParams, const SRenderingPassInfo& passInfo, const AABB& aabb) = 0;
    virtual void RenderInternalDeform(CRenderObject* pRenderObject
        , int nLod, const AABB& bbox, const SRenderingPassInfo& passInfo
        , const SRendItemSorter& rendItemSorter) = 0;

    virtual void BakeDeform(const Matrix34& worldTM) = 0;

    virtual bool HasDeformableData() const = 0;
};