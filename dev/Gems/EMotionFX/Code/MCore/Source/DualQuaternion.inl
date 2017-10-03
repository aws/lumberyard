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

// extended constructor
MCORE_INLINE DualQuaternion::DualQuaternion(const Quaternion& rotation, const Vector3& translation)
    : mReal(rotation)
{
    mDual = 0.5f * (Quaternion(translation.x, translation.y, translation.z, 0.0f) * rotation);
}



// transform a 3D point with the dual quaternion
MCORE_INLINE Vector3 DualQuaternion::TransformPoint(const Vector3& point) const
{
    const Vector3 realVector(mReal.x, mReal.y, mReal.z);
    const Vector3 dualVector(mDual.x, mDual.y, mDual.z);
    const Vector3 position      = point + 2.0f * (realVector.Cross(realVector.Cross(point) + (mReal.w * point)));
    const Vector3 displacement  = 2.0f * (mReal.w * dualVector - mDual.w * realVector + realVector.Cross(dualVector));
    return position + displacement;
}


// transform a vector with this dual quaternion
MCORE_INLINE Vector3 DualQuaternion::TransformVector(const Vector3& v) const
{
    const Vector3 realVector(mReal.x, mReal.y, mReal.z);
    const Vector3 dualVector(mDual.x, mDual.y, mDual.z);
    return v + 2.0f * (realVector.Cross(realVector.Cross(v) + mReal.w * v));
}

