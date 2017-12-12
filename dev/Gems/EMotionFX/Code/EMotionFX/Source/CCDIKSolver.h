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
#include <MCore/Source/Vector.h>
#include "GlobalSpaceController.h"


namespace EMotionFX
{
    /**
     * The Cyclic Coordinate Descent (CCD) Inverse Kinematics (IK) solver.
     * This is a global space controller that modifies the global space transformation matrices as post process.
     * It supports joint constraints as well. It is a numerical iterative solver that can work on any size of chains.
     * Because it is quite fast, it is quite popular in games. The disadvantage of this solver is that it can suddenly
     * come up with a completely different solution in the next frame, leading to some popping/non-smooth artifacts.
     */
    class EMFX_API CCDIKSolver
        : public GlobalSpaceController
    {
    public:
        /**
         * The controller type ID as returned by GetType().
         */
        enum
        {
            TYPE_ID = 0x00010002
        };

        /**
         * The creation method.
         * @param actorInstance The actor instance where this controller will work on.
         * @param startNodeIndex The index of the start node. For example the left upper arm's node index.
         * @param endNodeIndex The index of the end node. For example the left hand's node index.
         */
        static CCDIKSolver* Create(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 endNodeIndex);

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
         * Set the goal position, in global space.
         * This is the point the IK solver will try to reach.
         * @param goal The goal position in global space.
         */
        void SetGoal(const MCore::Vector3& goal);

        /**
         * Get the goal position, in global space.
         * @result The goal position, in global space coordinates.
         */
        const MCore::Vector3& GetGoal() const;

        /**
         * Check if we have found a solution or not.
         * @result Returns true when the last called Update has found a solution. Otherwise false is returned.
         */
        bool GetHasFoundSolution() const;

        /**
         * Set the maximum number of iterations.
         * On default this is set to 10, which is quite high.
         * Reducing this number will result in more unstable solutions that can change rapidly.
         * Lower numbers however can result in much faster processing of this controller, especially in cases
         * where no solutions can be found (worst case scenario of this controller).
         * @param numIterations The maximum number of iterations of the CCD algorithm.
         */
        void SetMaxIterations(uint32 numIterations);

        /**
         * Get the maximum number of iterations of the CCD algorithm.
         * On default this is set to 10, which is quite high.
         * Reducing this number will result in more unstable solutions that can change rapidly.
         * Lower numbers however can result in much faster processing of this controller, especially in cases
         * where no solutions can be found (worst case scenario of this controller).
         * @result The maximum number of iterations of the CCD processing loop.
         */
        uint32 GetMaxIterations() const;

        /**
         * Set the distance threshold.
         * If the end node comes within the distance range of the specified goal
         * the controller will stop its iterations and will consider the solution solved.
         * @param distThreshold The distance of the maximum distance between the goal and the end node, before we consider the solution solved.
         */
        void SetDistThreshold(float distThreshold);

        /**
         * Get the distance threshold.
         * If the end node comes within the distance range of the specified goal
         * the controller will stop it's iterations and will consider the solution solved.
         * @result The distance of the maximum distance between goal and end node, before we consider the solution solved.
         */
        float GetDistThreshold() const;

        /**
         * Enable or disable joint limits. On default joint limits are enabled.
         * @param doJointLimits Set to true when you like to enable to limit the rotations of the joints (nodes).
         */
        void SetDoJointLimits(bool doJointLimits);

        /**
         * Check if the processing of joint limits is enabled or not.
         * @result Returns true when joint limits are enabled, otherwise false is returned.
         */
        bool GetDoJointLimits() const;


    private:
        MCore::Vector3  mGoal;              /**< The goal position, in global space. */
        uint32          mStartNodeIndex;    /**< The start node, for example the upper arm. */
        uint32          mEndNodeIndex;      /**< The end node, for example the hand (the end effector). */
        uint32          mMaxIterations;     /**< The maximum number of iterations (tradeoff between speed and accuracy) [default value=10]. */
        float           mDistThreshold;     /**< The maximum squared distance between goal and end effector that makes the solution solved. */
        bool            mHasSolution;       /**< Did we find a solution? */
        bool            mDoJointLimits;     /**< True if you want to impose joint limits, false otherwise. [default=true]. */

        /**
         * Calculate the Error Measure.
         * @param curPos The current position of the endpoint.
         * @result The error measure.
         */
        float ErrorMeasure(MCore::Vector3& curPos) const;

        /**
         * Calculate the rotation matrix for a ball-and-socket joint.
         * @param curEnd The location of the end effector.
         * @param rootPos The location of the joint.
         * @result The rotation matrix corresponding to a rotation from the rootPos to the curEnd.
         */
        MCore::Matrix RotMatBallAndSocket(MCore::Vector3& curEnd, MCore::Vector3& rootPos);

        /**
         * The constructor.
         * @param actorInstance The actor instance where this controller will work on.
         * @param startNodeIndex The index of the start node. For example the left upper arm's node index.
         * @param endNodeIndex The index of the end node. For example the left hand's node index.
         */
        CCDIKSolver(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 endNodeIndex);

        /**
         * The destructor.
         */
        ~CCDIKSolver();
    };
} // namespace EMotionFX
