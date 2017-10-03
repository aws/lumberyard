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
#include <MCore/Source/Vector.h>

#include "EMotionFXConfig.h"
#include "NodeAttribute.h"
#include "MemoryCategories.h"


namespace EMotionFX
{
    /**
     * The node/joint limit attribute class.
     * This class describes position, rotation and scale limits of a given node.
     * These values can be used during calculations of controllers, such as Inverse Kinematics solvers.
     * However, when a node has limits assigned to them, this does not mean these limits are used.
     * Controllers are not forced to use these limits, so adjusting values might not have any influence if they are
     * not supported by the specific controller.
     */
    class EMFX_API NodeLimitAttribute
        : public NodeAttribute
    {
    public:
        // the unique type ID of this node attribute
        enum
        {
            TYPE_ID = 0x07000002
        };

        /**
         * The limit activation flags.
         * They specify what components are limited.
         */
        enum ELimitType
        {
            TRANSLATIONX    = 1 << 0,   /**< Position limit on the x axis. */
            TRANSLATIONY    = 1 << 1,   /**< Position limit on the y axis. */
            TRANSLATIONZ    = 1 << 2,   /**< Position limit on the z axis. */
            ROTATIONX       = 1 << 3,   /**< Rotation limit on the x axis. */
            ROTATIONY       = 1 << 4,   /**< Rotation limit on the y axis. */
            ROTATIONZ       = 1 << 5,   /**< Rotation limit on the z axis. */
            SCALEX          = 1 << 6,   /**< Scale limit on the x axis. */
            SCALEY          = 1 << 7,   /**< Scale limit on the y axis. */
            SCALEZ          = 1 << 8    /**< Scale limit on the z axis. */
        };

        /**
         * The constructor.
         */
        static NodeLimitAttribute* Create();

        /**
         * Get the attribute type.
         * @result The attribute ID.
         */
        uint32 GetType() const override;

        /**
         * Get the attribute type as a string.
         * This string should contain the name of the class.
         * @result The string containing the type name.
         */
        const char* GetTypeString() const override;

        /**
         * Clone the node attribute.
         * @result Returns a pointer to a newly created exact copy of the node attribute.
         */
        NodeAttribute* Clone() override;

        /**
         * Set the minimum translation values, and automatically enable the limit to be true.
         * @param translateMin The minimum translation values.
         */
        void SetTranslationMin(const MCore::Vector3& translateMin);

        /**
         * Set the maximum translation values, and automatically enable the limit to be true.
         * @param translateMax The maximum translation values.
         */
        void SetTranslationMax(const MCore::Vector3& translateMax);

        /**
         * Set the minimum rotation angles, and automatically enable the limit to be true.
         * The rotation angles must be in radians.
         * @param rotationMin The minimum rotation angles.
         */
        void SetRotationMin(const MCore::Vector3& rotationMin);

        /**
         * Set the maximum rotation angles, and automatically enable the limit to be true.
         * The rotation angles must be in radians.
         * @param rotationMax The maximum rotation angles.
         */
        void SetRotationMax(const MCore::Vector3& rotationMax);

        /**
         * Set the minimum scale values, and automatically enable the limit to be true.
         * @param scaleMin The minimum scale values.
         */
        void SetScaleMin(const MCore::Vector3& scaleMin);

        /**
         * Set the maximum scale values, and automatically enable the limit to be true.
         * @param scaleMax The maximum scale values.
         */
        void SetScaleMax(const MCore::Vector3& scaleMax);

        /**
         * Get the minimum translation values.
         * @return The minimum translation values.
         */
        const MCore::Vector3& GetTranslationMin() const;

        /**
         * Get the maximum translation values.
         * @return The maximum translation values.
         */
        const MCore::Vector3& GetTranslationMax() const;

        /**
         * Get the minimum rotation values.
         * The rotation angles are in RADs.
         * @return The minimum rotation values.
         */
        const MCore::Vector3& GetRotationMin() const;

        /**
         * Get the maximum rotation values.
         * The rotation angles are in RADs.
         * @return The maximum rotation values.
         */
        const MCore::Vector3& GetRotationMax() const;

        /**
         * Get the minimum scale values.
         * @return The minimum scale values.
         */
        const MCore::Vector3& GetScaleMin() const;

        /**
         * Get the maximum scale values.
         * @return The maximum scale values.
         */
        const MCore::Vector3& GetScaleMax() const;

        /**
         * Enable or disable the limit for the specified limit type.
         * @param limitType The limit type to enable or disable.
         * @param flag True to enable the specified limit, false to disable it.
         */
        void EnableLimit(ELimitType limitType, bool flag = true);

        /**
         * Toggle limit state.
         * @param limitType The limit type to toggle.
         */
        void ToggleLimit(ELimitType limitType);

        /**
         * Determine if the specified limit is enabled or disabled.
         * @param limitType The limit type to check.
         * @return True if specified limit is limited, false it not.
         */
        bool GetIsLimited(ELimitType limitType) const;

    protected:
        MCore::Vector3      mTranslationMin;    /**< The minimum translation values. */
        MCore::Vector3      mTranslationMax;    /**< The maximum translation values. */
        MCore::Vector3      mRotationMin;       /**< The minimum rotation values. */
        MCore::Vector3      mRotationMax;       /**< The maximum rotation values. */
        MCore::Vector3      mScaleMin;          /**< The minimum scale values. */
        MCore::Vector3      mScaleMax;          /**< The maximum scale values. */
        uint16              mLimitFlags;        /**< The limit type activation flags. */

        /**
         * The constructor.
         */
        NodeLimitAttribute();

        /**
         * The destructor.
         */
        virtual ~NodeLimitAttribute();
    };
} // namespace EMotionFX

