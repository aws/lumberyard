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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_LOADERCAF_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_LOADERCAF_H
#pragma once

#include <IChunkFile.h>
#include <CGFContent.h>
#include "../CGA/Controller.h"  // IController_AutoPtr

struct IChunkFile;
class CContentCGF;
struct ChunkDesc;
//////////////////////////////////////////////////////////////////////////

typedef DynArray<PQLog> TrackPQLog;
typedef DynArray<int> TrackTimes;
typedef DynArray<CryTCB3Key> TrackTCB3Keys;
typedef DynArray<CryTCBQKey> TrackTCBQKeys;


class IController;

class CInternalSkinningInfo
    : public _reference_target_t
{
public:
    DynArray<TCBFlags> m_TrackVec3Flags;
    DynArray<TCBFlags> m_TrackQuatFlags;
    DynArray<TrackTCB3Keys*> m_TrackVec3;
    DynArray<TrackTCBQKeys*> m_TrackQuat;
    DynArray<CControllerType> m_arrControllers;
    DynArray<string> m_arrBoneNameTable;    //names of bones
    uint32 m_numChunks;
    QuatT m_StartLocation;
    bool m_bNewFormat;
    bool m_bOldFormat;
    bool m_bTCBFormat;
    DWORD m_dwTimeStamp;

    DynArray<IController_AutoPtr> m_pControllers;
    DynArray<uint32> m_arrControllerId;

    int32 m_nTicksPerFrame;
    f32 m_secsPerTick;
    int32 m_nStart;
    int32 m_nEnd;
    f32 m_Speed;
    f32 m_Distance;
    f32 m_Slope;
    int m_nAssetFlags;
    f32 m_LHeelStart, m_LHeelEnd;
    f32 m_LToe0Start, m_LToe0End;
    f32 m_RHeelStart, m_RHeelEnd;
    f32 m_RToe0Start, m_RToe0End;
    Vec3 m_MoveDirection;
    DynArray<uint8> m_FootPlantBits;

    // Fields added for skeleton-free animation import.
    uint32 m_rootBoneId;

    CInternalSkinningInfo()
        : m_bNewFormat(false)
        , m_bOldFormat(false)
        , m_bTCBFormat(false)
        , m_dwTimeStamp(0)
        , m_Speed(-1.0f)
        , m_Distance(-1.0f)
        , m_nAssetFlags(0)
        , m_LHeelStart(-10000.0f)
        , m_LHeelEnd(-10000.0f)
        , m_nStart(1)
        , m_nEnd(1)
        , m_LToe0Start(-10000.0f)
        , m_LToe0End(-10000.0f)
        , m_RHeelStart(-10000.0f)
        , m_RHeelEnd(-10000.0f)
        , m_RToe0Start(-10000.0f)
        , m_RToe0End(-10000.0f)
        , m_Slope(0)
    {
    }
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_LOADERCAF_H
