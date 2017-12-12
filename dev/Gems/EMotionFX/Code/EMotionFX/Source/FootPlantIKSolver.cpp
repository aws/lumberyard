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

#include "EMotionFXConfig.h"
#include "FootPlantIKSolver.h"
#include "Actor.h"
#include "Node.h"
#include "ActorInstance.h"
#include "GlobalPose.h"


namespace EMotionFX
{
    // constructor
    FootPlantIKSolver::FootPlantIKSolver(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex, Callback* callback)
        : TwoLinkIKSolver(actorInstance, startNodeIndex, midNodeIndex, endNodeIndex)
    {
        mCallback = callback;
        Initialize(actorInstance);
    }


    // destructor
    FootPlantIKSolver::~FootPlantIKSolver()
    {
        mCallback->Destroy();
    }


    // creation method
    FootPlantIKSolver* FootPlantIKSolver::Create(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex, Callback* callback)
    {
        return new FootPlantIKSolver(actorInstance, startNodeIndex, midNodeIndex, endNodeIndex, callback);
    }


    // switch the bend dir mode
    void FootPlantIKSolver::SetCustomBendDirEnabled(bool enabled)
    {
        if (enabled == mCustomBendDir)
        {
            return;
        }

        if (enabled == false)
        {
            MCore::Array<MCore::Matrix> matrices;
            mActorInstance->GetActor()->GetSkeleton()->CalcBindPoseGlobalMatrices(matrices);

            // hope it's facing us on Z Positive axis :) getting the backfacing vector
            MCore::Vector3 local = matrices[mMidNodeIndex].Inversed().Mul3x3(MCore::Vector3(0.0f, 0.0f, -1.0f)); // TODO: does this work in other coordinate systems at all?
            TwoLinkIKSolver::SetRelativeBendDir(local, mMidNodeIndex);
            TwoLinkIKSolver::SetUseRelativeBendDir(true);
        }

        mCustomBendDir = enabled;
    }


    // initialize the controller
    void FootPlantIKSolver::Initialize(ActorInstance* actorInstance)
    {
        MCore::Array<MCore::Matrix> matrices;
        Actor* actor = actorInstance->GetActor();

        actor->GetSkeleton()->CalcBindPoseGlobalMatrices(matrices);

        // hope it's facing us on Z Positive axis :) getting the backfacing vector
        MCore::Vector3 local = matrices[mMidNodeIndex].Inversed().Mul3x3(MCore::Vector3(0.0f, 0.0f, -1.0f)); // TODO: is this even correct with other coordinate systems?

        TwoLinkIKSolver::SetRelativeBendDir(local, mMidNodeIndex);
        TwoLinkIKSolver::SetUseRelativeBendDir(true);

        mFootDelta              = matrices[mEndNodeIndex].Inversed();
        mFootParentIndex        = actor->GetSkeleton()->GetNode(mEndNodeIndex)->GetParentIndex();
        mFootBindpose           = matrices[mEndNodeIndex] * matrices[mFootParentIndex].Inversed();
        mAltitude.SetNextValue  (0.0f, 0.0f, 0.0f);
        mLastHitAltitude        = 0.0f;
        mCurrentDelta           = 0.0f;
        mGroundAltitude         = 0.0f;
        mMinimumPelvisDistance  = -1.0f;// -1=no minimum, let it free
        mCustomBendDir          = false;
        mPelvisIndex            = 1;
        SetAdaptationSpeeds     (0.0f, 0.0f, 0.0f, 0.0f);
        SetStiffness            (0.25f);
        //mAngle                = 0.0f;
        //mLastNormal           = Vector3( 0.0f, 1.0f, 0.0f );

        SetFootBehavior(FB_GROUND_ALIGN);
        //  SetFootPitchLimits( -45.0f * ( Math::pi/180.0f ), 45.f * ( Math::pi / 180.0f ) );
        SetFootPitchLimits(MCore::Math::DegreesToRadians(-45.0f), MCore::Math::DegreesToRadians(45.f));
    }


    // get the controller type
    uint32 FootPlantIKSolver::GetType() const
    {
        return FootPlantIKSolver::TYPE_ID;
    }


    // get the type string
    const char* FootPlantIKSolver::GetTypeString() const
    {
        return "FootPlantIKSolver";
    }


    // clone the controller
    GlobalSpaceController* FootPlantIKSolver::Clone(ActorInstance* targetActorInstance)
    {
        // create the clone
        // TODO: cannot copy over the raytrace handler as the raytrace handler has no Clone function, to solve this we can use smart pointers or introduce a clone function to the callback
        FootPlantIKSolver* clone = new FootPlantIKSolver(targetActorInstance, mStartNodeIndex, mMidNodeIndex, mEndNodeIndex, nullptr);

        // setup the same user defined properties
        clone->SetGoalNodeIndex(mGoalNodeIndex);
        clone->SetGoal(mGoal);
        clone->SetBendDirection(mBendDirection);
        clone->SetPelvisMinimumDistance(mPelvisIndex, mMinimumPelvisDistance);
        clone->SetAdaptationSpeeds(mAdaptSpeedLow, mAdaptThresholdLow, mAdaptSpeedHigh, mAdaptThresholdHigh);
        clone->SetFootBehavior(mFootBehavior);
        clone->SetStiffness(mStiffness);
        clone->SetMainGroundAltitude(mGroundAltitude);
        clone->SetCustomBendDirEnabled(mCustomBendDir);

        // copy the base class weight settings
        // this makes sure the controller is in the same activated/deactivated and blending state
        // as the source controller that we are cloning
        clone->CopyBaseClassWeightSettings(this);

        // return the clone
        return clone;
    }


    // the main update method, which performs the modifications and calculations of the new global space matrices
    void FootPlantIKSolver::Update(GlobalPose* outPose, float timePassedInSeconds)
    {
        mCallback->AdjustCharacterAltitude(mActorInstance, this);

        MCore::Matrix* globalMatrices = outPose->GetGlobalMatrices();

        // get actual foot position, compute delta ground/foot
        MCore::Vector3          hit;
        MCore::Vector3          normal;
        // MCore::Vector3       hipPos  = globalMatrices[mStartNodeIndex].GetTranslation();
        const MCore::Vector3&   kneePos = globalMatrices[mMidNodeIndex].GetTranslation();
        const MCore::Vector3&   footPos = globalMatrices[mEndNodeIndex].GetTranslation();
        MCore::Vector3          rayPos  = footPos;

        // start from foot position, get knee altitude, start from there downward
        // assuming that y axis, is the altitude
        if (footPos.z < kneePos.z)
        {
            rayPos.z = kneePos.z;
        }

        // if no hit point, current foot position will be used
        MCore::Vector3 direction(0.0f, 0.0f, -1.0f);
        hit = footPos;
        mCallback->GetHitPoint(rayPos, direction, &hit, &normal);

        if (mMinimumPelvisDistance > 0.0f)
        {
            const MCore::Vector3& pelvisPos = globalMatrices[mPelvisIndex].GetTranslation();
            if ((pelvisPos.z - hit.z) < mMinimumPelvisDistance)
            {
                hit.z = pelvisPos.z - mMinimumPelvisDistance;
            }
        }

        // get the new ground altitude, but interpolate the tweaking to smooth things
        // detect a change in hit altitude (where the foot has to be planted)
        if (MCore::Math::Abs(hit.z - mLastHitAltitude) > MCore::Math::epsilon)
        {
            //  feet correction has changed i need to do some modifications in the current interpolation
            const float delta           = hit.z - mLastHitAltitude;
            const float adaptThreshold  = mAdaptThresholdLow * (1.0f - mStiffness) + mAdaptThresholdHigh * mStiffness;
            const float adaptSpeed      = mAdaptSpeedLow * (1.0f - mStiffness) + mAdaptSpeedHigh * mStiffness;
            float       adaptDuration   = 0.0f;

            if (MCore::Math::Abs(adaptSpeed) > MCore::Math::epsilon)
            {
                if (MCore::Math::Abs(delta) > adaptThreshold)
                {
                    adaptDuration = adaptThreshold / adaptSpeed;
                }
                else
                {
                    adaptDuration = MCore::Math::Abs(delta) / adaptSpeed;
                }
            }

            // the smallest interpolation duration, the better
            // by doing this, next frame is already an 'intermediary' position: on my way
            mAltitude.SetNextValue(hit.z, 0.0f, timePassedInSeconds + adaptDuration);
            mLastHitAltitude = hit.z;
        }

        mCurrentDelta = mAltitude.Update(timePassedInSeconds) - mGroundAltitude;

        // assuming that Z axis is altitude
        const MCore::Vector3 goalPos = footPos + MCore::Vector3(0.0f, 0.0f, mCurrentDelta);

        // finally we're using parent class
        TwoLinkIKSolver::SetGoal(goalPos);

        // backup old global foot orientation, for further use
        MCore::Matrix footGTMBackup;
        if (mFootBehavior != FB_NO_CORRECTION)
        {
            footGTMBackup = globalMatrices[mEndNodeIndex];
        }

        // solve our goal problem
        TwoLinkIKSolver::Update(outPose, timePassedInSeconds);

        // conforming foot
        if (mFootBehavior != FB_NO_CORRECTION)
        {
            if (mFootBehavior == FB_GROUND_ALIGN)
            {
                MCore::Matrix footSpace = mFootDelta * footGTMBackup;
                MCore::Matrix footReference = mFootDelta * mFootBindpose * globalMatrices[mFootParentIndex].Inversed();
                MCore::Matrix delta     = footSpace * footReference.Inversed();
                const MCore::Vector3& zAxis = delta.GetRow(2).SafeNormalized();
                float currentAngle  = MCore::Math::ASin(zAxis.z);

                // requested elevation
                MCore::Matrix ident;
                ident.Identity();
                ident.SetRow(0, MCore::Vector3(footSpace.GetRow(0).x, footSpace.GetRow(0).y, 0.0f).SafeNormalized());
                ident.SetRow(1, MCore::Vector3(footSpace.GetRow(2).x, footSpace.GetRow(2).y, 0.0f).SafeNormalized());
                ident.SetRow(2, MCore::Vector3(0.0f, 0.0f, 1.0f));

                const MCore::Vector3    localNormal     = ident.Inversed().Mul3x3(normal);
                float                   requestedAngle  = MCore::Math::ATan2(localNormal.y, localNormal.z);

                float ratio;
                if (requestedAngle < 0.0f)
                {
                    //  z axis/foot going upward
                    if (requestedAngle < mFootPitchLow)
                    {
                        requestedAngle = mFootPitchLow;
                    }
                }
                else
                {
                    // z axis/foot going downward
                    if (requestedAngle > mFootPitchHigh)
                    {
                        requestedAngle = mFootPitchHigh;
                    }
                }
                if (currentAngle < 0.0f)
                {
                    if (currentAngle < mFootPitchLow)
                    {
                        currentAngle = mFootPitchLow;
                    }

                    if (MCore::Math::Abs(mFootPitchLow) > MCore::Math::epsilon)
                    {
                        ratio = currentAngle / mFootPitchLow;
                    }
                    else
                    {
                        ratio = 0.0f;
                    }
                }
                else
                {
                    if (currentAngle > mFootPitchHigh)
                    {
                        currentAngle = mFootPitchHigh;
                    }

                    if (MCore::Math::Abs(mFootPitchHigh) > MCore::Math::epsilon)
                    {
                        ratio = currentAngle / mFootPitchHigh;
                    }
                    else
                    {
                        ratio = 0.0f;
                    }
                }

                ratio = 1.0f - ratio;
                MCore::Clamp<float>(ratio, 0.0f, 1.0f);

                const float pitch = requestedAngle * ratio;

                MCore::Matrix rotate;
                rotate.SetRotationMatrixAxisAngle(mFootDelta.GetRow(0).SafeNormalized(), pitch);
                footGTMBackup = rotate * footGTMBackup;
            }

            // and updating feet
            footGTMBackup.SetTranslation(globalMatrices[mEndNodeIndex].GetTranslation());
            mActorInstance->RecursiveUpdateGlobalTM(mEndNodeIndex, &footGTMBackup, globalMatrices);
        }

        // you can still call HasFoundSolution after that
        mCallback->UpdateTwistBones(globalMatrices);
    }


    void FootPlantIKSolver::SetMainGroundAltitude(float altitude)
    {
        mGroundAltitude = altitude;
    }


    void FootPlantIKSolver::SetPelvisMinimumDistance(uint32 pelvisIndex, float minimumDistance)
    {
        mPelvisIndex = pelvisIndex;
        mMinimumPelvisDistance = minimumDistance;
    }


    void FootPlantIKSolver::SetAdaptationSpeeds(float adaptSpeedLow, float adaptThresholdLow, float adaptSpeedHigh, float adaptThresholdHigh)
    {
        mAdaptSpeedLow      = adaptSpeedLow;
        mAdaptThresholdLow  = adaptThresholdLow;
        mAdaptSpeedHigh     = adaptSpeedHigh;
        mAdaptThresholdHigh = adaptThresholdHigh;
    }


    void FootPlantIKSolver::SetStiffness(float stiffness)
    {
        mStiffness = stiffness;
    }


    void FootPlantIKSolver::SetFootBehavior(EFootBehaviour footBehavior)
    {
        mFootBehavior = footBehavior;
    }


    void FootPlantIKSolver::SetFootPitchLimits(float lowPitch, float highPitch)
    {
        mFootPitchLow = lowPitch;
        mFootPitchHigh = highPitch;
    }


    FootPlantIKSolver::Callback* FootPlantIKSolver::GetCallback() const
    {
        return mCallback;
    }


    bool FootPlantIKSolver::GetCustomBendDirEnabled() const
    {
        return mCustomBendDir;
    }


    //------------------------------------------------------------------------
    // FootPlantIKSolver::SmoothValueInterpolator
    //------------------------------------------------------------------------

    // default constructor
    FootPlantIKSolver::SmoothValueInterpolator::SmoothValueInterpolator()
    {
        mValues[0]  = 0.0f;
        mValues[1]  = 0.0f;
        mTimes[0]   = 0.0f;
        mTimes[1]   = 0.0f;
        mSpeeds[0]  = 0.0f;
        mSpeeds[1]  = 0.0f;
        mCurrentTime = 0.0f;
        mCurrentValue = 0.0f;
        mCurrentSpeed = 0.0f;
    };


    // constructor
    FootPlantIKSolver::SmoothValueInterpolator::SmoothValueInterpolator(float defaultValue)
    {
        mValues[0]  = defaultValue;
        mValues[1]  = defaultValue;
        mTimes[0]   = 0.0f;
        mTimes[1]   = 0.0f;
        mSpeeds[0]  = 0.0f;
        mSpeeds[1]  = 0.0f;
        mCurrentTime = 0.0f;
        mCurrentValue = 0.0f;
        mCurrentSpeed = 0.0f;
    }


    // constructor
    void FootPlantIKSolver::SmoothValueInterpolator::SetNextValue(float value, float speed, float duration)
    {
        mValues[0]  = mCurrentValue;
        mValues[1]  = value;
        mSpeeds[0]  = mCurrentSpeed;
        mSpeeds[1]  = speed;
        mTimes[0]   = mCurrentTime;
        mTimes[1]   = mCurrentTime + duration;
    }


    // cubic hermite spline basis marices
    // T being the time matrix
    // B being the basis matrix (depends on the spline type)
    // Compute T*B   =>   [B00     B03]
    //                    [           ]
    //                    [           ]
    //                    [B30     B33]
    //      [T0 T1 T2 T3] [BT0 ... BT3]
    float FootPlantIKSolver::SmoothValueInterpolator::HermiteBasis[4][4] =
    {
        {  2.0f, -2.0f,  1.0f,  1.0f },
        { -3.0f,  3.0f, -2.0f, -1.0f },
        {  0.0f,  0.0f,  1.0f,  0.0f },
        {  1.0f,  0.0f,  0.0f,  0.0f }
    };

    float FootPlantIKSolver::SmoothValueInterpolator::Hermite1stDerivBasis[3][4] =
    {
        {  6.0f, -6.0f,  3.0f,  3.0f },
        { -6.0f,  6.0f, -4.0f, -2.0f },
        {  0.0f,  0.0f,  1.0f,  0.0f }
    };

    float FootPlantIKSolver::SmoothValueInterpolator::Hermite2ndDerivBasis[2][4] =
    {
        {  12.0f, -12.0f,  6.0f,  6.0f },
        {  -6.0f,   6.0f, -4.0f, -2.0f }
    };


    void FootPlantIKSolver::SmoothValueInterpolator::ComputeTimeBasis(float basis0[4][4], float t, float B[])
    {
        MCORE_ASSERT(t >= 0.0f && t <= 1.0f);

        // building time matrix
        float s[4];
        s[3] = 1.0f;
        s[2] = t;
        s[1] = t * t;
        s[0] = t * t * t;

        // this is basis matrix
        for (int c = 0; c < 4; ++c)
        {
            B[c] = 0.0f;
            for (int r = 0; r < 4; ++r)
            {
                B[c] += s[r] * basis0[r][c];
            }
        }
    }


    // T * B * G
    // evaluating spline tangents (speed)
    // generate P0, P1, T0, T1 coefficients for T=P0*B[0]+P1*B[1]+T0*B[2]+T1*B[3]
    void FootPlantIKSolver::SmoothValueInterpolator::Compute1stDerivTimeBasis(float basis1[4][4], float t, float B[])
    {
        MCORE_ASSERT(t >= 0.0f && t <= 1.0f);

        float s[3];
        s[0] = t * t;
        s[1] = t;
        s[2] = 1.0f;

        for (int c = 0; c < 4; ++c)
        {
            B[c] = 0.0f;
            for (int r = 0; r < 3; ++r)
            {
                B[c] += s[r] * basis1[r][c];
            }
        }
    }


    float FootPlantIKSolver::SmoothValueInterpolator::EvaluateP(float t, float basis[4][4], float P0, float T0, float P1, float T1)
    {
        float B[4];
        ComputeTimeBasis(basis, t, B);
        return P0 * B[0] + P1 * B[1] + T0 * B[2] + T1 * B[3];
    }


    float FootPlantIKSolver::SmoothValueInterpolator::EvaluateT(float t, float basis[4][4], float P0, float T0, float P1, float T1)
    {
        float B[4];
        Compute1stDerivTimeBasis(basis, t, B);
        return P0 * B[0] + P1 * B[1] + T0 * B[2] + T1 * B[3];
    }


    float FootPlantIKSolver::SmoothValueInterpolator::HermiteCubicInterpolate(float t, float x1, float v1, float x2, float v2, float* pv)
    {
        float result = EvaluateP(t, HermiteBasis, x1, v1, x2, v2);
        if (pv)
        {
            *pv = EvaluateT(t, Hermite1stDerivBasis, x1, v1, x2, v2);
        }

        return result;
    }


    // update interpolator: compute current speed and value
    float FootPlantIKSolver::SmoothValueInterpolator::Update(float elapsedTime)
    {
        mCurrentTime += elapsedTime;

        if (mCurrentTime >= mTimes[1])
        {
            mCurrentValue = mValues[1];
            mCurrentSpeed = mSpeeds[1];
        }
        else if (mCurrentTime <= mTimes[0])
        {
            mCurrentValue = mValues[0];
            mCurrentSpeed = mSpeeds[0];
        }
        else
        {
            float delta = mTimes[1] - mTimes[0];
            if (delta <= 0)
            {
                mCurrentValue = mValues[1];
                mCurrentSpeed = mSpeeds[1];
            }
            else
            {
                mCurrentValue = HermiteCubicInterpolate((mCurrentTime - mTimes[0]) / delta, mValues[0], mSpeeds[0], mValues[1], mSpeeds[1], &mCurrentSpeed);
            }
        }

        return mCurrentValue;
    }
} // namespace EMotionFX
