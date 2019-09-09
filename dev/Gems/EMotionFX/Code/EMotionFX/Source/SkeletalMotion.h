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

// include the required headers
#include <AzCore/std/parallel/shared_mutex.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Source/Motion.h>


namespace EMotionFX
{
    // forward declaration
    class Actor;
    class Transform;
    class MorphSubMotion;
    class SkeletalSubMotion;

    /**
     * The motion class.
     * A motion contains a set of skeletal sub-motions. So this forms a complete skeleton
     * animation. An example of a motion would be a walk animation. Every motion can be shared between
     * different actors. So multiple actors can use the same motion, but still with different playback settings.
     */
    class EMFX_API SkeletalMotion
        : public Motion
    {
        AZ_CLASS_ALLOCATOR(SkeletalMotion, MotionAllocator, 0)

    public:
        // The skeletal motion type ID, returned by GetType()
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * Creation method.
         * @param name The name/description of the motion.
         */
        static SkeletalMotion* Create(const char* name);

        /**
         * Find a sub-motion by its name.
         * @param name The case sensitive name.
         * @result A pointer to the sub-motion, or nullptr when not found.
         */
        SkeletalSubMotion* FindSubMotionByName(const char* name) const;

        /**
         * Find a sub-motion by its ID.
         * @param id The ID value to search for.
         * @result A pointer to the sub-motion, or nullptr when not found.
         */
        SkeletalSubMotion* FindSubMotionByID(uint32 id) const;

        /**
         * Find a sub-motion number by name.
         * @param name The case sensitive name.
         * @result The sub-motion index in range of [0..GetNumSubMotions()-1], or MCORE_INVALIDINDEX32 when not found.
         */
        uint32 FindSubMotionIndexByName(const char* name) const;

        /**
         * Find a sub-motion number by its ID.
         * @param id The ID to search for.
         * @result The sub-motion number in range of [0..GetNumSubMotions()-1], or MCORE_INVALIDINDEX32 when not found.
         */
        uint32 FindSubMotionIndexByID(uint32 id) const;

        /**
         * Returns the number of sub-motions inside this skeletal motion.
         * @result The number of sub-motions.
         */
        MCORE_INLINE uint32 GetNumSubMotions() const                                { return static_cast<uint32>(mSubMotions.size()); }

        /**
         * Get a given sub-motion.
         * @param nr The sub-motion number, which must be in range of [0..GetNumSubMotions()-1].
         * @result A pointer to the skeletal sub-motion.
         */
        MCORE_INLINE SkeletalSubMotion* GetSubMotion(uint32 nr) const               { return mSubMotions[nr]; }

        /**
         * Add a given skeletal sub-motion to this motion.
         * @param part The skeletal sub-motion to add.
         */
        void AddSubMotion(SkeletalSubMotion* part);

        /**
         * Set the value of a given submotion.
         * @param index The submotion number, which must be in range of [0..GetNumSubMotions()-1].
         * @param subMotion The submotion value to set at this index.
         */
        void SetSubMotion(uint32 index, SkeletalSubMotion* subMotion);

        /**
         * Set the number of submotions.
         * This resizes the array of pointers to submotions.
         * @param numSubMotions The number of submotions to allocate space for.
         * @result Returns false if the number of submotions couldnt be set (i.e. numSubMotions is too big)
         */
        bool SetNumSubMotions(uint32 numSubMotions);

        /**
         * Delete all sub-motions from memory and from the motion (this will clear the motion).
         */
        void RemoveAllSubMotions();

        /**
         * Calculates the node transformation of the given node for this motion.
         * @param instance The motion instance that contains the motion links to this motion.
         * @param outTransform The node transformation that will be the output of this function.
         * @param actor The actor to apply the motion to.
         * @param node The node to apply the motion to.
         * @param timeValue The time value.
         * @param enableRetargeting Set to true if you like to enable motion retargeting, otherwise set to false.
         */
        void CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting) override;

        /**
         * Returns the type identification number of the motion class.
         * @result The type identification number.
         */
        uint32 GetType() const override;

        /**
         * Gets the type as a description.
         * @result The string containing the type of the motion.
         */
        const char* GetTypeString() const override;

        /**
         * Creates the motion links inside a given actor.
         * So we know what nodes are effected by what motions, this allows faster updates.
         * @param actor The actor to create the links in.
         * @param instance The motion instance to use for this link.
         */
        void CreateMotionLinks(Actor* actor, MotionInstance* instance) override;

        /**
         * Update the maximum time of this animation. Normally called after loading a motion. The maximum time is
         * estimated by iterating through all sub-motions and locating the maximum keyframe time value.
         */
        void UpdateMaxTime() override;

        /**
         * Returns if this motion type should overwrite body parts that are not touched by a motion
         * when played in non-mixing mode. Skeletal motions would return true, because if you play a non-mixing
         * motion you want the complete body to go into the new motion.
         * However, facial motions only need to adjust the facial bones, so basically they are always played in
         * mixing mode. Still, the behaviour of mixing motions is different than motions that are played in non-mixing
         * mode. If you want to play the motion in non-mixing mode and don't want to reset the nodes that are not influenced
         * by this motion, the motion type should return false.
         * @result Returns true if the motion type resets nodes even when they are not touched by the motion, when playing in non-mix mode.
         *         Otherwise false is returned.
         */
        bool GetDoesOverwriteInNonMixMode() const override;

        /**
         * Check if this motion supports the CalcNodeTransform method.
         * This is for example used in the motion based actor repositioning code.
         * @result Returns true when the CalcNodeTransform method is supported, otherwise false is returned.
         */
        bool GetSupportsCalcNodeTransform() const override                              { return true; }

        /**
         * Make the motion loopable, by adding a new keyframe at the end of the keytracks.
         * This added keyframe will have the same value as the first keyframe.
         * @param fadeTime The relative offset after the last keyframe. If this value is 0.5, it means it will add
         *                 a keyframe half a second after the last keyframe currently in the keytrack.
         */
        void MakeLoopable(float fadeTime = 0.3f) override;

        /**
         * The main update method, which outputs the result for a given motion instance into a given output local pose.
         * @param inPose The current pose, as it is currently in the blending pipeline.
         * @param outPose The output pose, which this motion will modify and write its output into.
         * @param instance The motion instance to calculate the pose for.
         */
        void Update(const Pose* inPose, Pose* outPose, MotionInstance* instance) override;

        /**
         * Check if the bind pose transformations of the given actor matching the one from this skeletal motion.
         * In case the bind poses are not matching skinning errors can occur when using motion mirroring.
         * A detailed report about the non-matching transformation attributes will be logged.
         * @param actor A pointer to the actor to check the bind pose for.
         * @return True in case the given actor bind pose matches the one from this skeletal motion, false if not.
         */
        bool CheckIfIsMatchingActorBindPose(Actor* actor);

        /**
         * Output the skeleton of a given actor, when applying the bind pose of this motion onto it.
         * @param actor The actor from which to use the bind pose in case there are some nodes that are not animated in the motion while they still are in the actor.
         * @param outPose The output pose containing the bind pose of the motion. Please keep in mind to only use GetLocalSpaceTransform and GetModelSpaceTransform on this pose.
         */
        void CalcMotionBindPose(Actor* actor, Pose* outPose);

        //----------------
        void RemoveMorphSubMotion(uint32 nr, bool delFromMem);
        uint32 FindMorphSubMotionByID(uint32 id) const;
        uint32 FindMorphSubMotionByPhoneme(MorphTarget::EPhonemeSet phonemeSet) const;

        /**
         * Does this morph motion influence any phoneme morph targets?
         * @result Returns true when phonemes are influenced by this motion, otherwise false is returned.
         */
        bool GetDoesInfluencePhonemes() const;

        /**
         * Set the status if this morph target influences any phonemes morph targets or not.
         * @param enabled Set to true when this motion influences morph targets that are also phonemes. Otherwise set to false.
         */
        void SetDoesInfluencePhonemes(bool enabled);

        /**
         * Automatically update the status you can adjust with SetDoesInfluencePhonemes.
         */
        void UpdatePhonemeInfluenceStatus();

        //-------------------

        /**
         * Get the number of morph sub-motions in this motion.
         * @result The number of morph sub-motions.
         */
        MCORE_INLINE uint32 GetNumMorphSubMotions() const                                   { return static_cast<uint32>(mMorphSubMotions.size()); }

        /**
         * Add a sub-motion to this motion.
         * @param subMotion The sub-motion to add.
         */
        void AddMorphSubMotion(MorphSubMotion* subMotion);

        /**
         * Set the number of submotions.
         * @param numSubMotions The number of submotions to allocate space for in the array.
         */
        void SetNumMorphSubMotions(uint32 numSubMotions);

        /**
         * Set the value of a given submotion in the array of submotions.
         * @param index The submotion number, which must be in range of [0..GetNumMorphSubMotions()-1].
         * @param subMotion The value of the submotion to set at this array index.
         */
        void SetMorphSubMotion(uint32 index, MorphSubMotion* subMotion);

        /**
         * Get a pointer to a specified sub-motion.
         * @param nr The motion part number.
         * @result A pointer to the sub-motion.
         */
        MCORE_INLINE MorphSubMotion* GetMorphSubMotion(uint32 nr) const                     { return mMorphSubMotions[nr]; }

        //----------------

        /**
         * Scale all motion data.
         * This is a very slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        void Scale(float scaleFactor) override;

        void BasicRetarget(const MotionInstance* motionInstance, const SkeletalSubMotion* subMotion, uint32 nodeIndex, Transform& inOutTransform);

    protected:
        class SubMotionIndexCache
            : public EventHandler
        {
            friend class EventHandler;

        public:
            AZ_CLASS_ALLOCATOR(SubMotionIndexCache, MotionAllocator, 0)

            SubMotionIndexCache() = default;
            ~SubMotionIndexCache() = default;

            void CreateMotionLinks(Actor* actor, SkeletalMotion& skeletalMotion, MotionInstance* instance);

        private:
            const AZStd::vector<EventTypes> GetHandledEventTypes() const override
            {
                return {
                           EVENT_TYPE_ON_DELETE_ACTOR
                };
            }
            void OnDeleteActor(Actor* actor) override;

            using IndexVector = AZStd::vector<uint32, STLAllocator<MotionAllocator> >;  // the vector index is the nodeIndex, the value is the submotion index

            static void RecursiveAddIndices(const IndexVector& indicesVector, Actor* actor, uint32 nodeIndex, MotionInstance* instance);

            AZStd::unordered_map<Actor*,
                IndexVector,
                AZStd::hash<Actor*>,
                AZStd::equal_to<Actor*>,
                STLAllocator<MotionAllocator> > m_subMotionIndexByActorNode;

            AZStd::shared_mutex m_mutex;
        };

        AZStd::vector<SkeletalSubMotion*, STLAllocator<MotionAllocator> >    mSubMotions;            /**< The sub-motions. */
        AZStd::vector<MorphSubMotion*, STLAllocator<MotionAllocator> >       mMorphSubMotions;       /**< The morph sub-motions, that contain the keyframing data. */
        SubMotionIndexCache                  m_subMotionIndexCache;  /**< A cache of sub-motion index by actor's node. The vector will be the same size as the actor's node count. */
        bool                                 mDoesInfluencePhonemes; /**< Does this motion influence any phonemes? */


        /**
         * Constructor.
         * @param name The name/description of the motion.
         */
        SkeletalMotion(const char* name);

        /**
         * Destructor.
         */
        ~SkeletalMotion();

        void MirrorPose(Pose* inOutPose, MotionInstance* instance);
    };
} // namespace EMotionFX
