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

// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion()
    : mX(0)
    , mY(0)
    , mZ(0)
    , mW(CONVERT_VALUE)
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion(float xVal, float yVal, float zVal, float wVal)
    : mX((StorageType)xVal)
    , mY((StorageType)yVal)
    , mZ((StorageType)zVal)
    , mW((StorageType)wVal)
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion(const Quaternion& quat)
    : mX((StorageType)(quat.x * CONVERT_VALUE))
    , mY((StorageType)(quat.y * CONVERT_VALUE))
    , mZ((StorageType)(quat.z * CONVERT_VALUE))
    , mW((StorageType)(quat.w * CONVERT_VALUE))
{
}


// create from a quaternion
template <class StorageType>
MCORE_INLINE void TCompressedQuaternion<StorageType>::FromQuaternion(const Quaternion& quat)
{
    // pack it
    mX = (StorageType)(quat.x * CONVERT_VALUE);
    mY = (StorageType)(quat.y * CONVERT_VALUE);
    mZ = (StorageType)(quat.z * CONVERT_VALUE);
    mW = (StorageType)(quat.w * CONVERT_VALUE);
}

// uncompress into a quaternion
template <class StorageType>
MCORE_INLINE void TCompressedQuaternion<StorageType>::UnCompress(Quaternion* output) const
{
    const float f = 1.0f / (float)CONVERT_VALUE;
    output->Set(mX * f, mY * f, mZ * f, mW * f);
}


// uncompress into a quaternion
template <>
MCORE_INLINE void TCompressedQuaternion<int16>::UnCompress(Quaternion* output) const
{
    output->Set(mX * 0.000030518509448f, mY * 0.000030518509448f, mZ * 0.000030518509448f, mW * 0.000030518509448f);
}


// convert to a quaternion
template <class StorageType>
MCORE_INLINE Quaternion TCompressedQuaternion<StorageType>::ToQuaternion() const
{
    const float f = 1.0f / (float)CONVERT_VALUE;
    return Quaternion(mX * f, mY * f, mZ * f, mW * f);
}


// convert to a quaternion
template <>
MCORE_INLINE Quaternion TCompressedQuaternion<int16>::ToQuaternion() const
{
    return Quaternion(mX * 0.000030518509448f, mY * 0.000030518509448f, mZ * 0.000030518509448f, mW * 0.000030518509448f);
}
