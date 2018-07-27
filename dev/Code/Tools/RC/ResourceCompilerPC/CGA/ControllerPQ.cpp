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

#include "stdafx.h"


#include "AnimationLoader.h"
#include "AnimationManager.h"
#include "wavelets/Compression.h"

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


    ITrackPositionStorage* GetPositionControllerPtr(ECompressionFormat format)
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
            return new NoCompressPosition;
        }
        }
        return 0;
        //return new
    }

    ITrackRotationStorage* GetRotationControllerPtr(ECompressionFormat format)
    {
        switch (format)
        {
        /*
        typedef CTrackDataStorage<Quat, NoCompressQuat, ITrackRotationStorage> NoCompressRotation;
        typedef _smart_ptr<NoCompressRotation> NoCompressRotationPtr;

        typedef CTrackDataStorage<Quat, SmallTree48BitQuat, ITrackRotationStorage> SmallTree48BitQuatRotation;
        typedef _smart_ptr<SmallTree48BitQuatRotation> SmallTree48BitQuatRotationPtr;

        typedef CTrackDataStorage<Quat, SmallTree64BitQuat, ITrackRotationStorage> SmallTree64BitQuatRotation;
        typedef _smart_ptr<SmallTree64BitQuatRotation> SmallTree64BitQuatRotationPtr;

        typedef CTrackDataStorage<Quat, SmallTree64BitExtQuat, ITrackRotationStorage> SmallTree64BitExtQuatRotation;
        typedef _smart_ptr<SmallTree64BitExtQuatRotation> SmallTree64BitExtQuatRotationPtr;

        */
        case    eNoCompress:
        case eNoCompressQuat:
            return new NoCompressRotation;

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
            return new SmallTree48BitQuatRotation;

        case eSmallTree64BitQuat:
            //return new SmallTree64BitQuatRotationTrackInformation;
            return new SmallTree64BitQuatRotation;

        //case ePolarQuat:
        //  //return new PolarQuatRotationTrackInformation;

        case eSmallTree64BitExtQuat:
            //return new SmallTree64BitExtQuatRotationTrackInformation;
            return new SmallTree64BitExtQuatRotation;
        }
        return 0;
    }

    IKeyTimesInformation* GetKeyTimesControllerPtr(uint32 format)
    {
        switch (format)
        {
        case eF32:
            return new F32KeyTimesInformation;

        case eUINT16:
            return new UINT16KeyTimesInformation;

        case eByte:
            return new ByteKeyTimesInformation;

        //case eF32StartStop:
        //  return new F32SSKeyTimesInformation;

        //case eUINT16StartStop:
        //  return new UINT16SSKeyTimesInformation;

        //case eByteStartStop:
        //  return new ByteSSKeyTimesInformation;

        case eBitset:
            return new CKeyTimesInformationBitSet;
        }
        return 0;
    }
    
    //Gives us the index of the first set low bit (1) found
    const uint8 m_byteLowBit[byteArraySize] _ALIGN(16) = {
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
    
    //Gives us the index of the first set high bit (1) found
    const uint8 m_byteHighBit[byteArraySize] _ALIGN(16) = {
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
};


uint8 CKeyTimesInformationBitSet::m_bTable[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3,
    4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3,
    3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3,
    4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5,
    4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5,
    6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};
