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

#pragma once

// include the required files
#include "EMotionFXConfig.h"
#include "GlobalSpaceController.h"
#include "Transform.h"

#include <MCore/Source/Vector.h>
#include <MCore/Source/Matrix4.h>
#include <MCore/Source/Quaternion.h>


namespace EMotionFX
{
    /**
     * The lookat controller.
     * This can be used to make node point towards (look at) a given goal.
     * This is for example nice to apply to a character's head and eyes if you want to make them look at a given point.
     * Constraints are also implemented here. Not using regular rotation constraints, but using special constraints using
     * spherical ellipsoids that you can setup as well. This results in nice smooth, more natural looking limits.
     */
    class EMFX_API LookAtController
        : public GlobalSpaceController
    {
    public:
        /**
         * The controller type ID as returned by GetType().
         */
        enum
        {
            TYPE_ID = 0x00010001
        };

        /**
         * The different types of constraints that can be applied.
         */
        enum EConstraintType
        {
            CONSTRAINT_NONE         = 0,    /**< Do not use any constraints. */
            CONSTRAINT_ELLIPSE      = 1,    /**< Contraint using an ellipse, which is the nicest. */
            CONSTRAINT_RECTANGLE    = 2     /**< Constraint using a rectangle, which doesn't look nice, but is fast (use it on low LOD levels). */
        };

        /**
         * The creation method.
         * @param actorInstance The actor instance on which this controller will work.
         * @param nodeIndex The node number to apply the controller on.
         */
        static LookAtController* Create(ActorInstance* actorInstance, uint32 nodeIndex);

        /**
         * The main update method, which performs the modifications and calculations of the new global space matrices.
         * @param outPose The global space output pose where you have to output your calculated global space matrices in.
         *                This output pose is also the input. It will contain the current set of global space matrices for all nodes.
         * @param timePassedInSeconds The time passed, in seconds, since the last update.
         */
        void Update(GlobalPose* outPose, float timePassedInSeconds) override;

        /**
         * Clone the global space contoller.
         * This creates and returns an exact copy of this controller.
         * Be sure to also use the CopyBaseClassWeightSettings member to give the clone the same weight settings as this controller.
         * @param targetActorInstance The actor instance on which you want to use the cloned controller that will be returned.
         * @result A pointer to a clone of the current controller.
         */
        GlobalSpaceController* Clone(ActorInstance* targetActorInstance) override;

        /**
         * Get the controller type.
         * This has to be a unique ID per global space controller type/class and can be used to identify
         * what type of controller you are dealing with.
         * @result The unique controller type ID.
         */
        uint32 GetType() const override;

        /**
         * Get a description of the global space controller type.
         * This can for example be the name of the class, which can be useful when debugging.
         * @result A string containing the type description of this class.
         */
        const char* GetTypeString() const override;

        /**
         * Reset the controller, so that the initial rotation of the control is set to the current global space
         * rotation of the node the controller works on.
         * If you deactivate the controller, and later reactivate, the rotation the controller will use to blend towards
         * will be the last rotation before deactivation was completed.
         */
        void Reset();

        /**
         * Get the goal position, this is used in case there is no target node set.
         * @result The goal position in global space.
         */
        const MCore::Vector3& GetGoal() const;

        /**
         * Set the goal global space position. This position is used in case there is no target node set.
         * @param goalGlobalSpacePos The global space position where the node should look at.
         */
        void SetGoal(const MCore::Vector3& goalGlobalSpacePos);

        /**
         * Get the node on which we apply this look-at controller.
         * This is the node index you passed to the constructor.
         * @result The node index of the node on which we apply this controller.
         */
        MCORE_INLINE uint32 GetNodeIndex() const                                { return mNodeIndex; }

        /**
         * Set the interpolation speed factor, which is how fast/smooth the node will rotate to point at the goal.
         * On default this value is set to 1.0f.
         * @param factor The new interpolation speed factor.
         */
        void SetInterpolationSpeed(float factor);

        /**
         * Get the interpolation speed factor, which is how fast/smooth the node will rotate to point at the goal.
         * On default this value is set to 1.0f.
         * @result The interpolation speed factor.
         */
        float GetInterpolationSpeed() const;

        /**
         * Set the pre-rotation matrix. This matrix rotates the node's up-vector, which will be used during the calculations.
         * This is useful when you expect the up vector of the head to point upwards, while it actually points forward mathematically.
         * You can then set a pre-rotation matrix that would rotate the forward vector into a vector that points upwards to get the desired result.
         * If you do not do this, the head might face towards the goal with the wrong direction, for example with the top of the head instead of the face itself.
         * @param mat The pre-rotation matrix, which is identity on default.
         */
        void SetPreRotation(const MCore::Matrix& mat);

        /**
         * Get the current pre-rotation matrix. For more information about what exactly the pre-rotation matrix is, please read
         * the documentation of the SetPreRotation method.
         * @result The matrix containing the pre-rotation which rotates the original up-vector in the desired up-vector.
         * @see SetPreRotation
         */
        const MCore::Matrix& GetPreRotation() const;

        /**
         * Set the post rotation matrix. This matrix is being applied after the new orientation of the node has been calculated.
         * If you use a pre-rotation, or if the object already was rotated, you most likely have to specify a post rotation that
         * will rotate the object back into the desired orientation.
         * Also sometimes you might want the head to look a bit more downwards on default. The post rotation matrix, which you specify here, can
         * be used to add a little bit of extra rotation to the node, after the lookat has been applied. So we could rotate the head a bit, so it
         * will look more downwards, which might give a more desired effect.
         * @param mat The post rotation matrix. On default this is an identity matrix.
         */
        void SetPostRotation(const MCore::Matrix& mat);

        /**
        * Get the current post rotation matrix. On default this is set to identity.
        * You can read more about what exactly the post rotation matrix is in the documentation of the SetPostRotation method.
        * @result The post rotation matrix.
        * @see SetPostRotation
        */
        const MCore::Matrix& GetPostRotation() const;

        /**
         * Set the orientation of the ellipse. This must be a rotation-only matrix.
         * The ellipse is projected onto a sphere. The center of the ellipse will be projected the center of the sphere
         * that has been rotated using the ellipse orientation matrix. The area of the ellipse marks the area of valid rotations of the look-at node.
         * That means that the rotations will be constraint to be inside the ellipse area.
         * @param mat The rotation matrix that marks the center of the ellipse.
         */
        void SetEllipseOrientation(const MCore::Matrix& mat);

        /**
         * Get the orientation matrix of the ellipse.
         * For more information about what exactly this orientation is, please read the documentation of the SetEllipseOrientation method.
         * @result The ellipse orientation (rotation) matrix.
         * @see SetEllipseOrientation
         */
        const MCore::Matrix& GetEllipseOrientation() const;

        /**
         * Set the width and height of the ellipse that will be projected on a unit sphere.
         * This defines the rotational limits of the look-at node.
         * @param radii The width and height of the ellipse.
         */
        void SetEllipseRadii(const AZ::Vector2& radii);

        /**
         * Set the width and height of the ellipse, using two floats instead of a Vector2.
         * @param rx The width of the ellipse.
         * @param ry The height of the ellipse.
         */
        void SetEllipseRadii(float rx, float ry);

        /**
        * Set the minimum and maximum twist angles, in radians.
        * @param minValue The minimum twist value, in radians.
        * @param maxValue The maximum twist value, in radians.
        */
        void SetTwist(float minValue, float maxValue);

        /**
         * Set the minimum and maximum twist angles, in radians, by using a Vector2.
         * The x component must contain the minimum twist, and the y component the maximum.
         * @param minMaxTwist The minimum and maximum twist angles, in radians.
         */
        void SetTwist(const AZ::Vector2& minMaxTwist);

        /**
         * Set the constraints via euler angles instead of ellipse width and height.
         * This converts the input into the radii for the ellipse and the twist value.
         * The Vector3 x, y, z, stand for the rotation angles around each axis, in radians.
         * @param minVals The minimum euler angles, in radians.
         * @param maxVals The maximum euler angles, in radians.
         */
        void SetEulerConstraints(const MCore::Vector3& minVals, const MCore::Vector3& maxVals);

        /**
         * Set the constraint type. This allows you to disable constraints, or enable them, and choose the
         * method of constraining. Available methods are using ellipse and rectangle.
         * The rectangle method is the faster than the ellipse method, but produces less desirable results.
         * For some types of constraints however you'd probably like to use the rectangle, especially when you
         * want to disable the rotation on a specific axis.
         * On default the constraints are disabled.
         * @param constraintType The constraint type to use.
         */
        void SetConstraintType(EConstraintType constraintType);

        /**
         * Get the constraint type that is currently used.
         * On default the constraints are disabled.
         * @result The constraint type.
         */
        EConstraintType GetConstraintType() const;

        /**
         * Enable or disable EMotion FX 2 compatibility mode for this lookat controller.
         * On default this option is disabled. There is a small difference in the setup of the lookat controller
         * between EMotion FX 2 and 3. But since we don't want to break any code, or at least don't want you to change your
         * existing lookat-controller setup, we added this feature, so that you can enable this when you use the
         * old lookat controller code.
         * @param enabled Set to true when you want to enable. On default the EMotion FX 2 compatibility mode is disabled.
         */
        void SetEMFX2CompatbilityModeEnabled(bool enabled);

        /**
         * Check if the EMotion FX 2 lookat setup code compatibility is enabled.
         * On default this option is disabled. There is a small difference in the setup of the lookat controller
         * between EMotion FX 2 and 3. But since we don't want to break any code, or at least don't want you to change your
         * existing lookat-controller setup, we added this feature, so that you can enable this when you use the
         * old lookat controller code.
         * @result Returns true when this is enabled, and false when it is not.
         */
        bool GetIsEMFX2CompatbilityModeEnabled() const;

    private:
        MCore::Matrix       mPreRotation;           /**< This is used basically to tell the controller which vector to use as the up-vector. **/
        MCore::Matrix       mPostRotation;          /**< Used to make sure the node 'looks' along the new forward vector. **/
        MCore::Matrix       mEllipseOrientation;    /**< The orientation of the ellipse. */
        MCore::Quaternion   mRotationQuat;          /**< The current orientation of the node. **/
        MCore::Vector3      mGoal;                  /**< The goal position in case there is no target node set. */
        AZ::Vector2         mEllipseRadii;          /**< The radius values for the ellipse, that specify its dimensions. */
        AZ::Vector2         mMinMaxTwist;           /**< The twist rotation limits. */
        float               mInterpolationSpeed;    /**< The speed factor of the interpolation (default = 1.0f). */
        uint32              mNodeIndex;             /**< The node to work on. */
        EConstraintType     mConstraintType;        /**< Which type of constraint to use. */
        bool                mEMFX2CompatibleMode;   /**< True when EMotion FX 2 look-at controller compatibility mode has been enabled (default=false). */

        /**
         * The constructor.
         * @param actorInstance The actor instance on which this controller will work.
         * @param nodeIndex The node number to apply the controller on.
         */
        LookAtController(ActorInstance* actorInstance, uint32 nodeIndex);

        /**
         * The destructor.
         */
        ~LookAtController();
    };
} // namespace EMotionFX
