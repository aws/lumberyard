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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_COMPRESSIONCONTROLLER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_COMPRESSIONCONTROLLER_H
#pragma once


#include "QuatQuantization.h"

class IController;
struct GlobalAnimationHeaderCAF;


struct BaseCompressedQuat
{
    virtual ~BaseCompressedQuat() = default;
    virtual void FromQuat(Quat& q) = 0;
    virtual Quat ToQuat() const = 0;
};

struct BaseCompressedVec3
{
    virtual ~BaseCompressedVec3() = default;
    virtual void FromVec3(Vec3& v) = 0;
    virtual Vec3 ToVec3() const = 0;
};

template<class _Realization>
struct CompressedQuatImpl
    : public BaseCompressedQuat
{
    ~CompressedQuatImpl() override = default;
    virtual void FromQuat(Quat& q)
    {
        val.ToInternalType(q);
    }

    virtual Quat ToQuat() const
    {
        return val.ToExternalType();
    }

private:
    _Realization val;
};

typedef CompressedQuatImpl<NoCompressQuat> TNoCompressedQuat;
typedef CompressedQuatImpl<ShotInt3Quat> TShortInt3CompressedQuat;
typedef CompressedQuatImpl<SmallTreeDWORDQuat> TSmallTreeDWORDCompressedQuat;
typedef CompressedQuatImpl<SmallTree48BitQuat> TSmallTree48BitCompressedQuat;
typedef CompressedQuatImpl<SmallTree64BitQuat> TSmallTree64BitCompressedQuat;
typedef CompressedQuatImpl<SmallTree64BitExtQuat> TSmallTree64BitExtCompressedQuat;
typedef CompressedQuatImpl<PolarQuat> TPolarQuatQuat;

template<class _Realization>
struct CompressedVec3Impl
    : public BaseCompressedVec3
{
    ~CompressedVec3Impl() override = default;
    virtual void FromVec3(Vec3& q)
    {
        val.ToInternalType(q);
    }

    virtual Vec3 ToVec3() const
    {
        return val.ToExternalType();
    }

private:
    _Realization val;
};

typedef CompressedVec3Impl<NoCompressVec3> TBaseCompressedVec3;


class CCompressonator
{
private:
    enum EOperation
    {
        eOperation_DontCompress,
        eOperation_Compress,
        eOperation_Delete
    };

    struct Operations
    {
        EOperation m_ePosOperation;
        EOperation m_eRotOperation;
    };

public:
    std::vector<Quat> m_Rotations;
    std::vector<Vec3> m_Positions;
    std::vector<int> m_PosTimes;
    std::vector<int> m_RotTimes;

public:
    void CompressAnimation(
        const GlobalAnimationHeaderCAF& header,
        GlobalAnimationHeaderCAF& newheader,
        const CSkeletonInfo* skeleton,
        const SPlatformAnimationSetup& platform,
        const SAnimationDesc& desc,
        const CInternalSkinningInfo* controllerSkinningInfo,
        bool bDebugCompression);

    void CompressRotations(CController* pController, ECompressionFormat rotationFormat, bool bRemoveKeys, float rotationToleranceInDegrees);
    void CompressPositions(CController* pController, ECompressionFormat positionFormat, bool bRemoveKeys, float positionTolerance);

private:
    bool ComputeControllerDelete_Override(
        Operations& operations,
        const GlobalAnimationHeaderCAF& header,
        const CSkeletonInfo* pSkeleton,
        uint32 controllerIndex,
        const SPlatformAnimationSetup& platform,
        const SAnimationDesc& desc,
        const CInternalSkinningInfo* controllerSkinningInfo,
        bool bDebugCompression);

    bool CompressController_Override(
        const Operations& operations,
        const GlobalAnimationHeaderCAF& header,
        GlobalAnimationHeaderCAF& newheader,
        const CSkeletonInfo* pSkeleton,
        uint32 controllerIndex,
        const SPlatformAnimationSetup& platform,
        const SAnimationDesc& desc,
        const CInternalSkinningInfo* controllerSkinningInfo,
        bool bDebugCompression);

    bool ComputeControllerDelete_Additive(
        Operations& operations,
        const GlobalAnimationHeaderCAF& header,
        const CSkeletonInfo* pSkeleton,
        uint32 controllerIndex,
        const SPlatformAnimationSetup& platform,
        const SAnimationDesc& desc,
        const CInternalSkinningInfo* controllerSkinningInfo,
        bool bDebugCompression);

    bool CompressController_Additive(
        const Operations& operations,
        const GlobalAnimationHeaderCAF& header,
        GlobalAnimationHeaderCAF& newheader,
        const CSkeletonInfo* pSkeleton,
        uint32 controllerIndex,
        const SPlatformAnimationSetup& platform,
        const SAnimationDesc& desc,
        const CInternalSkinningInfo* controllerSkinningInfo,
        bool bDebugCompression);

    void CompressController(
        float positionTolerance,
        float rotationToleranceInDegrees,
        const Operations& operations,
        GlobalAnimationHeaderCAF& newheader,
        uint32 controllerIndex,
        uint32 controllerId,
        uint32 flags);
};


// This function is copy of pre-March2012 version of Quat::IsEquivalent().
// (Changes made in Quat::IsEquivalent() broke binary compatibility, so now
// we are forced to use our own copy of the function).
ILINE bool IsQuatEquivalent_dot(const Quat& q0, const Quat& q1, float e)
{
    const Quat p1 = -q1;
    const bool t0 = (fabs_tpl(q0.v.x - q1.v.x) <= e) && (fabs_tpl(q0.v.y - q1.v.y) <= e) && (fabs_tpl(q0.v.z - q1.v.z) <= e) && (fabs_tpl(q0.w - q1.w) <= e);
    const bool t1 = (fabs_tpl(q0.v.x - p1.v.x) <= e) && (fabs_tpl(q0.v.y - p1.v.y) <= e) && (fabs_tpl(q0.v.z - p1.v.z) <= e) && (fabs_tpl(q0.w - p1.w) <= e);
    return t0 || t1;
}

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_COMPRESSIONCONTROLLER_H
