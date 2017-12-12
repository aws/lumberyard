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
MCORE_INLINE TCompressedVector3<StorageType>::TCompressedVector3()
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedVector3<StorageType>::TCompressedVector3(const Vector3& vec, float minValue, float maxValue)
{
    // TODO: make sure due to rounding/floating point errors the result is not negative?
    const StorageType f = (StorageType)((1.0f / (maxValue - minValue)) * CONVERT_VALUE);
    mX = (StorageType)((vec.x - minValue) * f);
    mY = (StorageType)((vec.y - minValue) * f);
    mZ = (StorageType)((vec.z - minValue) * f);
}


// create from a Vector3
template <class StorageType>
MCORE_INLINE void TCompressedVector3<StorageType>::FromVector3(const Vector3& vec, float minValue, float maxValue)
{
    // TODO: make sure due to rounding/floating point errors the result is not negative?
    const StorageType f = (StorageType)((1.0f / (maxValue - minValue)) * CONVERT_VALUE);
    mX = (StorageType)((vec.x - minValue) * f);
    mY = (StorageType)((vec.y - minValue) * f);
    mZ = (StorageType)((vec.z - minValue) * f);
}


// uncompress into a Vector3
template <class StorageType>
MCORE_INLINE void TCompressedVector3<StorageType>::UnCompress(Vector3* output, float minValue, float maxValue) const
{
    // unpack and normalize
    const float f = (1.0f / (float)CONVERT_VALUE) * (maxValue - minValue);
    output->x = ((float)mX * f) + minValue;
    output->y = ((float)mY * f) + minValue;
    output->z = ((float)mZ * f) + minValue;
}


// convert to a Vector3
template <class StorageType>
MCORE_INLINE Vector3 TCompressedVector3<StorageType>::ToVector3(float minValue, float maxValue) const
{
    const float f = (1.0f / (float)CONVERT_VALUE) * (maxValue - minValue);
    return Vector3((mX * f) + minValue, (mY * f) + minValue, (mZ * f) + minValue);
}
