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
#include "EMotionFXConfig.h"
#include "EMotionFXManager.h"
#include "PlayBackInfo.h"
#include "BaseObject.h"

#include <MCore/Source/StringIdPool.h>
#include <MCore/Source/Distance.h>


namespace EMotionFX
{
    // forward declarations
    class Node;
    class Actor;
    class MotionInstance;
    class Pose;
    class Transform;
    class MotionEventTable;

    /**
     * The motion base class.
     * The unified motion processing system requires all motions to have a base class.
     * This base class is the Motion class (so this one). Different types of motions can be for example
     * skeletal motions (body motions) or facial motions. The main function inside this base class is the method
     * named Update, which will output the resulting transformations into a Pose object.
     */
    class EMFX_API Motion
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Set the name of the motion.
         * @param name The name of the motion.
         */
        void SetName(const char* name);

        /**
         * Returns the name of the motion.
         * @result The name of the motion.
         */
        const char* GetName() const;

        /**
         * Returns the name of the motion, as a AZStd::string object.
         * @result The name of the motion.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Set the filename of the motion.
         * @param[in] filename The filename of the motion.
         */
        void SetFileName(const char* filename);

        /**
         * Get the filename of the motion.
         * @result The filename of the motion.
         */
        const char* GetFileName() const;

        /**
         * Returns the filename of the motion, as a AZStd::string object.
         * @result The filename of the motion.
         */
        const AZStd::string& GetFileNameString() const;

        /**
         * Returns the type identification number of the motion class.
         * @result The type identification number.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Gets the type as a description. This for example could be "FacialMotion" or "SkeletalMotion".
         * @result The string containing the type of the motion.
         */
        virtual const char* GetTypeString() const = 0;

        /**
         * Set the unique identification number for the motion.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Get the unique identification number for the motion.
         * @return The unique identification number.
         */
        uint32 GetID() const;

        /**
         * Calculates the node transformation of the given node for this motion.
         * @param instance The motion instance that contains the motion links to this motion.
         * @param outTransform The node transformation that will be the output of this function.
         * @param actor The actor to apply the motion to.
         * @param node The node to apply the motion to.
         * @param timeValue The time value.
         * @param enableRetargeting Set to true if you like to enable motion retargeting, otherwise set to false.
         */
        virtual void CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting)      { MCORE_UNUSED(instance); MCORE_UNUSED(outTransform); MCORE_UNUSED(actor); MCORE_UNUSED(node); MCORE_UNUSED(timeValue); MCORE_UNUSED(enableRetargeting); }

        /**
         * Creates the motion links inside a given actor.
         * So we know what nodes are effected by what motions, this allows faster updates.
         * @param actor The actor to create the links in.
         * @param instance The motion instance to use for this link.
         */
        virtual void CreateMotionLinks(Actor* actor, MotionInstance* instance) = 0;

        /**
         * Get the maximum time value of the motion.
         * @result The maximum time value, in seconds.
         */
        float GetMaxTime() const;

        /**
         * Set the maximum time of the motion.
         * @param maxTime The maximum playback time (the duration) of the motion, in seconds.
         */
        void SetMaxTime(float maxTime);

        /**
         * Update the maximum time of this animation. Normally called after loading a motion.
         */
        virtual void UpdateMaxTime() = 0;

        /**
         * Returns if this motion type should overwrite body parts that are not touched by a motion
         * when played in non-mixing mode. Skeletal motions would return true, because if you play a non-mixing
         * motion you want the complete body to go into the new motion.
         * However, facial motions only need to adjust the facial bones, so basically they are always played in
         * mixing mode. Still, the behaviour of mixing motions is different than motions that are played in non-mixing
         * mode. If you want to play the motion in non-mixing mode and don't want to reset the nodes that are not influenced
         * by this motion, the motion type should return false.
         * @result Returns true if the motion type resets nodes even when they are not touched by the motion, when playing in non-mix mode.
         *        Otherwise false is returned.
         */
        virtual bool GetDoesOverwriteInNonMixMode() const = 0;

        /**
         * Check if this motion supports the CalcNodeTransform method.
         * This is for example used in the motion based actor repositioning code.
         * @result Returns true when the CalcNodeTransform method is supported, otherwise false is returned.
         */
        virtual bool GetSupportsCalcNodeTransform() const           { return false; }

        /**
         * Make the motion loopable, by adding a new keyframe at the end of the keytracks.
         * This added keyframe will have the same value as the first keyframe.
         * @param fadeTime The relative offset after the last keyframe. If this value is 0.5, it means it will add
         *                 a keyframe half a second after the last keyframe currently in the keytrack.
         */
        virtual void MakeLoopable(float fadeTime = 0.3f) = 0;

        /**
         * Get the event table.
         * This event table stores all motion events and can execute them as well.
         * @result The event table, which stores the motion events.
         */
        MotionEventTable* GetEventTable() const;

        /**
         * Set the motion framerate.
         * @param motionFPS The number of keyframes per second.
         */
        void SetMotionFPS(float motionFPS);

        /**
         * Get the motion framerate.
         * @return The number of keyframes per second.
         */
        float GetMotionFPS() const;

        /**
         * The main update method, which outputs the result for a given motion instance into a given output local pose.
         * @param inPose The current pose, as it is currently in the blending pipeline.
         * @param outPose The output pose, which this motion will modify and write its output into.
         * @param instance The motion instance to calculate the pose for.
         */
        virtual void Update(const Pose* inPose, Pose* outPose, MotionInstance* instance) = 0;

        /**
         * Specify the actor to use as retargeting source.
         * This would be the actor from which the motion was originally exported.
         * So if you would play a human motion on a dwarf, you would pass the actor that represents the human as parameter.
         * @param actor The actor to use as retarget source.
         */
        virtual void SetRetargetSource(Actor* actor)                            { MCORE_UNUSED(actor); }

        /**
         * Set the pointer to some custom data that you'd like to associate with this motion object.
         * Please keep in mind you are responsible yourself for deleting any allocations you might do
         * inside this custom data object. You can use the motion event system to detect when a motion gets
         * deleted to help you find out when to delete allocated custom data.
         * @param dataPointer The pointer to your custom data. This can also be nullptr.
         */
        void SetCustomData(void* dataPointer);

        /**
         * Get the pointer to some custom data that you'd like to associate with this motion object.
         * Please keep in mind you are responsible yourself for deleting any allocations you might do
         * inside this custom data object. You can use the motion event system to detect when a motion gets
         * deleted to help you find out when to delete allocated custom data.
         * @result A pointer to the custom data linked with this motion object, or nullptr when it has not been set.
         */
        void* GetCustomData() const;

        /**
         * Allocate memory for the default playback info. After creating the default playback info using this function you can retrieve a
         * pointer to it by calling the GetDefaultPlayBackInfo() function.
         */
        void CreateDefaultPlayBackInfo();

        /**
         * Set the default playback info to the given playback info. In case the default playback info hasn't been created yet this function will automatically take care of that.
         * @param playBackInfo The new playback info which will be copied over to this motion's default playback info.
         */
        void SetDefaultPlayBackInfo(const PlayBackInfo& playBackInfo);

        /**
         * Get the default playback info of this motion.
         * @return A pointer to the default playback info, nullptr in case it hasn't been allocated yet. You can call CreateDefaultPlayBackInfo() to allocate memory for
         *         the default playback info. This case only happens when the loaded motion file didn't contain a default playback info in the file already.
         */
        PlayBackInfo* GetDefaultPlayBackInfo() const;

        /**
         * Get the motion extraction flags.
         * @result The motion extraction flags, which can be a combination of the values inside the specific enum.
         */
        EMotionExtractionFlags GetMotionExtractionFlags() const;

        /**
         * Set the motion extraction flags.
         * @param mask The motion extraction flags to set.
         */
        void SetMotionExtractionFlags(EMotionExtractionFlags flags);

        /**
         * Set the dirty flag which indicates whether the user has made changes to the motion. This indicator should be set to true
         * when the user changed something like adding a motion event. When the user saves the motion, the indicator is usually set to false.
         * @param dirty The dirty flag.
         */
        void SetDirtyFlag(bool dirty);

        /**
         * Get the dirty flag which indicates whether the user has made changes to the motion. This indicator is set to true
         * when the user changed something like adding a motion event. When the user saves the motion, the indicator is usually set to false.
         * @return The dirty flag.
         */
        bool GetDirtyFlag() const;

        /**
         * Set if we want to automatically unregister this motion from the motion manager when we delete this motion.
         * On default this is set to true.
         * @param enabled Set to true when you wish to automatically have the motion unregistered, otherwise set it to false.
         */
        void SetAutoUnregister(bool enabled);

        /**
         * Check if this motion is automatically being unregistered from the motion manager when this motion gets deleted or not.
         * @result Returns true when it will get automatically deleted, otherwise false is returned.
         */
        bool GetAutoUnregister() const;

        /**
         * Marks the object as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        void SetUnitType(MCore::Distance::EUnitType unitType);
        MCore::Distance::EUnitType GetUnitType() const;

        void SetFileUnitType(MCore::Distance::EUnitType unitType);
        MCore::Distance::EUnitType GetFileUnitType() const;

        /**
         * Scale all motion data.
         * This is a very slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        virtual void Scale(float scaleFactor)       { MCORE_UNUSED(scaleFactor); }

        /**
         * Scale to a given unit type.
         * This method does nothing if the motion is already in this unit type.
         * You can check what the current unit type is with the GetUnitType() method.
         * @param targetUnitType The unit type to scale into (meters, centimeters, etc).
         */
        void ScaleToUnitType(MCore::Distance::EUnitType targetUnitType);

    protected:
        AZStd::string               mFileName;              /**< The filename of the motion. */
        PlayBackInfo*               mDefaultPlayBackInfo;   /**< The default/fallback motion playback info which will be used when no playback info is passed to the Play() function. */
        MotionEventTable*           mEventTable;            /**< The event table, which contains all events, and will make sure events get executed. */
        MCore::Distance::EUnitType  mUnitType;              /**< The type of units used. */
        MCore::Distance::EUnitType  mFileUnitType;          /**< The type of units used, inside the file that got loaded. */
        void*                       mCustomData;            /**< A pointer to custom user data that is linked with this motion object. */
        float                       mMaxTime;               /**< The maximum playback time of the animation. */
        float                       mMotionFPS;             /**< The number of keyframes per second. */
        uint32                      mNameID;                /**< The ID represention the name or description of this motion. */
        uint32                      mID;                    /**< The unique identification number for the motion. */
        EMotionExtractionFlags      mExtractionFlags;       /**< The motion extraction flags, which define behavior of the motion extraction system when applied to this motion. */
        bool                        mDirtyFlag;             /**< The dirty flag which indicates whether the user has made changes to the motion since the last file save operation. */
        bool                        mAutoUnregister;        /**< Automatically unregister the motion from the motion manager when this motion gets deleted? Default is true. */

#if defined(EMFX_DEVELOPMENT_BUILD)
        bool                        mIsOwnedByRuntime;
#endif // EMFX_DEVELOPMENT_BUILD

        /**
         * Constructor.
         * @param name The name/description of the motion.
         */
        Motion(const char* name);

        /**
         * Destructor.
         */
        virtual ~Motion();
    };
} // namespace EMotionFX
