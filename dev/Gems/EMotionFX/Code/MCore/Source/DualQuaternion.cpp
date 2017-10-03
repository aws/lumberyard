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

// include required headers
#include "DualQuaternion.h"


namespace MCore
{
    // calculate the inverse
    DualQuaternion& DualQuaternion::Inverse()
    {
        const float realLength = mReal.Length();
        const float dotProduct = mReal.Dot(mDual);
        const float dualFactor = realLength - 2.0f * dotProduct;

        mReal.Set(-mReal.x * realLength, -mReal.y * realLength, -mReal.z * realLength, mReal.w * realLength);
        mDual.Set(-mDual.x * dualFactor, -mDual.y * dualFactor, -mDual.z * dualFactor, mDual.w * dualFactor);

        return *this;
    }


    // calculate the inversed version
    DualQuaternion DualQuaternion::Inversed() const
    {
        const float realLength = mReal.Length();
        const float dotProduct = mReal.Dot(mDual);
        const float dualFactor = realLength - 2.0f * dotProduct;
        return DualQuaternion(Quaternion(-mReal.x * realLength, -mReal.y * realLength, -mReal.z * realLength, mReal.w * realLength),
            Quaternion(-mDual.x * dualFactor, -mDual.y * dualFactor, -mDual.z * dualFactor, mDual.w * dualFactor));
    }


    // convert the dual quaternion to a matrix
    Matrix DualQuaternion::ToMatrix() const
    {
        Matrix m;
        const float sqLen   = mReal.Dot(mReal);
        const float x       = mReal.x;
        const float y       = mReal.y;
        const float z       = mReal.z;
        const float w       = mReal.w;
        const float t0      = mDual.w;
        const float t1      = mDual.x;
        const float t2      = mDual.y;
        const float t3      = mDual.z;

        MMAT(m, 0, 0) = w * w + x * x - y * y - z * z;
        MMAT(m, 1, 0) = 2.0f * x * y - 2.0f * w * z;
        MMAT(m, 2, 0) = 2.0f * x * z + 2.0f * w * y;
        MMAT(m, 0, 1) = 2.0f * x * y + 2.0f * w * z;
        MMAT(m, 1, 1) = w * w + y * y - x * x - z * z;
        MMAT(m, 2, 1) = 2.0f * y * z - 2.0f * w * x;
        MMAT(m, 0, 2) = 2.0f * x * z - 2.0f * w * y;
        MMAT(m, 1, 2) = 2.0f * y * z + 2.0f * w * x;
        MMAT(m, 2, 2) = w * w + z * z - x * x - y * y;

        MMAT(m, 3, 0) = -2.0f * t0 * x + 2.0f * w * t1 - 2.0f * t2 * z + 2.0f * y * t3;
        MMAT(m, 3, 1) = -2.0f * t0 * y + 2.0f * t1 * z - 2.0f * x * t3 + 2.0f * w * t2;
        MMAT(m, 3, 2) = -2.0f * t0 * z + 2.0f * x * t2 + 2.0f * w * t3 - 2.0f * t1 * y;

        MMAT(m, 0, 3) = 0.0f;
        MMAT(m, 1, 3) = 0.0f;
        MMAT(m, 2, 3) = 0.0f;
        MMAT(m, 3, 3) = sqLen;

        const float invSqLen = 1.0f / sqLen;
        m *= invSqLen;
        return m;
    }


    // construct the dual quaternion from a given non-scaled matrix
    DualQuaternion DualQuaternion::ConvertFromMatrix(const Matrix& m)
    {
        Vector3 pos;
        Quaternion rot;
        m.Decompose(&pos, &rot); // does not allow scale
        return DualQuaternion(rot, pos);
    }


    // normalizes the dual quaternion
    DualQuaternion& DualQuaternion::Normalize()
    {
        const float length = mReal.Length();
        const float invLength = 1.0f / length;

        mReal.Set(mReal.x * invLength, mReal.y * invLength, mReal.z * invLength, mReal.w * invLength);
        mDual.Set(mDual.x * invLength, mDual.y * invLength, mDual.z * invLength, mDual.w * invLength);
        mDual += mReal * (mReal.Dot(mDual) * -1.0f);
        return *this;
    }


    // convert back into rotation and translation
    void DualQuaternion::ToRotationTranslation(Quaternion* outRot, Vector3* outPos) const
    {
        const float invLength = 1.0f / mReal.Length();
        *outRot = mReal * invLength;
        outPos->Set(2.0f * (-mDual.w * mReal.x + mDual.x * mReal.w - mDual.y * mReal.z + mDual.z * mReal.y) * invLength,
            2.0f * (-mDual.w * mReal.y + mDual.x * mReal.z + mDual.y * mReal.w - mDual.z * mReal.x) * invLength,
            2.0f * (-mDual.w * mReal.z - mDual.x * mReal.y + mDual.y * mReal.x + mDual.z * mReal.w) * invLength);
    }


    // special case version for conversion into rotation and translation
    // only works with normalized dual quaternions
    void DualQuaternion::NormalizedToRotationTranslation(Quaternion* outRot, Vector3* outPos) const
    {
        *outRot = mReal;
        outPos->Set(2.0f * (-mDual.w * mReal.x + mDual.x * mReal.w - mDual.y * mReal.z + mDual.z * mReal.y),
            2.0f * (-mDual.w * mReal.y + mDual.x * mReal.z + mDual.y * mReal.w - mDual.z * mReal.x),
            2.0f * (-mDual.w * mReal.z - mDual.x * mReal.y + mDual.y * mReal.x + mDual.z * mReal.w));
    }


    // convert into a dual quaternion from a translation and rotation
    DualQuaternion DualQuaternion::ConvertFromRotationTranslation(const Quaternion& rotation, const Vector3& translation)
    {
        return DualQuaternion(rotation,  0.5f * (Quaternion(translation.x, translation.y, translation.z, 0.0f) * rotation));
    }
}   // namespace MCore
