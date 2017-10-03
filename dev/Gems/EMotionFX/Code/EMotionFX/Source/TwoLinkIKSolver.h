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

#include "EMotionFXConfig.h"
#include "GlobalSpaceController.h"
#include <MCore/Source/Vector.h>
#include <MCore/Source/Matrix4.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Actor;
    class Node;

    /**
     * The two link (bones) Inverse Kinematics (IK) solver.
     * This can be used to quickly calculate an IK solution for systems involving two bones, such as limbs.
     * The following ascii 'art' will show an example:
     *
     * <pre>
     *      B
     *     / \
     *    /   C
     *   A   (goal)
     *
     * where:
     * A = start node
     * B = mid node
     * C = end node (end effector)
     * (goal) = The goal we try to reach.
     * </pre>
     *
     * Given the global space positions of the nodes A, B and C we can calculate their lenghts.
     * The problem is now defined as: "What is the position of B so that C touches the goal, while we remain
     * the lenghts of the bones unchanged?".
     *
     * The implementation of this controller uses the solution described by Ken Perlin.
     *
     * <pre>
     * <b>Description by Ken Perlin:</b>
     * Given a two link joint from [0,0,0] to end effector position P,
     * let link lengths be a and b, and let norm |P| = c.  Clearly a+b >= c.
     *
     * Problem: find a "knee" position Q such that |Q| = a and |P-Q| = b.
     *
     * In the case of a point on the x axis R = [c,0,0], there is a
     * closed form solution S = [d,e,0], where |S| = a and |R-S| = b:
     *
     *    d2+e2 = a2                  -- because |S| = a
     *    (c-d)2+e2 = b2              -- because |R-S| = b
     *
     *    c2-2cd+d2+e2 = b2           -- combine the two equations
     *    c2-2cd = b2 - a2
     *    c-2d = (b2-a2)/c
     *    d - c/2 = (a2-b2)/c / 2
     *
     *    d = (c + (a2-b2/c) / 2      -- to solve for d and e.
     *    e = Sqrt(a2-d2)
     *
     * This leads to a solution to the more general problem:
     *
     *   (1) R = Mfwd(P)         -- rotate P onto the x axis
     *   (2) Solve for S
     *   (3) Q = Minv(S)         -- rotate back again
     *
     * If "knee" position Q needs to be as close as possible to some point D,
     * then choose M such that M(D) is in the y>0 half of the z=0 plane.
     * </pre>
     *
     * After you have created the controller you also have to set the desired bend direction.
     * This bend direction is a unit vector that points into the direction (in globalspace) of where the 'elbow' or 'knee'
     * should point inside the solution. An example would be if you put your fists on top of eachother and keep your arms stretched
     * pointing forward, away from you. Now if you move your fists closer to your chest, you see that the elbow of your right arm
     * points to the right (and maybe a bit downwards) and your left elbow points to the left. These directions are the bend directions
     * that you specify to the controller. Please note that these are in globalspace. This means that if you rotate your character
     * you also have to rotate the direction vectors with the same rotation as you rotate the character.
     */
    class EMFX_API TwoLinkIKSolver
        : public GlobalSpaceController
    {
    public:
        /**
         * The controller type ID as returned by GetType().
         */
        enum
        {
            TYPE_ID = 0x10123001
        };

        /**
         * The creation method.
         * @param actorInstance The actor instance to apply the inverse kinematics on.
         * @param endNodeIndex The end effector node, for example the hand.
         */
        static TwoLinkIKSolver* Create(ActorInstance* actorInstance, uint32 endNodeIndex);

        /**
         * The extended creation method.
         * @param actorInstance The actor instance to apply the inverse kinematics on.
         * @param startNodeIndex The start node index, for example the upper arm (or shoulder).
         * @param midNodeIndex The mid node index, for example the lower arm (or elbow).
         * @param endNodeIndex The end effector node index, for example the hand.
         */
        static TwoLinkIKSolver* Create(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex);

        /**
         * The main update method, which performs the modifications and calculations of the new global space matrices.
         * @param outPose The global space output pose where you have to output your calculated global space matrices in.
         *                This output pose is also the input. It will contain the current set of global space matrices for all nodes.
         * @param timePassedInSeconds The time passed, in seconds, since the last update.
         */
        void Update(GlobalPose* outPose, float timePassedInSeconds) override;

        /**
         * Get the goal node, from which the global space position will be the target goal position.
         * When this goal node index is set to MCORE_INVALIDINDEX32, this means no goal node is being used.
         * In that case the solver will use the specified goal position that you can specify using SetGoal and retrieve with
         * the GetGoal method.
         * This makes it possible to for example hold a gun with two arms, by making IK anchor nodes on the gun.
         * @result The index of the node that will act as IK goal, or MCORE_INVALIDINDEX32 when the goal node isn't used.
         */
        uint32 GetGoalNodeIndex() const;

        /**
         * Check if the controller is using the global space position of another node as goal, or if it is using
         * a manually specified goal position. You can get the goal node index with the GetGoalNodeIndex() method.
         * @result Returns true when it is using another node's global space position, and otherwise false is returned.
         * @see GetGoalNodeIndex.
         * @see SetGoalNodeIndex.
         */
        bool GetIsUsingGoalNode() const;

        /**
         * Set the goal node index from which the global space position will be the target goal position.
         * When this goal node index is set to MCORE_INVALIDINDEX32, this means no goal node is being used.
         * In that case the solver will use the specified goal position that you can specify using SetGoal and retrieve with
         * the GetGoal method.
         * If you do not want to specify a goal node, but want to specify a 3D position (MCore::Vector3) instead,
         * you have to set the goal node index to MCORE_INVALIDINDEX32 or use the DisableGoalNode method.
         * On default no goal node is used (after construction of the controller).
         * @param goalNodeIndex The index of the goal node, or MCORE_INVALIDINDEX32 when you want to use the SetGoal and GetGoal methods.
         */
        void SetGoalNodeIndex(uint32 goalNodeIndex);

        /**
         * Disable using a goal node, and use a manually specified goal position instead.
         * You can set this goal with the SetGoal() method.
         * On default using the goal node is already disabled.
         */
        void DisableGoalNode();

        /**
         * Set the goal position, in global space.
         * This is the point the IK solver will try to reach.
         * Please note that this goal position is only used when the goal node is set to nullptr.
         * You can do this with the SetGoalNode method.
         * @param goal The goal position in global space.
         */
        void SetGoal(const MCore::Vector3& goal);

        /**
         * Get the goal position, in global space.
         * @result The goal position, in global space coordinates.
         */
        const MCore::Vector3& GetGoal() const;

        /**
         * Get the bend direction in global space.
         * This bend direction is a vector that points into the direction where the midNode joint
         * should point towards in the solution. If the character is facing the camera which is at the origin and
         * looking forward into depth, the bend direction for the knee of a human character would point towards
         * the camera, so would be (0, 0, -1) in this case.
         * Since the bend direction is in global space, you will have to rotate it with the characters globalspace rotation.
         * @result The bend direction of the 'elbow' or 'knee'.
         */
        const MCore::Vector3& GetBendDirection() const;

        /**
         * Set the bend direction in global space.
         * The bend direction must be a unit vector. So a vector of a length of 1 (normalized vector).
         * This bend direction is a vector that points into the direction where the midNode joint
         * should point towards in the solution. If the character is facing the camera which is at the origin and
         * looking forward into depth, the bend direction for the knee of a human character would point towards
         * the camera, so would be (0, 0, -1) in this case.
         * Since the bend direction is in global space, you will have to rotate it with the characters globalspace rotation.
         * @param bendDir The direction vector where the knee or elbow should point to. This vector must be normalized.
         */
        void SetBendDirection(const MCore::Vector3& bendDir);

        /**
         * Set the bend direction relative to the position and orientation of a node.
         * The bend direction must be a unit vector.
         * If the bendDirNodeIndex is set to nullptr, the base of the ActorInstance is used.
         * @param bendDir The direction vector, relative to the node, where the knee or elbow points. This vector must be normalized.
         * @param bendDirNodeIndex The node from which the bend direction is defined.
         */
        void SetRelativeBendDir(const MCore::Vector3& bendDir, uint32 bendDirNodeIndex);

        /**
         * Set the bend direction relative to the position and orientation of a node.
         * The bend direction must be a unit vector.
         * @param bendDir The direction vector, relative to the node, where the knee or elbow points. This vector must be normalized.
         */
        void SetRelativeBendDir(const MCore::Vector3& bendDir);

        /**
         * Returns the bend direction vector.
         */
        const MCore::Vector3& GetRelativeBendDir() const;

        /**
         * Set the node from which the relative bend direction is defined.
         * If the bendDirNodeIndex is set to nullptr, the base of the ActorInstance is used.
         * @param bendDirNodeIndex The node from which the bend direction is defined.
         */
        void SetRelativeBendDirNode(uint32 bendDirNodeIndex);

        /**
         * Returns the index of the node from which the bend direction is defined.
         * If this is MCORE_INVALIDINDEX32, the bend direction is defined from the base of the actorInstance.
         * @result Returns the index of the node to relatively bend to.
         */
        uint32 GetRelativeBendDirNode() const;

        /**
         * Enable or disable the bend direction to be relative to a node.
         * By default, this is disabled, and the bend direction is specified in global space.
         * @param doRelBendDir Set true to set the bend direction to be relative to a node.
         */
        void SetUseRelativeBendDir(bool doRelBendDir);

        /**
         * Check if the bend direction is specified relative to a node or not.
         * @result Returns true when the bend direction is specified relative to a node, otherwise false is returned.
         */
        bool GetUseRelativeBendDir() const;

        /**
         * Get the unique type ID of this controller.
         * @result The unique ID.
         */
        uint32 GetType() const override;

        /**
         * Get the type string, containing the class name of this controller.
         * @result A pointer to the string containing the class name.
         */
        const char* GetTypeString() const override;

        /**
         * Check if we have found a solution or not.
         * @result Returns true when the last called Update has found a solution. Otherwise false is returned.
         */
        bool GetHasFoundSolution() const;

        /**
         * Solve the two bone IK problem.
         * This function works with positions only, and does not solve the rotations of nodes.
         * Actually this function has nothing to do with nodes. It is however the core of the algorithm.
         * Please keep in mind that all parameters should be in the same space, for example global space.
         * The output of this method is a vector that contains the position of the middle joint.
         * If you draw a line between the specified posA and this output middle joint pos and a line starting at
         * the output middle pos in direction of the goal, with a length of (posC - posB).Length() you are drawing
         * the solution of this solver.
         * @param posA The position of the first joint (for example the upper arm / shoulder).
         * @param posB The position of the middle joint (for example the elbow / lower arm).
         * @param posC The position of the end effector (for example the hand / wrist).
         * @param goal The goal the solver should try to reach.
         * @param bendDir The desired middle joint bend direction (as unit vector). So the direction the knee or elbow should point to.
         * @param outMidPos A pointer to the vector to which the function will write the new middle joint position.
         * @result Returns true when a solution has been found, or false when the goal is not reachable.
         */
        static bool Solve2LinkIK(const MCore::Vector3& posA, const MCore::Vector3& posB, const MCore::Vector3& posC, const MCore::Vector3& goal, const MCore::Vector3& bendDir, MCore::Vector3* outMidPos);

        /**
         * Clone the global space contoller.
         * This creates and returns an exact copy of this controller.
         * Be sure to also use the CopyBaseClassWeightSettings member to give the clone the same weight settings as this controller.
         * @param targetActorInstance The actor instance on which you want to use the cloned controller that will be returned.
         * @result A pointer to a clone of the current controller.
         */
        GlobalSpaceController* Clone(ActorInstance* targetActorInstance) override;

        /**
         * Get the start node index.
         * This is for example the upper arm (or shoulder) node.
         * @result The start node's index.
         */
        uint32 GetStartNodeIndex() const;

        /**
         * Get the mid node index.
         * This is for example the lower arm (or elbow) node.
         * @result The mid node's index.
         */
        uint32 GetMidNodeIndex() const;

        /**
         * Get the end effector node index.
         * This is for example the hand node.
         * @result The end effector node index.
         */
        uint32 GetEndNodeIndex() const;

    protected:
        MCore::Vector3  mGoal;              /**< The goal in globalspace. */
        MCore::Vector3  mBendDirection;     /**< The prefered bend direction, in globalspace. */
        MCore::Vector3  mRelativeBendDir;   /**< The bend direction relative to a node */
        uint32          mStartNodeIndex;    /**< The start node's index, for example the index of the upper arm node. */
        uint32          mMidNodeIndex;      /**< The mid node's index, for example the index of the lower arm node. */
        uint32          mEndNodeIndex;      /**< The end node's index, which is the end effector, for example the index of the hand node. */
        uint32          mGoalNodeIndex;     /**< The goal node's index, which is set to -1 on default. */
        uint32          mBendDirNodeIndex;  /**< The node from which the relative bend direction is defined. */
        bool            mHasSolution;       /**< Have we found a solution? */
        bool            mUseRelativeBendDir;/**< True if the bend direction is specified relative to a node, false otherwise */

        /**
         * The constructor.
         * @param actorInstance The actor instance to apply the inverse kinematics on.
         * @param endNodeIndex The end effector node, for example the hand.
         */
        TwoLinkIKSolver(ActorInstance* actorInstance, uint32 endNodeIndex);

        /**
         * The constructor.
         * @param actorInstance The actor instance to apply the inverse kinematics on.
         * @param startNodeIndex The start node index, for example the upper arm (or shoulder).
         * @param midNodeIndex The mid node index, for example the lower arm (or elbow).
         * @param endNodeIndex The end effector node index, for example the hand.
         */
        TwoLinkIKSolver(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex);

        /**
         * Calculates the matrix that rotates the object from solve space back into the original space.
         * The inverse (which equals the transpose) of this matrix is used to convert the specified parameters into solve space.
         * @param goal The goal we try to reach.
         * @param bendDir The desired middle joint bend direction (as unit vector). So the direction the knee or elbow should point to.
         * @param outForward The output matrix which will contain the matrix that rotates from solve space into the space of the goal.
         */
        static void CalculateMatrix(const MCore::Vector3& goal, const MCore::Vector3& bendDir, MCore::Matrix* outForward);
    };
} // namespace EMotionFX
