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
#include "Transform.h"
#include "Actor.h"
#include "PlayBackInfo.h"
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    /*
    // copy constructor
    Transform::Transform(const Transform& other)
    {
        mRotation = other.mRotation;
        mPosition = other.mPosition;

        EMFX_SCALECODE
        (
            mScale          = other.mScale;
            mScaleRotation  = other.mScaleRotation;
        )
    }
    */

    // extended constructor
    Transform::Transform(const AZ::Vector3& pos, const MCore::Quaternion& rotation)
    {
        Set(pos, rotation);
    }


    // extended constructor
    Transform::Transform(const AZ::Vector3& pos, const MCore::Quaternion& rotation, const AZ::Vector3& scale)
    {
        Set(pos, rotation, scale);
    }


    // init from a matrix
    Transform::Transform(const MCore::Matrix& mat)
    {
        InitFromMatrix(mat);
    }


    // set
    void Transform::Set(const AZ::Vector3& position, const MCore::Quaternion& rotation)
    {
        mRotation       = rotation;
        mPosition       = position;

        EMFX_SCALECODE
        (
            mScale.Set(1.0f, 1.0f, 1.0f);
        )
    }


    // set
    void Transform::Set(const AZ::Vector3& position, const MCore::Quaternion& rotation, const AZ::Vector3& scale)
    {
    #ifdef EMFX_SCALE_DISABLED
        MCORE_UNUSED(scale);
    #endif

        mRotation       = rotation;
        mPosition       = position;

        EMFX_SCALECODE
        (
            mScale          = scale;
        )
    }


    // check if this transform is equal to another
    bool Transform::operator == (const Transform& right) const
    {
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mPosition, right.mPosition, MCore::Math::epsilon) == false)
        {
            return false;
        }

        if (MCore::Compare<MCore::Quaternion>::CheckIfIsClose(mRotation, right.mRotation, MCore::Math::epsilon) == false)
        {
            return false;
        }

        EMFX_SCALECODE
        (
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mScale, right.mScale, MCore::Math::epsilon) == false)
            {
                return false;
            }
        )

        return true;
    }


    // check if this transform is not equal to another
    bool Transform::operator != (const Transform& right) const
    {
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mPosition, right.mPosition, MCore::Math::epsilon))
        {
            return true;
        }

        if (MCore::Compare<MCore::Quaternion>::CheckIfIsClose(mRotation, right.mRotation, MCore::Math::epsilon))
        {
            return true;
        }

        EMFX_SCALECODE
        (
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mScale, right.mScale, MCore::Math::epsilon))
            {
                return true;
            }
        )

        return false;
    }


    // add a given transform
    Transform Transform::operator +  (const Transform& right) const
    {
        Transform result(*this);
        return result.Add(right);
    }


    // subtract a given transform
    Transform Transform::operator - (const Transform& right) const
    {
        Transform result(*this);
        return result.Subtract(right);
    }


    // multiply a given transform
    Transform Transform::operator * (const Transform& right) const
    {
        Transform result(*this);
        return result.Multiply(right);
    }


    // add
    Transform& Transform::operator += (const Transform& right)
    {
        return Add(right);
    }


    // subtract
    Transform& Transform::operator -= (const Transform& right)
    {
        return Subtract(right);
    }


    // multiply
    Transform& Transform::operator *= (const Transform& right)
    {
        return Multiply(right);
    }


    // init from a matrix
    void Transform::InitFromMatrix(const MCore::Matrix& mat)
    {
    #ifndef EMFX_SCALE_DISABLED
        AZ::Vector3 p, s;
        mat.DecomposeQRGramSchmidt(p, mRotation, s);
        mPosition = p;
        mScale = s;
    #else
        mat.DecomposeQRGramSchmidt(mPosition, mRotation);
    #endif
    }


    // convert to a matrix
    MCore::Matrix Transform::ToMatrix() const
    {
        MCore::Matrix result;

    #ifndef EMFX_SCALE_DISABLED
        result.InitFromPosRotScale(mPosition, mRotation, mScale);
    #else
        result.InitFromPosRot(mPosition, mRotation);
    #endif

        return result;
    }


    // convert to a matrix
    void Transform::ToMatrix(MCore::Matrix& outMatrix) const
    {
    #ifndef EMFX_SCALE_DISABLED
        outMatrix.InitFromPosRotScale(mPosition, mRotation, mScale);
    #else
        outMatrix.InitFromPosRot(mPosition, mRotation);
    #endif
    }


    // identity the transform
    void Transform::Identity()
    {
        mPosition = AZ::Vector3::CreateZero();
        mRotation.Identity();

        EMFX_SCALECODE
        (
            mScale.Set(1.0f, 1.0f, 1.0f);
        );
    }

    // Zero out the position, scale, and rotation.
    void Transform::Zero()
    {
        mPosition = AZ::Vector3::CreateZero();
        mRotation.Set(0, 0, 0, 0);

        EMFX_SCALECODE
        (
            mScale = AZ::Vector3::CreateZero();
        );
    }

    // Zero out the position and scale, but set quaternion to identity.
    void Transform::ZeroWithIdentityQuaternion()
    {
        mPosition = AZ::Vector3::CreateZero();
        mRotation.Identity();

        EMFX_SCALECODE
        (
            mScale = AZ::Vector3::CreateZero();
        );
    }

    // pre multiply with another
    Transform& Transform::PreMultiply(const Transform& other)
    {
    #ifdef EMFX_SCALE_DISABLED
        mPosition   += mRotation * other.mPosition;
    #else
        mPosition   += mRotation * (other.mPosition * mScale);
    #endif

        mRotation       = mRotation * other.mRotation;

        EMFX_SCALECODE
        (
            mScale = mScale * other.mScale;
        )

        return *this;
    }


    // return a multiplied copy
    Transform Transform::PreMultiplied(const Transform& other) const
    {
        Transform result(*this);
        result.PreMultiply(other);
        return result;
    }


    // multiply
    Transform& Transform::Multiply(const Transform& other)
    {
    #ifdef EMFX_SCALE_DISABLED
        mPosition   = other.mRotation * mPosition + other.mPosition;
    #else
        mPosition   = other.mRotation * (mPosition * other.mScale) + other.mPosition;
    #endif

        mRotation       = other.mRotation * mRotation;

        EMFX_SCALECODE
        (
            mScale = other.mScale * mScale;
        )

        return *this;
    }


    // return a multiplied copy
    Transform Transform::Multiplied(const Transform& other) const
    {
        Transform result(*this);
        result.Multiply(other);
        return result;
    }


    // normalize the quaternions
    Transform& Transform::Normalize()
    {
        mRotation.Normalize();
        return *this;
    }


    // return a normalized copy
    Transform Transform::Normalized() const
    {
        Transform result(*this);
        result.Normalize();
        return result;
    }


    // inverse the transform
    Transform& Transform::Inverse()
    {
        EMFX_SCALECODE
        (
            mScale.SetX(1.0f / mScale.GetX());
            mScale.SetY(1.0f / mScale.GetY());
            mScale.SetZ(1.0f / mScale.GetZ());
        )

        mRotation.Conjugate();

    #ifdef EMFX_SCALE_DISABLED
        mPosition = mRotation * -mPosition;
    #else
        mPosition = mRotation * -mPosition * mScale;
    #endif

        return *this;
    }


    // return an inversed copy
    Transform Transform::Inversed() const
    {
        Transform result(*this);
        result.Inverse();
        return result;
    }


    // mirror the transform over the given normal
    Transform& Transform::Mirror(const AZ::Vector3& planeNormal)
    {
        // mirror the position over the plane with the specified normal
        mPosition = MCore::Mirror(mPosition, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(mRotation.x, mRotation.y, mRotation.z), planeNormal);

        // update the rotation quaternion with inverted angle
        mRotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -mRotation.w);

        return *this;
    }


    // mirror the transform over the given normal, with given mirror flags
    Transform& Transform::MirrorWithFlags(const AZ::Vector3& planeNormal, uint8 mirrorFlags)
    {
        // apply the mirror flags
        ApplyMirrorFlags(this, mirrorFlags);

        // mirror the position over the plane with the specified normal
        mPosition = MCore::Mirror(mPosition, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(mRotation.x, mRotation.y, mRotation.z), planeNormal);

        // update the rotation quaternion with inverted angle
        mRotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -mRotation.w);

        return *this;
    }


    // return an mirrored copy
    Transform Transform::Mirrored(const AZ::Vector3& planeNormal) const
    {
        Transform result(*this);
        result.Mirror(planeNormal);
        return result;
    }



    // multiply but output into another
    void Transform::PreMultiply(const Transform& other, Transform* outResult) const
    {
    #ifdef EMFX_SCALE_DISABLED
        outResult->mPosition    = mPosition + mRotation * other.mPosition;
    #else
        outResult->mPosition    = mPosition + mRotation * other.mPosition * mScale;
    #endif

        outResult->mRotation        = mRotation * other.mRotation;

        EMFX_SCALECODE
        (
            outResult->mScale       = mScale * other.mScale;
        )
    }


    // multiply but output into another
    void Transform::Multiply(const Transform& other, Transform* outResult) const
    {
    #ifdef EMFX_SCALE_DISABLED
        outResult->mPosition    = other.mPosition + other.mRotation * mPosition;
    #else
        outResult->mPosition    = other.mPosition + other.mRotation * mPosition * other.mScale;
    #endif

        outResult->mRotation        = other.mRotation * mRotation;

        EMFX_SCALECODE
        (
            outResult->mScale       = other.mScale * mScale;
        )
    }



    // inverse but store the result into another
    void Transform::Inverse(Transform* outResult) const
    {
        // inverse the rotation
        outResult->mRotation = mRotation;
        outResult->mRotation.Conjugate();

        // calculate inverse position
        outResult->mPosition = outResult->mRotation * -mPosition;

        EMFX_SCALECODE
        (
            outResult->mScale.SetX(1.0f / mScale.GetX());
            outResult->mScale.SetY(1.0f / mScale.GetY());
            outResult->mScale.SetZ(1.0f / mScale.GetZ());
        )
    }


    // calculate the transform relative to another
    void Transform::CalcRelativeTo(const Transform& relativeTo, Transform* outTransform) const
    {
    #ifndef EMFX_SCALE_DISABLED
        const AZ::Vector3 invScale = AZ::Vector3(1.0f) / relativeTo.mScale;
        MCore::Quaternion invRot = relativeTo.mRotation;
        invRot.Conjugate();

        outTransform->mPosition = (invRot * (mPosition - relativeTo.mPosition)) * invScale;
        outTransform->mRotation = invRot * mRotation;
        outTransform->mScale    = mScale * invScale;
    #else
        MCore::Quaternion invRot = relativeTo.mRotation;
        invRot.Conjugate();
        outTransform->mPosition = invRot * (mPosition - relativeTo.mPosition);
        outTransform->mRotation = invRot * mRotation;
    #endif
    }


    // return this transformation relative to another
    Transform Transform::CalcRelativeTo(const Transform& relativeTo) const
    {
        Transform result;
        CalcRelativeTo(relativeTo, &result);
        return result;
    }


    // mirror but output into another transform
    void Transform::Mirror(const AZ::Vector3& planeNormal, Transform* outResult) const
    {
        // mirror the position over the normal
        outResult->mPosition = MCore::Mirror(mPosition, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(mRotation.x, mRotation.y, mRotation.z), planeNormal);
        outResult->mRotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -mRotation.w); // store the mirrored quat

        EMFX_SCALECODE
        (
            outResult->mScale = mScale;
        )
    }


    // check if there is scale present
    bool Transform::CheckIfHasScale() const
    {
    #ifdef EMFX_SCALE_DISABLED
        return false;
    #else
        return (
            !MCore::Compare<float>::CheckIfIsClose(mScale.GetX(), 1.0f, MCore::Math::epsilon) ||
            !MCore::Compare<float>::CheckIfIsClose(mScale.GetY(), 1.0f, MCore::Math::epsilon) ||
            !MCore::Compare<float>::CheckIfIsClose(mScale.GetZ(), 1.0f, MCore::Math::epsilon));
    #endif
    }


    // blend into another transform
    Transform& Transform::Blend(const Transform& dest, float weight)
    {
        mPosition = MCore::LinearInterpolate<AZ::Vector3>(mPosition, dest.mPosition, weight);
        mRotation = mRotation.NLerp(dest.mRotation, weight);

        EMFX_SCALECODE
        (
            //mScaleRotation    = mScaleRotation.NLerp( dest.mScaleRotation, weight );
            mScale          = MCore::LinearInterpolate<AZ::Vector3>(mScale, dest.mScale, weight);
        )

        return *this;
    }


    // additive blend
    Transform& Transform::BlendAdditive(const Transform& dest, const Transform& orgTransform, float weight)
    {
        const AZ::Vector3    relPos         = dest.mPosition - orgTransform.mPosition;
        const MCore::Quaternion& orgRot     = orgTransform.mRotation;
        const MCore::Quaternion rot         = orgRot.NLerp(dest.mRotation, weight);

        // apply the relative changes
        mRotation = mRotation * (orgRot.Conjugated() * rot);
        mRotation.Normalize();
        mPosition += (relPos * weight);

        EMFX_SCALECODE
        (
            mScale += (dest.mScale - orgTransform.mScale) * weight;
        )

        return *this;
    }


    Transform& Transform::ApplyAdditive(const Transform& additive)
    {
        mPosition += additive.mPosition;
        mRotation = mRotation * additive.mRotation;

        EMFX_SCALECODE
        (
            mScale += additive.mScale;
        )
        return *this;
    }


    Transform& Transform::ApplyAdditive(const Transform& additive, float weight)
    {
        mPosition += additive.mPosition * weight;
        mRotation = mRotation.NLerp(mRotation * additive.mRotation, weight);

        EMFX_SCALECODE
        (
            mScale += additive.mScale * weight;
        )
        return *this;
    }


    // sum the transforms
    Transform& Transform::Add(const Transform& other, float weight)
    {
        mPosition += other.mPosition    * weight;

        // make sure we use the correct hemisphere
        if (mRotation.Dot(other.mRotation) < 0.0f)
        {
            mRotation += -(other.mRotation) * weight;
        }
        else
        {
            mRotation += other.mRotation * weight;
        }

        EMFX_SCALECODE
        (
            mScale += other.mScale * weight;
        )

        return *this;
    }


    // add a transform
    Transform& Transform::Add(const Transform& other)
    {
        mPosition += other.mPosition;
        mRotation += other.mRotation;
        EMFX_SCALECODE
        (
            mScale += other.mScale;
        )
        return *this;
    }


    // subtract a transform
    Transform& Transform::Subtract(const Transform& other)
    {
        mPosition -= other.mPosition;
        mRotation -= other.mRotation;
        EMFX_SCALECODE
        (
            mScale -= other.mScale;
        )
        return *this;
    }


    // log the transform
    void Transform::Log(const char* name) const
    {
        if (name)
        {
            MCore::LogInfo("Transform(%s):", name);
        }

        MCore::LogInfo("mPosition = %.6f, %.6f, %.6f", 
            static_cast<float>(mPosition.GetX()), 
            static_cast<float>(mPosition.GetY()), 
            static_cast<float>(mPosition.GetZ()));
        MCore::LogInfo("mRotation = %.6f, %.6f, %.6f, %.6f", mRotation.x, mRotation.y, mRotation.z, mRotation.w);

        EMFX_SCALECODE
        (
            MCore::LogInfo("mScale    = %.6f, %.6f, %.6f", 
                static_cast<float>(mScale.GetX()), 
                static_cast<float>(mScale.GetY()), 
                static_cast<float>(mScale.GetZ()));
        )
    }


    // apply mirror flags to this transformation
    void Transform::ApplyMirrorFlags(Transform* inOutTransform, uint8 mirrorFlags)
    {
        if (mirrorFlags == 0)
        {
            return;
        }

        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_X)
        {
            inOutTransform->mRotation.w *= -1.0f;
            inOutTransform->mRotation.x *= -1.0f;
            inOutTransform->mPosition.SetY(inOutTransform->mPosition.GetY() * -1.0f);
            inOutTransform->mPosition.SetZ(inOutTransform->mPosition.GetZ() * -1.0f);
            return;
        }
        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_Y)
        {
            inOutTransform->mRotation.w *= -1.0f;
            inOutTransform->mRotation.y *= -1.0f;
            inOutTransform->mPosition.SetX(inOutTransform->mPosition.GetX() * -1.0f);
            inOutTransform->mPosition.SetZ(inOutTransform->mPosition.GetZ() * -1.0f);
            return;
        }
        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_Z)
        {
            inOutTransform->mRotation.w *= -1.0f;
            inOutTransform->mRotation.z *= -1.0f;
            inOutTransform->mPosition.SetX(inOutTransform->mPosition.GetX() * -1.0f);
            inOutTransform->mPosition.SetY(inOutTransform->mPosition.GetY() * -1.0f);
            return;
        }
    }


    // apply the mirrored version of the delta to this transformation
    void Transform::ApplyDeltaMirrored(const Transform& sourceTransform, const Transform& targetTransform, const AZ::Vector3& mirrorPlaneNormal, uint8 mirrorFlags)
    {
        // calculate the delta from source towards target transform
        Transform invSourceTransform = sourceTransform;
        invSourceTransform.Inverse();

        Transform delta = targetTransform;
        delta.Multiply(invSourceTransform);

        Transform::ApplyMirrorFlags(&delta, mirrorFlags);

        // mirror the delta over the plane which the specified normal
        delta.Mirror(mirrorPlaneNormal);

        // apply the mirrored delta to the current transform
        PreMultiply(delta);
    }


    // apply the delta from source towards target transform to this transformation
    void Transform::ApplyDelta(const Transform& sourceTransform, const Transform& targetTransform)
    {
        // calculate the delta from source towards target transform
        Transform invSourceTransform = sourceTransform;
        invSourceTransform.Inverse();

        Transform delta = targetTransform;
        delta.Multiply(invSourceTransform);

        PreMultiply(delta);
    }


    // apply the delta from source towards target transform to this transformation
    void Transform::ApplyDeltaWithWeight(const Transform& sourceTransform, const Transform& targetTransform, float weight)
    {
        // calculate the delta from source towards target transform
        Transform invSourceTransform = sourceTransform;
        invSourceTransform.Inverse();

        Transform targetDelta = targetTransform;
        targetDelta.Multiply(invSourceTransform);

        Transform finalDelta; // inits at identity
        finalDelta.Blend(targetDelta, weight);

        // apply the delta to the current transform
        PreMultiply(finalDelta);
    }


    void Transform::ApplyMotionExtractionFlags(EMotionExtractionFlags flags)
    {
        // Only keep translation over the XY plane and assume a height of 0.
        if (!(flags & MOTIONEXTRACT_CAPTURE_Z))
        {
            mPosition.SetZ(0.0f);
        }

        // Only keep the rotation on the Z axis.
        mRotation.x = 0.0f;
        mRotation.y = 0.0f;
        mRotation.Normalize();
    }


    // Return a version of this transform projected to the ground plane.
    Transform Transform::ProjectedToGroundPlane() const
    {
        Transform result(*this);

        // Only keep translation over the XY plane and assume a height of 0.
        result.mPosition.SetZ(0.0f);

        // Only keep the rotation on the Z axis.
        result.mRotation.x = 0.0f;
        result.mRotation.y = 0.0f;
        result.mRotation.Normalize();

        return result;
    }
}   // namespace EMotionFX
