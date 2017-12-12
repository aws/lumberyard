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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace LmbrCentral
{
    /**
     * Force Mode
     */
    enum class ForceMode
    {
        ePoint,     ///< The force direction will be calculated relative to the volume's AABB centroid
        eDirection  ///< The force direction will be calculated using direction specified
    };   

    /**
     * Force Space
     */
    enum class ForceSpace
    {
        eLocalSpace,    ///< The force will be applied in local space relative to the volume's transform
        eWorldSpace     ///< The force will be applied in world space
    };

    /**
     * Messages serviced by the ForceVolumeComponent
     */
    class ForceVolumeRequests
        : public AZ::ComponentBus
    {
    public:

        /**
        * \brief Sets how the direction of the force applied to the entity will be calculated
        * \param mode The ForceMode to set
        */
        virtual void SetForceMode(ForceMode mode) = 0;

        /**
        * \brief Get the way the force direction is calculated
        * \return ForceMode indicating how the force direction is calculated
        */
        virtual ForceMode GetForceMode() = 0;

        /**
        * \brief Sets the space the force direction is calculated in. Only used if the ForceMode is eDirection
        * \param space The force space to set
        */
        virtual void SetForceSpace(ForceSpace space) = 0;

        /**
        * \brief Get the space the force direction is calculated in
        * \return The space the force direction is calculated in
        */
        virtual ForceSpace GetForceSpace() = 0;

        /*
         * \brief Sets if the force applied is mass dependent or not.
         * \param massDependent If true, the force applied to each entity is proportional to it's mass. Otherwise the same force will be applied to each entity regardless of their mass
         */
        virtual void SetForceMassDependent(bool massDependent) = 0;

        /**
        * \brief Gets if the force applied is mass dependent
        * \return true if the force is mass dependent, otherwise false
        */
        virtual bool GetForceMassDependent() = 0;

        /**
         * \brief Set the magnitude of the force to use when applying to entities within the volume
         * \param magnitude A value representing the length of the force vector in newtowns
         */
        virtual void SetForceMagnitude(float magnitude) = 0;

        /**
         * \brief Gets the magnitude of the force being applied to entities within the volume
         * \return A value representing the length of the force vector in newtowns
         */
        virtual float GetForceMagnitude() = 0;

        /**
         * \brief Sets the direction the force is applied in. Internally this will be transformed into the space defined, normalized, and scaled by the magnitude
         * \param direction A vector representing the direction of the force in the space specified by ForceSpace
         */
        virtual void SetForceDirection(const AZ::Vector3& direction) = 0;

        /**
         * \brief Gets the force direction being applied to entities within the volume
         * \return A direction vector in the space defined by ForceSpace
         */
        virtual const AZ::Vector3& GetForceDirection() = 0;

        /**
         * \brief Sets the damping of the volume. Damping is used to apply a force opposite to the entities velocity vector. The size of the force will be scaled by the entities speed
         * \param damping A value representing the size of the damping force
         */
        virtual void SetVolumeDamping(float damping) = 0;

        /**
         * \brief Gets the volume damping factor
         * \return A value representing the size of the damping force
         */
        virtual float GetVolumeDamping() = 0;

        /**
         * \brief Sets the density of the volume. Density is used to calculate a drag force on entities within the volume
         * \param density A value representing the density of the volume in kg/m^3
         */
        virtual void SetVolumeDensity(float density) = 0;

        /**
         * \brief Gets the density of the volume.
         * \return A value representing the density of the volume in kg/m^3
         */
        virtual float GetVolumeDensity() = 0;
    };

    using ForceVolumeRequestBus = AZ::EBus<ForceVolumeRequests>;
}
