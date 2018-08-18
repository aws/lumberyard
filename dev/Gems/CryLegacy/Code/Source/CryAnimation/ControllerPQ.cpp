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

#include "CryLegacy_precompiled.h"

#include "ControllerPQ.h"
#include "CharacterManager.h"

#include "ControllerOpt.h"

#include <float.h>

namespace ControllerHelper
{
    uint32 GetPositionsFormatSizeOf(uint32 format)
    {
        switch (format)
        {
        case eNoCompress:
        case eNoCompressVec3:
        {
            return sizeof (NoCompressVec3);
        }
        }
        return 0;
    }


    uint32 GetRotationFormatSizeOf(uint32 format)
    {
        switch (format)
        {
        case    eNoCompress:
        case eNoCompressQuat:
            return sizeof(NoCompressQuat);

        //case eSmallTreeDWORDQuat:
        //  return sizeof(Sma;

        case eSmallTree48BitQuat:
            return sizeof(SmallTree48BitQuat);

        case eSmallTree64BitQuat:
            return sizeof(SmallTree64BitQuat);

        case eSmallTree64BitExtQuat:
            return sizeof(SmallTree64BitExtQuat);
        }
        return 0;
    }

    uint32 GetKeyTimesFormatSizeOf(uint32 format)
    {
        switch (format)
        {
        case eF32:
            return sizeof(float);

        case eUINT16:
            return sizeof(uint16);

        case eByte:
            return sizeof(uint8);

        case eF32StartStop:
            return sizeof(float);

        case eUINT16StartStop:
            return sizeof(uint16);

        case eByteStartStop:
            return sizeof(uint8);

        case eBitset:
            return sizeof(uint16);
        }

        return 0;
    }

    ITrackPositionStorage* GetPositionControllerPtr(uint32 format, void* pData, size_t numElements)
    {
        //      Quat t;
        //      int GG = test.GetO(0,0,t);

        switch (format)
        {
        case eNoCompress:
        case eNoCompressVec3:
        {
            //                  return new NoCompressVec3PositionTrackInformation;
            //                  IPositionInformation * pTrack = new PositionTrackInformation;
            //              ITrackPositionStorage * pStorage = new NoCompressPosition;
            //                  pTrack->SetPositionStorage(TrackPositionStoragePtr(pStorage));
            //                  return pTrack;
            return new NoCompressPosition(reinterpret_cast<NoCompressVec3*>(pData), numElements);
        }
        }
        return 0;
        //return new
    }

    ITrackPositionStorage* GetPositionControllerPtrArray(uint32 format, uint32 size)
    {
        switch (format)
        {
        case eNoCompress:
        case eNoCompressVec3:
        {
            //                  return new NoCompressVec3PositionTrackInformation;
            //                  IPositionInformation * pTrack = new PositionTrackInformation;
            //              ITrackPositionStorage * pStorage = new NoCompressPosition;
            //                  pTrack->SetPositionStorage(TrackPositionStoragePtr(pStorage));
            //                  return pTrack;
            return new NoCompressPositionInplace[size];
        }
        }
        return 0;
        //return new
    }

    ITrackPositionStorage* GetPositionPtrFromArray(void* ptr, uint32 format, uint32 num)
    {
        switch (format)
        {
        case eNoCompress:
        case eNoCompressVec3:
            return &((NoCompressPositionInplace*)(ptr))[num];
        }
        return 0;
    }

    ITrackRotationStorage* GetRotationControllerPtr(uint32 format, void* pData, size_t numElements)
    {
        switch (format)
        {
        case    eNoCompress:
        case eNoCompressQuat:
            return new NoCompressRotation(reinterpret_cast<NoCompressQuat*>(pData), numElements);
        //case eSmallTreeDWORDQuat:
        //  //return new SmallTreeDWORDQuatRotationTrackInformation;

        case eSmallTree48BitQuat:
            //return new SmallTree48BitQuatRotationTrackInformation;
            return new SmallTree48BitQuatRotation(reinterpret_cast<SmallTree48BitQuat*>(pData), numElements);

        case eSmallTree64BitQuat:
            //return new SmallTree64BitQuatRotationTrackInformation;
            return new SmallTree64BitQuatRotation(reinterpret_cast<SmallTree64BitQuat*>(pData), numElements);

        //case ePolarQuat:
        //  //return new PolarQuatRotationTrackInformation;

        case eSmallTree64BitExtQuat:
            //return new SmallTree64BitExtQuatRotationTrackInformation;
            return new SmallTree64BitExtQuatRotation(reinterpret_cast<SmallTree64BitExtQuat*>(pData), numElements);
        }
        return 0;
    }

    ITrackRotationStorage* GetRotationControllerPtrArray(uint32 format, uint32 size)
    {
        switch (format)
        {
        case    eNoCompress:
        case eNoCompressQuat:
            return new NoCompressRotationInplace[size];

        //case eShotInt3Quat:

        //  //{
        //  //  //                  return new ShotInt3QuatRotationTrackInformation;
        //  //  IPositionInformation * pTrack = new PositionTrackInformation;
        //  //  ITrackPositionStorage * pStorage = new NoCompressPosition;
        //  //  pTrack->SetPositionStorage(TrackPositionStoragePtr(pStorage));

        //  //  return pTrack;
        //  //}



        //case eSmallTreeDWORDQuat:
        //  //return new SmallTreeDWORDQuatRotationTrackInformation;

        case eSmallTree48BitQuat:
            //return new SmallTree48BitQuatRotationTrackInformation;
            return new SmallTree48BitQuatRotationInplace[size];

        case eSmallTree64BitQuat:
            //return new SmallTree64BitQuatRotationTrackInformation;
            return new SmallTree64BitQuatRotationInplace[size];

        //case ePolarQuat:
        //  //return new PolarQuatRotationTrackInformation;

        case eSmallTree64BitExtQuat:
            //return new SmallTree64BitExtQuatRotationTrackInformation;
            return new SmallTree64BitExtQuatRotationInplace[size];
        }
        return 0;
    }

    ITrackRotationStorage* GetRotationPtrFromArray(void* ptr, uint32 format, uint32 num)
    {
        switch (format)
        {
        case    eNoCompress:
        case eNoCompressQuat:
            return &((NoCompressRotationInplace*)(ptr))[num];


        case eSmallTree48BitQuat:
            return &((SmallTree48BitQuatRotationInplace*)(ptr))[num];


        case eSmallTree64BitQuat:
            return &((SmallTree64BitQuatRotationInplace*)(ptr))[num];

        case eSmallTree64BitExtQuat:

            return &((SmallTree64BitExtQuatRotationInplace*)(ptr))[num];
        }
        return 0;
    }


    IKeyTimesInformation* GetKeyTimesControllerPtr(uint32 format, void* pData, size_t numElements)
    {
        switch (format)
        {
        case eF32:
            return new F32KeyTimesInformation(reinterpret_cast<f32*>(pData), numElements);

        case eUINT16:
            return new UINT16KeyTimesInformation(reinterpret_cast<uint16*>(pData), numElements);

        case eByte:
            return new ByteKeyTimesInformation(reinterpret_cast<uint8*>(pData), numElements);

        case eF32StartStop:
            return new F32SSKeyTimesInformation(reinterpret_cast<f32*>(pData), numElements);

        case eUINT16StartStop:
            return new UINT16SSKeyTimesInformation(reinterpret_cast<uint16*>(pData), numElements);

        case eByteStartStop:
            return new ByteSSKeyTimesInformation(reinterpret_cast<uint8*>(pData), numElements);

        case eBitset:
            return new CKeyTimesInformationBitSet(reinterpret_cast<uint16*>(pData), numElements);
        }
        return 0;
    }

    IKeyTimesInformation* GetKeyTimesControllerPtrArray(uint32 format, uint32 size)
    {
        switch (format)
        {
        case eF32:
            return new F32KeyTimesInformationInplace[size];

        case eUINT16:
            return new UINT16KeyTimesInformationInplace[size];

        case eByte:
            return new ByteKeyTimesInformationInplace[size];

        case eF32StartStop:
            return new F32SSKeyTimesInformation[size];

        case eUINT16StartStop:
            return new UINT16SSKeyTimesInformation[size];

        case eByteStartStop:
            return new ByteSSKeyTimesInformation[size];

        case eBitset:
            return new CKeyTimesInformationBitSet[size];
        }
        return 0;
    }

    IKeyTimesInformation* GetKeyTimesPtrFromArray(void* ptr, uint32 format, uint32 num)
    {
        switch (format)
        {
        case eF32:
        {
            //  F32KeyTimesInformationInplace* p = (F32KeyTimesInformationInplace *)(ptr);
            //  return &p[num];
            return &((F32KeyTimesInformationInplace*)(ptr))[num];
        }


        case eUINT16:
            return &((UINT16KeyTimesInformationInplace*)(ptr))[num];

        case eByte:
            return &((ByteKeyTimesInformationInplace*)(ptr))[num];

        case eF32StartStop:
            return &((F32SSKeyTimesInformation*)(ptr))[num];

        case eUINT16StartStop:
            return &((UINT16SSKeyTimesInformation*)(ptr))[num];

        case eByteStartStop:
            return &((ByteSSKeyTimesInformation*)(ptr))[num];

        case eBitset:
            return &((CKeyTimesInformationBitSet*)(ptr))[num];
        }
        return 0;
    }

    void DeletePositionControllerPtrArray(ITrackPositionStorage* ptr)
    {
        delete[] ptr;
    }

    void DeleteRotationControllerPtrArray(ITrackRotationStorage* ptr)
    {
        //free(ptr);
        delete[] ptr;
    }

    void DeleteTimesControllerPtrArray(IKeyTimesInformation* ptr)
    {
        //free(ptr);
        delete[] ptr;
    }
}


#if 0 // Currently unused, and needs support for modifiable controllers
void CCompressedController::UnCompress()
{
    CWaveletData Rotations[3];
    CWaveletData Positions[3];


    //  for (uint32 i = 0; i < 3; ++i)
    if (m_Rotations0.size() > 0)
    {
        Wavelet::UnCompress(m_Rotations0, Rotations[0]);
        Wavelet::UnCompress(m_Rotations1, Rotations[1]);
        Wavelet::UnCompress(m_Rotations2, Rotations[2]);

        uint32 size = m_pRotationController->GetKeyTimesInformation()->GetNumKeys();//Rotations[0].m_Data.size();

        //      m_pRotationController->Reserve(size);

        for (uint32 i = 0; i < size; ++i)
        {
            Ang3 ang(Rotations[0].m_Data[i], Rotations[1].m_Data[i], Rotations[2].m_Data[i]);
            Quat quat;
            quat.SetRotationXYZ(ang);
            quat.Normalize();
            m_pRotationController->GetRotationStorage()->AddValue(quat);
        }
    }


    if (m_Positions0.size() > 0)
    {
        Wavelet::UnCompress(m_Positions0, Positions[0]);
        Wavelet::UnCompress(m_Positions2, Positions[2]);
        Wavelet::UnCompress(m_Positions1, Positions[1]);



        uint32 size = m_pPositionController->GetKeyTimesInformation()->GetNumKeys(); //Positions[0].m_Data.size();
        //      m_pPositionController->Reserve(size);

        for (uint32 i = 0; i < size; ++i)
        {
            Vec3 pos(Positions[0].m_Data[i], Positions[1].m_Data[i], Positions[2].m_Data[i]);
            m_pPositionController->GetPositionStorage()->AddValue(pos);
        }
    }

    m_Rotations0.clear();
    m_Rotations1.clear();
    m_Rotations2.clear();
    m_Positions0.clear();
    m_Positions1.clear();
    m_Positions2.clear();
}
#endif



namespace ControllerHelper
{
    const uint8 /*CKeyTimesInformationBitSet::*/ m_byteTable[256] _ALIGN(16) = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3,
        4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3,
        3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3,
        4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5,
        4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5,
        6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    };

    const uint8 m_byteLowBit[256] _ALIGN(16) = {
        8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };

    const uint8 m_byteHighBit[256] _ALIGN(16) = {
        16, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
    };
}

void IPositionController::GetValueFromKey(uint32 key, Vec3& val)
{
    uint32 numKeys = GetNumKeys();

    if (key >= numKeys)
    {
        key = numKeys - 1;
    }

    m_pData->GetValue(key, val);
}

void IRotationController::GetValueFromKey(uint32 key, Quat& val)
{
    uint32 numKeys = GetNumKeys();

    if (key >= numKeys)
    {
        key = numKeys - 1;
    }

    m_pData->GetValue(key, val);
}

JointState CControllerOptNonVirtual::GetOP(f32 normalized_time, Quat& quat, Vec3& pos) const
{
    return
        this->CControllerOptNonVirtual::GetO(normalized_time, quat) |
        this->CControllerOptNonVirtual::GetP(normalized_time, pos);
}

JointState CControllerOptNonVirtual::GetO(f32 normalized_time, Quat& quat) const
{
    if (eNoFormat == m_rotation.getTimeFormat())
    {
        return 0;
    }
    quat = this->CControllerOptNonVirtual::GetRotValue(normalized_time);
    return eJS_Orientation;
}

JointState CController::GetOP(f32 normalized_time, Quat& quat, Vec3& pos) const
{
    return
        GetO(normalized_time, quat) |
        GetP(normalized_time, pos);
}

JointState CController::GetO(f32 normalized_time, Quat& quat) const
{
    if (m_pRotationController)
    {
        m_pRotationController->GetValue(normalized_time, quat);
        return eJS_Orientation;
    }

    return 0;
}

JointState CController::GetP(f32 normalized_time, Vec3& pos) const
{
    if (m_pPositionController)
    {
        m_pPositionController->GetValue(normalized_time, pos);
        return eJS_Position;
    }

    return 0;
}

#if defined(_CPU_SSE) && !defined(_DEBUG)
__m128 SmallTree48BitQuat::div;
__m128 SmallTree48BitQuat::ran;

int Init48b =  SmallTree48BitQuat::Init();
#endif
