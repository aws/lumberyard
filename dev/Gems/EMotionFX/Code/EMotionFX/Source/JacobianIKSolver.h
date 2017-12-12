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

// include MCore related files
#include "EMotionFXConfig.h"
#include "LocalSpaceController.h"
#include "TransformData.h"

#include <MCore/Source/Vector.h>
#include <MCore/Source/NMatrix.h>
#include <MCore/Source/Matrix4.h>
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Actor;
    class Node;


    /**
     * The Selectively Damped Least Squares Jacobian Pseudo-Inverse Inverse Kinematics solver (tm) :).
     * This solver is the most advanced one of all the solvers included with EMotion FX.
     * It supports joint limits, multiple end effectors/chains to solve at the same time and it can
     * handle both positional and rotational goals.
     */
    class EMFX_API JacobianIKSolver
        : public LocalSpaceController
    {
    public:
        // the unique ID of this controller
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * The main creation method.
         * @param actorInstance The actor instance that this controller shall work on.
         */
        static JacobianIKSolver* Create(ActorInstance* actorInstance);

        /**
         * Add a chain of nodes from the actor to be solved. Each chain has an end effector linked with it.
         * @param startNode The start node, for example the upper arm node.
         * @param endNode The end effector node, for example the hand node.
         * @param doPos Set to true if you want to use a positional goal on this chain. Set this using the SetGoalPos() method.
         * @param doRot Set to true if you want to use a rotational goal on this chain. Set this using the SetGoalRot() method.
         * @result The chain index, or MCORE_INVALIDINDEX32 when both position and rotation goals are disabled.
         */
        uint32 AddChain(const Node* startNode, Node* endNode, bool doPos = true, bool doRot = false);

        /**
         * Initialize the solver.
         * This should be called after all the required chains have been added.
         * No chains should be added after this call.
         */
        void Initialize();

        /**
         * Get the unique type ID of this controller.
         * @result The unique ID.
         */
        MCORE_INLINE uint32 GetType() const;

        /**
         * Get the type string, containing the class name of this controller.
         * @result A pointer to the string containing the class name.
         */
        MCORE_INLINE const char* GetTypeString() const;

        /**
         * Set the goal position for a given chain / end effector, in global space.
         * This is the point the IK solver will try to reach for the given chain.
         * @param chainIndex The index of the chain/end effector to manipulate.
         * @param goalPos The goal position in global space.
         */
        MCORE_INLINE void SetGoalPos(uint32 chainIndex, const MCore::Vector3& goalPos);

        /**
         * Get the goal position, in global space.
         * @param chainIndex The index of the chain/end effector to get information from.
         * @result The goal position, in global space coordinates.
         */
        MCORE_INLINE const MCore::Vector3& GetGoalPos(uint32 chainIndex) const;

        /**
         * Set the goal rotation, in global space.
         * This is the rotation the IK solver will try to reach for the given chain / end effector.
         * @param chainIndex The index of the chain/end effector to manipulate.
         * @param goalOri The goal rotation, in global space (radian euler angles).
         */
        MCORE_INLINE void SetGoalRot(uint32 chainIndex, const MCore::Vector3& goalOri);

        /**
         * Get the goal orientation, in global space.
         * @param chainIndex The index of the chain/end effector to get information from.
         * @result The goal rotation, in global space euler angles.
         */
        MCORE_INLINE const MCore::Vector3& GetGoalRot(uint32 chainIndex) const;

        /**
         * Check if we have found a solution or not.
         * @result Returns true when the last called Update has found a solution. Otherwise false is returned.
         */
        MCORE_INLINE bool GetHasFoundSolution() const;

        /**
         * Set the maximum number of iterations.
         * On default this is set to 10, which is quite high.
         * Reducing this number will result in more unstable solutions that can change rapidly.
         * Lower numbers however can result in much faster processing of this controller, especially in cases
         * where no solutions can be found (worst case scenario of this controller).
         * Lower values also make the character move slower after the goals.
         * @param numIterations The maximum number of iterations of the Jacobian algorithm.
         */
        MCORE_INLINE void SetMaxIterations(uint32 numIterations);

        /**
         * Get the maximum number of iterations the solver will perform in an update.
         * Reducing this number will result in more unstable solutions that can change rapidly.
         * Lower numbers however can result in much faster processing of this controller, especially in cases
         * where no solutions can be found (worst case scenario of this controller).
         * @result The maximum number of iterations of the solver loop.
         */
        MCORE_INLINE uint32 GetMaxIterations() const;

        /**
         * Set the distance threshold.
         * If the end node comes within the squared distance range of the specified goal
         * the controller will stop its iterations and will consider the solution solved.
         * @param distThreshold The squared distance of the maximum distance between the goal and the end node, before we consider the solution solved.
         */
        MCORE_INLINE void SetDistThreshold(float distThreshold);

        /**
         * Get the distance threshold.
         * If the end node comes within the squared distance range of the specified goal
         * the controller will stop it's iterations and will consider the solution solved.
         * @result The squared distance of the maximum distance between goal and end node, before we consider the solution solved.
         */
        MCORE_INLINE float GetDistThreshold() const;

        /**
         * Enable or disable processing of joint limits.
         * @param doLimits The flag you want to set.
         */
        MCORE_INLINE void SetDoJointLimits(bool doLimits);

        /**
         * Check if the processing of joint limits is enabled or not.
         * @result Returns true when joint limits are enabled, otherwise false is returned.
         */
        MCORE_INLINE bool GetDoJointLimits() const;

        /**
         * Set the maximum goal distance. This is the maximum distance the goal
         * will be placed from the end effectors at each iteration step.
         * @param dist The distance you want to set.
         */
        MCORE_INLINE void SetMaxGoalDist(float dist);

        /**
         * Get the maximum distance the goal will be placed from the end effectors
         * at each itteration step.
         * @result The maximum goal distance.
         */
        MCORE_INLINE float GetMaxGoalDist() const;

        /**
         * Set whether or not the root can translate. The root can move around if this is true.
         * will be placed from the end effectors at each iteration step.
         * @param canTranslate Set to true when you wish the root nodes to be able to translate. Otherwise set to false.
         */
        MCORE_INLINE void SetRootCanTranslate(bool canTranslate);

        /**
         * Get whether or not the root can translate or not.
         * @result Returns true if the root node is allowed to translate, otherwise false is returned.
         */
        MCORE_INLINE bool GetRootCanTranslate() const;

        /**
         * Set whether or not the the solver should use the null space projector.
         * The null space projector can improve joint limit handling.
         * On default this is enabled.
         * @param useNSP Set to true when you want the solver to use the null space projector.
         */
        MCORE_INLINE void SetUseNSP(bool useNSP);

        /**
         * Get whether or not the solver is using the null space projector.
         * The null space projector can improve joint limit handling.
         * On default this is enabled.
         * @result Returns true when the solver will use the null space projector. Otherwise false is returned.
         */
        MCORE_INLINE bool GetUseNSP() const;

        /**
         * Set the positional offset for an end effector.
         * This is an offset added to the last node (end effector) in the chain you specified.
         * So instead of the end effector position trying to get at the same location as the specified goal, now
         * the end effector position + offset will be the point trying to get to the goal.
         * @param chainIndex The index of the chain/end effector to manipulate.
         * @param posOffset The positional offset in local space.
         */
        MCORE_INLINE void SetPosOffset(uint32 chainIndex, const MCore::Vector3& posOffset);

        /**
         * Get the positional offset for a given end effector.
         * This is an offset added to the last node (end effector) in the chain you specified.
         * So instead of the end effector position trying to get at the same location as the specified goal, now
         * the end effector position + offset will be the point trying to get to the goal.
         * @result Returns the positional offset.
         */
        MCORE_INLINE MCore::Vector3 GetPosOffset(uint32 chainIndex) const;

        /**
         * Get the number of successfully added chains.
         * This equals the number of end effectors as well.
         * @result The number of successfully added chains.
         */
        MCORE_INLINE uint32 GetNumChains() const                                { return mEndEffectors.GetLength(); }

        /**
         * Clone the local space controller.
         * When you write your own Clone implementation for your own controller, be sure to also call the
         * method named LocalSpaceController::CopyBaseControllerSettings.
         * @param targetActorInstance The actor instance where you will add the cloned local space controller to.
         * @param neverActivate When set to false, the controller activation status will be cloned as well. This means that the
         *                      controller can already be active as well after cloning. When set to true, the clone will never be active after cloning.
         * @result A pointer to an exact clone of the this local space controller.
         */
        LocalSpaceController* Clone(ActorInstance* targetActorInstance, bool neverActivate = false) override;

        /**
         * Create the motion links for this controller.
         * This specifies what nodes will be modified by this controller.
         * If this controller would only modify the arm of a human, only the nodes of the arm should have motion links created.
         * When your controller doesn't seem to have any effect, check if you actually setup the motion links properly or if you have
         * activated your controller using the Activate() method.
         * @param actor The actor from which the actor instance that we add this controller to has been created.
         * @param instance The motion instance that contains the playback settings. This contains also a start node that you can get with
         *                 the MotionInstance::GetStartNode() method. You might want to take this into account as well.
         */
        void CreateMotionLinks(Actor* actor, MotionInstance* instance) override;

        /**
         * The main update method.
         * This is where the calculations and modifications happen. So this is the method that will apply the
         * modifications to the local space transformations.
         * @param inPose The input local transformations for all nodes. This contains the current blended results.
         * @param outPose The local space transformation buffer in which you output your results. So do not modify the
         *                local space transforms inside the TransformData class, but output your transformations inside
         *                this Pose.
         * @param instance The motion instance object that contains the playback settings.
         */
        void Update(const Pose* inPose, Pose* outPose, MotionInstance* instance) override;


    private:
        /**
         * An end effector.
         * This is the last node in the chain. The position/rotation of this end effector is
         * going to be ultimately moved on top of the goal that we specify.
         */
        class EMFX_API EndEffector
        {
        public:
            /**
             * The constructor.
             * Doesn't use any goal offsets. And the goal is at the origin (0,0,0).
             */
            EndEffector()
            {
                mDoPos  = false;
                mDoOri  = false;
                mConstrainJoints = false;
                mPos.Set(0.0f, 0.0f, 0.0f);
                mGoalPos.Set(0.0f, 0.0f, 0.0f);
                mGoalOri.Set(0.0f, 0.0f, 0.0f);
                mPosOffset.Set(0.0f, 0.0f, 0.0f);
                mPosDeriv.Set(0.0f, 0.0f, 0.0f);
                mOriDeriv.Set(0.0f, 0.0f, 0.0f);
            }

            MCore::Vector3  mPos;                   /**< The position of the end Effector. This is updated in UpdateEndEffectors. */
            MCore::Vector3  mGoalPos;               /**< The positional goal of this end effector. */
            MCore::Vector3  mGoalOri;               /**< The orientational goal of this end efector. */
            MCore::Vector3  mPosOffset;             /**< The position offset is used to moved the end effector away from the nodes position. With out this the end effector wouldnt have any distance from the node to rotate properly. */
            MCore::Vector3  mPosDeriv;              /**< The vector from the end effectors position to its goal. */
            MCore::Vector3  mOriDeriv;              /**< Similar to mPosDeriv, but for orientation. */
            uint32          mArrayIndex;            /**< Index into the arrays for this end effector */
            uint32          mNodeIndex;             /**< The index of the node in the actor that this end effector coencides with. */
            bool            mDoPos;                 /**< The end effector controlls the position of the node if this is true. */
            bool            mDoOri;                 /**< The end effector controlls the orientation of the node if this is true. */
            bool            mConstrainJoints;       /**< The joints in the ik chain that this end effector controlls will be constrained if this is true. */
        };

        /**
         * A joint inside the IK chains.
         */
        struct EMFX_API Joint
        {
            MCore::Vector3          mMinLimits;     /**< The minimum limits of the joint. */
            MCore::Vector3          mMaxLimits;     /**< The maximum limits of the joint. */
            MCore::Array<uint32>    mEndEffectors;  /**< The array of end effectors that influence this joint. */
            MCore::Array<uint16>    mDOFs; /**< The Degree of Freedom array. Contains a list of the numerical indices of the DOFs this joint has. ie 0 = x, 1 = y, 2 = z. */            // TODO: make a byte array?
            uint32                  mJointState; /**< Flags for the state of each DOF of this joint, ie. 'free' (0) or 'locked' (1). */     // TODO: make a boolean?
            uint32                  mNodeIndex;     /**< The the index of the node in the actor that this joint represents. */
            uint32                  mJointIndex;    /**< The index inside the Joint State Array that this joint is located at. */
        };

        // friend so that it can use the private Joint class
        friend int32 MCORE_CDECL JointCmp(const JacobianIKSolver::Joint& itemA, const JacobianIKSolver::Joint& itemB); // this actually works! :D

        MCore::NMatrix              mJacobian;              /**< The Jacobian matrix. */
        MCore::NMatrix              mJacobianInv;           /**< The pseudo inverse of the Jacobian matrix. */
        MCore::NMatrix              mNullSpaceProjector;    /**< The Null Space Projection Matrix. */
        MCore::NMatrix              mTempMatrix;            /**< A temp matrix used to prevent some reallocs in certain operations. */
        MCore::Array<EndEffector>   mEndEffectors;          /**< The End Effector array contains information about each End Effector. See its listing for details on what it contains. */
        MCore::Array<uint32>        mPrerequisitNodes;      /**< The Prerequisit Node array contains a list of nodes whos globalspace information needs to be calculated before the solver can start. */
        MCore::Array<float>         mGradiantVector;        /**< The Gradient Vector contains joint angle derivatives for the secondary task. */
        MCore::Array<float>         mSigma;                 /**< The array of singular values returned by the Singular Value Decomposition of the Jacobian matrix. */
        MCore::Array<float>         mTempArray;             /**< The Temp Array to store the Jacobian matrix for Singular Value Decomposition. */
        MCore::Array<float>         mAI;                    /**< One of the arrays used by the SDLS algorithm. */
        MCore::Array<float>         mVI;                    /**< One of the arrays used by the SDLS algorithm. */
        MCore::Array<Joint>         mJoints;                /**< The Joint array contains information about each individual joint (Not DOF!). See its listing for details on what it contains. */
        MCore::NMatrix*             mV;                     /**< The V matrix is a result of the Singular Value Decomposition. */
        MCore::NMatrix*             mU;                     /**< The U matrix is a result of the Singular Value Decomposition. */
        MCore::Array<float>*        mJointState;            /**< The Joint State is an array containing the values for all the degrees of freedom that the solver has. */
        MCore::Array<float>*        mNewJointState;         /**< The updated joint state array. This is the result of solver before joint limits are checked. */
        TransformData*              mTFD;                   /**< The Transform Data of the Actor Instance. Stored for easy access. */
        MCore::Matrix*              mLocalMatrices;         /**< The Local Matrices of the Actor Instance. Stored for easy access. */
        MCore::Matrix*              mGlobalMatrices;            /**< The Global Matrices of the Actor Instance. Stored for easy access. */
        Joint                       mRootJoint;             /**< Information about the "root joint", eg. how the solver is allowed to move the root in globalspace. */
        float                       mMaxGoalDist;           /**< The maximum distance the goal will be placed from the end effector while solving. The goal is moves closer if its distance exceeds this value. */
        float                       mDistThreshold;         /**< The maximum squared distance between goal and end effector that makes the solution solved. */
        uint32                      mMaxIterations;         /**< The maximum number of iterations (tradeoff between speed and accuracy) [default value=10]. */
        bool                        mHasSolution;           /**< The solver has found a solution if this value is true once the solver has updated. */
        bool                        mConstrainJoints;       /**< True if you want to impose joint limits, false otherwise. [default=true]. */
        bool                        mRootCanTranslate;      /**< The root is allowed to traverse in globalspace if this is true. */
        bool                        mUseNSP;                /**< The solver will use the null space projector in the calculations if this is true. */
        bool                        mProcessRoot;           /**< The root node will be solved (seperately to the rest of the tree structure) and has been processed when this value true. */

        /**
         * The constructor.
         * @param actorInstance The Actor Instance that the Inverse Kinematics will be applied to.
         */
        JacobianIKSolver(ActorInstance* actorInstance);

        /**
         * The destructor.
         */
        ~JacobianIKSolver();

        /**
         * Update the global positions of all the end effectors.
         * @param inPose The input pose that contains the local space transforms.
         */
        void UpdateEndEffectors(const Pose* inPose);

        /**
         * Build the Jacobian Matrix from all the information about the joints/DOFs.
         */
        void BuildJacobianMatrix();

        /**
         * Print the Jacobian matrix to the log for debugging.
         */
        void PrintJacobian();

        /**
         * Process the root node and adds it to the solver for solving.
         * @param root The root node.
         */
        void ProcessRoot(Node* root);

        /**
         * Updates all the affected joints in the ActorInstance from the Joint State array.
         * @param inPose The input pose from the Update method.
         * @param outPose The pose which we update and output to.
         */
        void CopyStateToActor(const Pose* inPose, Pose* outPose);

        /**
         * Solves the system defined in the Jacobian matrix etc.
         */
        void Solve();

        /**
         * Check for DOFs that have gone outside of their limits.
         * The values are clamped and action is taken to improve the situation in the next iteration.
         */
        bool CheckJointLimits();

        /**
         * Check if a joint is already in the joint array and add the new End Effector index to the joint if it is found.
         * @param joint The joint to check.
         * @result Returns true if the joint already exists inside the joint array. Otherwise false is returned.
         */
        bool FindJoint(const Joint& joint);
    };

    // include the inline code
#include "JacobianIKSolver.inl"
} // namespace EMotionFX
