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

// include the required headers
#include "LookAtController.h"
#include "Node.h"
#include "ActorInstance.h"
#include "NodeLimitAttribute.h"
#include "GlobalPose.h"
#include "TransformData.h"
//#include <MCore/Source/SwingAndTwist.h>


namespace EMotionFX
{
    // constructor
    LookAtController::LookAtController(ActorInstance* actorInstance, uint32 nodeIndex)
        : GlobalSpaceController(actorInstance)
    {
        MCORE_ASSERT(nodeIndex < actorInstance->GetNumNodes());

        mNodeIndex = nodeIndex;

        mGoal.Set(0.0f, 0.0f, 0.0f);
        mInterpolationSpeed = 1.0f;

        // we need to keep a copy of the rotation
        MCore::Matrix globalTM = actorInstance->GetTransformData()->GetGlobalInclusiveMatrices()[ nodeIndex ];
        mRotationQuat.FromMatrix(globalTM.Normalized());
        mRotationQuat.Normalize();
        mPostRotation.Identity();
        mPreRotation.Identity();

        // set the limits accoring to the model's current limits if it has them set, otherwise just use default values
        NodeLimitAttribute* limit = (NodeLimitAttribute*)actorInstance->GetActor()->GetSkeleton()->GetNode(nodeIndex)->GetAttributeByType(NodeLimitAttribute::TYPE_ID);

        // set the limits
        if (limit)
        {
            SetEulerConstraints(limit->GetRotationMin(), limit->GetRotationMax());
        }
        else
        {
            mEllipseOrientation.Identity();
            mEllipseRadii.Set(1.0f, 1.0f);
            mMinMaxTwist.Set(0.0f, 0.0f);
        }

        // disable the constraints
        mConstraintType = CONSTRAINT_NONE;

        // disable EMotion FX 2 compatibility mode
        mEMFX2CompatibleMode = false;

        // specify that this controller will modify all nodes that come after the start node inside the hierarchy
        RecursiveSetNodeMask(mNodeIndex, true);
    }


    // destructor
    LookAtController::~LookAtController()
    {
    }


    // creation
    LookAtController* LookAtController::Create(ActorInstance* actorInstance, uint32 nodeIndex)
    {
        return new LookAtController(actorInstance, nodeIndex);
    }


    // reset the controller
    void LookAtController::Reset()
    {
        // get the current global space transform
        MCore::Matrix globalTM = mActorInstance->GetTransformData()->GetGlobalInclusiveMatrix(mNodeIndex);
        globalTM.Normalize(); // remove scaling

        // convert it into a quaternion and normalize it
        mRotationQuat.FromMatrix(globalTM);
        mRotationQuat.Normalize();
    }


    // the main update method
    void LookAtController::Update(GlobalPose* outPose, float timePassedInSeconds)
    {
        /*// get the global space matrices and scale values
        MCore::Matrix*      globalMatrices  = outPose->GetGlobalMatrices();
        MCore::Vector3*     globalScales    = mActorInstance->GetGlobalPose()->GetScales();

        // calculate the new forward vector
        MCore::Vector3 forward = (mGoal - globalMatrices[mNodeIndex].GetTranslation()).SafeNormalized();

        MCore::Vector3 up;
        const uint32 parentIndex = mActorInstance->GetActor()->GetSkeleton()->GetNode(mNodeIndex)->GetParentIndex();
        MCore::Matrix parentGlobalTM;
        parentGlobalTM.Identity();

        // if we have a parent node
        if (parentIndex != MCORE_INVALIDINDEX32)
        {
            parentGlobalTM = globalMatrices[parentIndex].Normalized();

            // get the up vector from the parent if it exsists
            up = (mPreRotation * parentGlobalTM).GetUp().SafeNormalized();
        }
        else // a root node, use the up vector of the node itself
        {
            up = (mPreRotation * globalMatrices[mNodeIndex].Normalized()).GetUp().SafeNormalized();
        }

        // caclculate the right and up vector we wish to use
        MCore::Vector3 right = up.Cross(forward).SafeNormalized();
        up = forward.Cross(right).SafeNormalized();

        // build the destination rotation matrix
        MCore::Matrix destRotation;
        destRotation.Identity();
        destRotation.SetRight(right);
        destRotation.SetUp(up);
        destRotation.SetForward(forward);

        // at this point, destRotation contains the global space orientation that we would like our node do be using

        // if we want to apply constraints
        if (mConstraintType != CONSTRAINT_NONE)
        {
            // find the destination relative to the node's parent if the node has one
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                destRotation = destRotation * parentGlobalTM.Inversed();
            }

            // now destRotation contains the exact orientation we wish to apply constraints to (relative to the identity matrix)

            // multiply by the inverse of the ellipse's orientation, and rotate our coordinate system into 'ellipse space'
            if (mEMFX2CompatibleMode)
            {
                destRotation = mEllipseOrientation.Inversed() * destRotation; // OLD EMotion FX 2 / EMotion FX v3.0 mul order
            }
            else
            {
                destRotation = destRotation * mEllipseOrientation.Inversed();
            }

            // calculate the swing and twist
            MCore::SwingAndTwist sAndt(destRotation);

            // apply swing constraints
            if (mConstraintType == CONSTRAINT_ELLIPSE)  // constraint using an ellipse
            {
                sAndt.ConstrainSwingWithEllipse(mEllipseRadii);
            }
            else
            if (mConstraintType == CONSTRAINT_RECTANGLE) // constraint using a rectangle
            {
                // the rectangle method will be a lot faster than the ellipse, but doesn't look anywhere near as good
                // this should probably only be used for distant models using a lower LOD

                // treat the ellipse's radii as bounds for a rectangle
                sAndt.ConstrainSwingWithRectangle(mEllipseRadii);
            }

            // apply twist constraints
            sAndt.ConstrainTwist(mMinMaxTwist);

            // convert the swing and twist back to a matrix
            destRotation = sAndt.ToMatrix();

            // rotate back out of ellipse space
            if (mEMFX2CompatibleMode)
            {
                destRotation = mEllipseOrientation * destRotation; // OLD EMotion FX 2 / EMotion FX v3.0 mul order
            }
            else
            {
                destRotation = destRotation * mEllipseOrientation;
            }

            // if we converted our coord system into the space relative to the parent, then convert it back to global space
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                destRotation = destRotation * parentGlobalTM;
            }
        }

        // apply the post rotation matrix
        destRotation = mPostRotation * destRotation;

        // interpolate between the current rotation and the destination rotation
        float speed = mInterpolationSpeed * timePassedInSeconds * 6.0f;
        speed = MCore::Clamp<float>(speed, 0.0f, 1.0f);
        mRotationQuat = mRotationQuat.Slerp(MCore::Quaternion(destRotation), speed);
        mRotationQuat.Normalize(); // TODO: this normally shouldn't be needed as Slerp already normalizes, still it is needed to prevent some suddenly introduced scaling

        // create the rotation matrix from the rotation quat
        destRotation = mRotationQuat.ToMatrix();

        // scale the matrix and put it back in the right position
        destRotation.Scale3x3(globalScales[mNodeIndex]);
        destRotation.SetTranslation(globalMatrices[mNodeIndex].GetTranslation());

        // update the matrix with forward kinematics
        mActorInstance->RecursiveUpdateGlobalTM(mNodeIndex, &destRotation, globalMatrices);*/
    }



    // Calculate the Ellipse radii and the twist value from the min and max euler angles
    void LookAtController::SetEulerConstraints(const MCore::Vector3& minVals, const MCore::Vector3& maxVals)
    {
        // twist is z
        mMinMaxTwist.SetX(minVals.z);
        mMinMaxTwist.SetY(maxVals.z);

        // Offset is the average
        const float xOffset = (minVals.x + maxVals.x) * 0.5f;
        const float yOffset = (minVals.y + maxVals.y) * 0.5f;

        // these lines are the same as above
        mEllipseRadii.SetX(maxVals.x - xOffset);
        mEllipseRadii.SetY(maxVals.y - yOffset);

        // build the apropriate matrices
        MCore::Matrix xOffsetMat, yOffsetMat, zRot;
        xOffsetMat.SetRotationMatrixX(xOffset);
        yOffsetMat.SetRotationMatrixY(yOffset);
        mEllipseOrientation = xOffsetMat * yOffsetMat;

        // TODO: check if this is required
        // basically just swaps the x and y axii
        if (mEMFX2CompatibleMode)
        {
            zRot.SetRotationMatrixZ(-MCore::Math::halfPi);
            mEllipseOrientation = zRot * mEllipseOrientation;
        }
    }



    // clone the controller
    GlobalSpaceController* LookAtController::Clone(ActorInstance* targetActorInstance)
    {
        // create the new constraint and copy its members
        LookAtController* newController = new LookAtController(targetActorInstance, mNodeIndex);

        // copy the settings
        newController->mPreRotation         = mPreRotation;
        newController->mPostRotation        = mPostRotation;
        newController->mEllipseOrientation  = mEllipseOrientation;
        newController->mRotationQuat        = mRotationQuat;
        newController->mGoal                = mGoal;
        newController->mEllipseRadii        = mEllipseRadii;
        newController->mMinMaxTwist         = mMinMaxTwist;
        newController->mInterpolationSpeed  = mInterpolationSpeed;
        newController->mConstraintType      = mConstraintType;

        // copy the base class weight settings
        // this makes sure the controller is in the same activated/deactivated and blending state
        // as the source controller that we are cloning
        newController->CopyBaseClassWeightSettings(this);

        // return the new controller
        return newController;
    }


    // get the type ID
    uint32 LookAtController::GetType() const
    {
        return LookAtController::TYPE_ID;
    }


    // get the type string
    const char* LookAtController::GetTypeString() const
    {
        return "LookAtController";
    }


    // get the goal
    const MCore::Vector3& LookAtController::GetGoal() const
    {
        return mGoal;
    }


    // set the goal position
    void LookAtController::SetGoal(const MCore::Vector3& goalGlobalSpacePos)
    {
        mGoal = goalGlobalSpacePos;
    }


    // set the interpolation speed
    void LookAtController::SetInterpolationSpeed(float factor)
    {
        mInterpolationSpeed = factor;
    }


    // get the interplation speed
    float LookAtController::GetInterpolationSpeed() const
    {
        return mInterpolationSpeed;
    }


    // set the pre-rotation matrix
    void LookAtController::SetPreRotation(const MCore::Matrix& mat)
    {
        mPreRotation = mat;
    }


    // get the pre-rotation matrix
    const MCore::Matrix& LookAtController::GetPreRotation() const
    {
        return mPreRotation;
    }


    // set the post rotation matrix
    void LookAtController::SetPostRotation(const MCore::Matrix& mat)
    {
        mPostRotation = mat;
    }


    // get the post rotation matrix
    const MCore::Matrix& LookAtController::GetPostRotation() const
    {
        return mPostRotation;
    }


    // set the ellipse orientation
    void LookAtController::SetEllipseOrientation(const MCore::Matrix& mat)
    {
        mEllipseOrientation = mat;
    }


    // get the ellipse orientation
    const MCore::Matrix& LookAtController::GetEllipseOrientation() const
    {
        return mEllipseOrientation;
    }


    // set the ellipse radii
    void LookAtController::SetEllipseRadii(const AZ::Vector2& radii)
    {
        mEllipseRadii = radii;
    }


    // set the ellipse radii using two floats
    void LookAtController::SetEllipseRadii(float rx, float ry)
    {
        mEllipseRadii.Set(rx, ry);
    }


    // set the twist rotation limits
    void LookAtController::SetTwist(float minValue, float maxValue)
    {
        mMinMaxTwist.Set(minValue, maxValue);
    }


    // set the twist rotation limits using a Vector2
    void LookAtController::SetTwist(const AZ::Vector2& minMaxTwist)
    {
        mMinMaxTwist = minMaxTwist;
    }


    // set the constraint type
    void LookAtController::SetConstraintType(EConstraintType constraintType)
    {
        mConstraintType = constraintType;
    }


    // get the constraint type
    LookAtController::EConstraintType LookAtController::GetConstraintType() const
    {
        return mConstraintType;
    }


    // enable/disable EMotion FX 2 compatibility mode
    void LookAtController::SetEMFX2CompatbilityModeEnabled(bool enabled)
    {
        mEMFX2CompatibleMode = enabled;
    }


    // check EMotion FX 2 compatibility mode
    bool LookAtController::GetIsEMFX2CompatbilityModeEnabled() const
    {
        return mEMFX2CompatibleMode;
    }
} // namespace EMotionFX
