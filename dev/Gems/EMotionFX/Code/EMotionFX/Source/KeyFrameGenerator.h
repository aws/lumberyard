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
#include "KeyTrackLinear.h"
#include "BaseObject.h"


namespace EMotionFX
{
    /**
     * The keyframe generator template base class.
     * Keyframe generators can generate keyframes in a given keytrack.
     * This can sometimes automate behaviour of specific things. An example is an eyeblink generator, which
     * can automatically generate keyframes which simulate eye blinks.
     * The main advantage of keyframe generators over procedural generators is that the values are editable afterwards,
     * while using procedural generators don't allow any fine tuning to the results.
     */
    template <class ReturnType, class StorageType>
    class KeyFrameGenerator
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(KeyFrameGenerator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_MOTIONS_MISC);

    public:
        /**
         * The main method, which applies the generator on a given keytrack.
         * Before you call the generate method you might want to set the properties of the generator, if it has any.
         * @param outTrack A pointer to the keytrack to write the keyframes in.
         */
        virtual void Generate(KeyTrackLinear<ReturnType, StorageType>* outTrack) = 0;

        /**
         * Get the description of the generator.
         * This should be the class name as a string.
         * @result The string containing the description of the generator.
         */
        virtual const char* GetDescription() const = 0;

        /**
         * Get the unique type identification number of this class.
         * It can be used to detect the keyframe generator types.
         * @result The unique ID of this class.
         */
        virtual uint32 GetType() const = 0;


    protected:
        /**
         * The constructor.
         */
        KeyFrameGenerator()
            : BaseObject()  {}

        /**
         * The destructor.
         */
        virtual ~KeyFrameGenerator()    {}
    };
} // namespace EMotionFX
