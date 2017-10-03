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

// include required EMotion FX headers
#include "EMotionFXConfig.h"
#include "TwoLinkIKSolver.h"
#include "BaseObject.h"


namespace EMotionFX
{
    /**
     * The foot plant inverse kinematics (IK) solver.
     */
    class EMFX_API FootPlantIKSolver
        : public TwoLinkIKSolver
    {
    public:
        /**
         * The controller type ID as returned by GetType().
         */
        enum
        {
            TYPE_ID = 0x10123002
        };

        /**
         * Available working modes for the foot plant controller.
         */
        enum EFootBehaviour
        {
            FB_NO_CORRECTION,       /**< No foot tweaking after leg conformation. */
            FB_KEEP_ORIENTATION,    /**< Preserve previous foot orientation. */
            FB_GROUND_ALIGN         /**< Align foot with ground, at rest (default mode). */
        };

        /*
         * Needed by the foot plant solver for interactive ray casting and to inform the foot plant
         * controller about the ground. The functions inside the callback will be used by the foot plant IK
         * controller.
         */
        class Callback
            : public BaseObject
        {
        public:
            /**
            * Before foot planting can happen we need to adjust the ground altitude of the character. Because the animation system is able to
            * move the character in global (depending on the animation data currently evaluated), we need the actor to have a correct altitude (planted on scenery).
            * This callback function will be called automatically from the foot plant IK solver at the very beginning of the update,
            * to allow client engine to perform the necessary computations.
            * @param actorInstance The actor instance to which this callback belongs to.
            * @param footPlantIKSolver The foot plant IK solver which called the function.
            */
            virtual void AdjustCharacterAltitude(ActorInstance* actorInstance, FootPlantIKSolver* footPlantIKSolver) = 0;

            /**
             * This is some interface for our solver to get the point the foot needs to be (usually using client engine scenery raytracing system).
             * @param rayStart The starting point of the casted ray.
             * @param direction The normalized direction vector.
             * @param outHitPosition A pointer to a vector that will contain the hit position if there is a hit.
             * @param outHitNormal A pointer to vector that will contain the normal at the hit position's location. The hit normal is used for foot alignment on ground.
             * @return False is returned when there is no hit detected. The parameters outHitPosition and outHitNormal will not be modified in this case. When a hit occurred, true is returned.
             */
            virtual bool GetHitPoint(const MCore::Vector3& rayStart, const MCore::Vector3& direction, MCore::Vector3* outHitPosition, MCore::Vector3* outHitNormal) = 0;

            /**
             * This function is called after we updated the global space matrices.
             * Sometimes twist bones act like a third leg, instead of being parented to for example the upper leg. So the IK algorithm wouldn't update this twist bone, which then results in
             * bad skinning of the mesh. Inside this function you could copy the global space transform of say the upper leg to the related twist bone, to rotate the twist bone with the leg.
             * @param globalMatrices The input and output global space matrices. Use the node index values to index inside the array.
             */
            virtual void UpdateTwistBones(MCore::Matrix* globalMatrices) { MCORE_UNUSED(globalMatrices); }

        protected:
            /**
             * Constructor.
             */
            Callback() {}

            /**
             * Destructor.
             */
            virtual ~Callback() {}
        };

        /**
         * The creation method.
         * @param actorInstance The actor instance to apply the foot plant on.
         * @param startNodeIndex The start node index, for example the upper foot.
         * @param midNodeIndex The mid node index, for example the lower foot.
         * @param endNodeIndex The end effector node index, for example the shoe.
         * @param callback A customized callback used for environment interaction. This callback calculates the height of the ground.
         */
        static FootPlantIKSolver* Create(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex, Callback* callback);

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
         * Clone the foot plant IK controller.
         * This creates and returns an exact copy of this controller.
         * @param targetActorInstance The actor instance on which you want to use the cloned controller that will be returned.
         * @result A pointer to a clone of the current controller.
         */
        GlobalSpaceController* Clone(ActorInstance* targetActorInstance) override;

        /**
         * The main update method, which performs the modifications and calculations of the new global space matrices.
         * @param outPose The global space output pose where you have to output your calculated global space matrices in.
         *                This output pose is also the input. It will contain the current set of global space matrices for all nodes.
         * @param timePassedInSeconds The time passed, in seconds, since the last update.
         */
        void Update(GlobalPose* outPose, float timePassedInSeconds) override;

        /**
         * Set the distance between the pelvis and the ground surface.
         * This should be called each time the pelvis is tweaked by the the client application.
         * @param altitude The distance from ground.
         */
        void SetMainGroundAltitude(float altitude);

        /**
         * A positive value, which prevents the solver from going beyond some limit.
         * @param pelvisIndex The node index of the pelvis node inside the skeleton.
         * @param minimumDistance The minimum distance between pelvis and foot.
         */
        void SetPelvisMinimumDistance(uint32 pelvisIndex, float minimumDistance);

        /**
         * When altitude is abruptly changing, this allows some interpolation control to avoid sudden
         * changes in foot correction: two sets of interpolation params (for slow/fast response), parameterized with 'stiffness' tweaked upong game context.
         * @param adaptSpeedLow Speed at which modification occurs. Very high speeds produce instant foot repositioning.
         * @param adaptThresholdLow Distance above which adaptation clamping occurs: above that value, the foot will always reach its goal in constant time (distance/speed).
         * @param adaptSpeedHigh See adaptSpeedLow.
         * @param adaptThresholdHigh See adaptThresholdHigh.
         */
        void SetAdaptationSpeeds(float adaptSpeedLow, float adaptThresholdLow, float adaptSpeedHigh, float adaptThresholdHigh);

        /**
         * Set the stiffness of the solver.
         * Stiffness is the interpolation ratio between two sets of adaptation parameters (usually between a soft and a fast response sets).
         * This value must be in range of 0 to 1, where 1 means stiff, and 0 means soft. The default value is 0.25.
         * @param stiffness The stiffness ratio parameter.
         */
        void SetStiffness(float stiffness);

        /**
         * This will define how the foot should be handled after foot planting.
         * @param footBehavior Define how foot bone is going to be tweaked or aligned with underlying ground.
         */
        void SetFootBehavior(EFootBehaviour footBehavior);

        /**
         * This defines the foot tweaking range.
         * Only useful when the foot behavior mode is set to FB_GROUND_ALIGN.
         * @param lowPitch Defines the range of possible foot alignment with ground.
         * @param highPitch Defines the range of possible foot alignment with ground.
         */
        void SetFootPitchLimits(float lowPitch, float highPitch);

        /**
         * Return the callback used for environment interaction.
         * @return A pointer to the callback object.
         */
        Callback* GetCallback() const;

        /**
         * Check whether custom bend direction mode is enabled.
         * The controller automatically calculates and sets the bend direction. In some cases you might want to disable this and provide your own bend direction vector.
         * If that is the case, you have to enable the custom bend direction mode using SetCustomBendDirEnabled() function.
         * On default custom bend direction mode is disabled, so on default this function will return false, unless the user explicitly enabled custom bend direction mode.
         * @result Returns true when custom bend direction mode is enabled, otherwise returns false.
         */
        bool GetCustomBendDirEnabled() const;

        /*
         * Enable or disable custom bend direction mode.
         * The controller automatically calculates and sets the bend direction. In some cases you might want to disable this and provide your own bend direction vector.
         * On default custom bend direction mode is disabled.
         * Please note that this operation is quite heavy internally, as it needs to precalculate the global space transforms of the bind pose if the mode changes.
         * Also keep in mind that relative bend direction mode is enabled on default, or when you disable the custom bend direction mode.
         * You can disable relative bend direction mode using SetUseRelativeBendDir().
         * @param enabled Set to true to enable custom bend direction mode, or false to disable it.
         */
        void SetCustomBendDirEnabled(bool enabled);


    private:
        /*
         * Some special interpolator designed to ensure speed continuity.
         * used for abrupt planting goals changes (like on stairs).
         */
        class SmoothValueInterpolator
        {
        public:
            /**
             * The default constructor.
             */
            SmoothValueInterpolator();

            /**
             * The extended constructor.
             * @param defaultValue The very first (returned) value.
             */
            SmoothValueInterpolator(float defaultValue);

            /**
             * Pushing here the next value to interpolate to.
             * @param value The Value to reach.
             * @param speed The Speed to reach.
             * @param duration The interpolation duration.
             */
            void SetNextValue(float value, float speed, float duration);

            /**
             * Update interpolator: compute current speed and value
             * @param elapsedTime The time delta.
             * @return The current Value (result of the interpolation).
             */
            float Update(float elapsedTime);

            /**
             * Returns an instant interpolated Value.
             * @return The result of the smooth interpolation.
             */
            MCORE_INLINE float GetCurrentValue() const          { return mCurrentValue; }

            /**
             * Returns the current speed, result of the interpolation.
             * @return Returns the current speed.
             */
            MCORE_INLINE float GetCurrentSpeed() const          { return mCurrentSpeed; }

        private:
            float mValues[2];           /**< The interpolated 'previous' and 'next' values. */
            float mSpeeds[2];           /**< The interpolated 'previous' and 'next' speeds. */
            float mTimes[2];            /**< The 'previous' and 'next' times. */
            float mCurrentTime;         /**< The 'current' time. Refreshed after each Update(). */
            float mCurrentValue;        /**< The 'current' value. Refreshed after each Update(). */
            float mCurrentSpeed;        /**< The 'current' speed. Refreshed after each Update(). */

            static float HermiteBasis[4][4];
            static float Hermite1stDerivBasis[3][4];
            static float Hermite2ndDerivBasis[2][4];

            void ComputeTimeBasis(float basis0[4][4], float t, float B[]);
            void Compute1stDerivTimeBasis(float basis1[4][4], float t, float B[]);
            float EvaluateP(float t, float basis[4][4], float P0, float T0, float P1, float T1);
            float EvaluateT(float t, float basis[3][4], float P0, float T0, float P1, float T1);
            float HermiteCubicInterpolate(float t, float x1, float v1, float x2, float v2, float* pv);
        };

        MCore::Matrix           mFootDelta;             /**< Internally used to store foot information before final foot tweaking. */
        MCore::Matrix           mFootBindpose;          /**< Internal use. */
        SmoothValueInterpolator mAltitude;              /**< Will smoothly interpolate the correction to apply. */
        Callback*               mCallback;              /**< Used for interactive hit detection. */

        float                   mLastHitAltitude;       /**< For the record, to detect altitude changes. */
        float                   mCurrentDelta;          /**< The altitude we moved 'foot' to. */
        float                   mMinimumPelvisDistance; /**< Some distance to limit correction. */
        float                   mGroundAltitude;        /**< The ground altitude used to position the actor, as a reference. */
        float                   mAdaptSpeedLow;         /**< Adaptivity parameters, controlled by stiffness parameter. */
        float                   mAdaptThresholdLow;     /**< Adaptivity parameters: when clamping occurs. */
        float                   mAdaptSpeedHigh;        /**< Adaptivity parameters, controlled by stiffness parameter. */
        float                   mAdaptThresholdHigh;    /**< Adaptivity parameters: when clamping occurs. */
        float                   mStiffness;             /**< Interpolation ratio between the two sets of planting parameters */
        float                   mFootPitchLow;          /**< If FB_GROUND_ALIGN, tell us how far we can pitch foot. */
        float                   mFootPitchHigh;         /**< If FB_GROUND_ALIGN, tell us how far we can pitch foot. */

        uint32                  mFootParentIndex;       /**< Needed for final foot tweaking. */
        uint32                  mPelvisIndex;           /**< Relatively to pelvis. */
        EFootBehaviour          mFootBehavior;          /**< Foot tweaking parameters, after leg conformation. */
        bool                    mCustomBendDir;         /**< Use a custom bend direction? (default=false). */

        /**
         * Initialize the controller. The constructor automatically initializes the controller.
         * @param actorInstance The actor instance to apply the foot plant on.
         */
        void Initialize(ActorInstance* actorInstance);

        /**
         * The constructor.
         * @param actorInstance The actor instance to apply the foot plant on.
         * @param startNodeIndex The start node index, for example the upper foot.
         * @param midNodeIndex The mid node index, for example the lower foot.
         * @param endNodeIndex The end effector node index, for example the shoe.
         * @param callback A customized callback used for environment interaction. This callback calculates the height of the ground.
         */
        FootPlantIKSolver(ActorInstance* actorInstance, uint32 startNodeIndex, uint32 midNodeIndex, uint32 endNodeIndex, Callback* callback);

        /**
         * The destructor.
         */
        ~FootPlantIKSolver();
    };
} // namespace EMotionFX
