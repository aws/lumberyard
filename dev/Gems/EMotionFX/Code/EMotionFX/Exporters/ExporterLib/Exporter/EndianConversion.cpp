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

#include "Exporter.h"


namespace ExporterLib
{
    void CopyVector2(EMotionFX::FileFormat::FileVector2& to, const AZ::Vector2& from)
    {
        to.mX = from.GetX();
        to.mY = from.GetY();
    }

    void CopyVector(EMotionFX::FileFormat::FileVector3& to, const MCore::Vector3& from)
    {
        to.mX = from.x;
        to.mY = from.y;
        to.mZ = from.z;
    }


    void CopyQuaternion(EMotionFX::FileFormat::FileQuaternion& to, MCore::Quaternion from)
    {
        from.Normalize();

        if (from.w < 0)
        {
            from = -from;
        }

        to.mX = from.x;
        to.mY = from.y;
        to.mZ = from.z;
        to.mW = from.w;
    }


    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, MCore::Quaternion from)
    {
        from.Normalize();

        if (from.w < 0)
        {
            from = -from;
        }

        MCore::Compressed16BitQuaternion compressedQuat(from);

        to.mX = compressedQuat.mX;
        to.mY = compressedQuat.mY;
        to.mZ = compressedQuat.mZ;
        to.mW = compressedQuat.mW;
    }


    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, MCore::Compressed16BitQuaternion from)
    {
        if (from.mW < 0)
        {
            from.mX = -from.mX;
            from.mY = -from.mY;
            from.mZ = -from.mZ;
            from.mW = -from.mW;
        }

        to.mX = from.mX;
        to.mY = from.mY;
        to.mZ = from.mZ;
        to.mW = from.mW;
    }


    void CopyColor(const MCore::RGBAColor& from, EMotionFX::FileFormat::FileColor& to)
    {
        to.mR = from.r;
        to.mG = from.g;
        to.mB = from.b;
        to.mA = from.a;
    }


    void ConvertUnsignedInt(uint32* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertInt(int* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertSignedInt32(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertUnsignedShort(uint16* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt16(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFloat(float* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(value, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileChunk(EMotionFX::FileFormat::FileChunk* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(&value->mChunkID,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mSizeInBytes,  EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mVersion,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileColor(EMotionFX::FileFormat::FileColor* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mR, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mG, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mB, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mA, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileVector2(EMotionFX::FileFormat::FileVector2* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileVector3(EMotionFX::FileFormat::FileVector3* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFile16BitVector3(EMotionFX::FileFormat::File16BitVector3* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt16(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileQuaternion(EMotionFX::FileFormat::FileQuaternion* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mW, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFile16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertSignedInt16(&value->mX, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->mY, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->mZ, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertSignedInt16(&value->mW, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileMotionEvent(EMotionFX::FileFormat::FileMotionEvent* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->mStartTime,         EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->mEndTime,           EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mEventTypeIndex,    EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mMirrorTypeIndex,   EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt16(&value->mParamIndex,        EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertFileMotionEventTable(EMotionFX::FileFormat::FileMotionEventTrack* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertUnsignedInt32(&value->mNumEvents,            EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mNumTypeStrings,       EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mNumParamStrings,      EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertUnsignedInt32(&value->mNumMirrorTypeStrings, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertRGBAColor(MCore::RGBAColor* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->r, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->g, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->b, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->a, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }


    void ConvertVector3(MCore::Vector3* value, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::Endian::ConvertFloat(&value->x, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->y, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
        MCore::Endian::ConvertFloat(&value->z, EXPLIB_PLATFORM_ENDIAN, targetEndianType);
    }
} // namespace ExporterLib
