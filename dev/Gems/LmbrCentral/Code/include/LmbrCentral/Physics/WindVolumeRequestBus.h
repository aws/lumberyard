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
     * Messages serviced by the WindVolumeComponent
     */
    class WindVolumeRequests
        : public AZ::ComponentBus
    {
    public:

        /**
         *  Sets an ellipsoidal falloff
         */
        virtual void SetEllipsoidal(bool ellipsoidal) = 0;

        /**
         * Returns true if the volume is ellipsoidal          
         */
        virtual bool GetEllipsoidal() = 0;

        /**
         * Sets the distance after which the distance-based falloff begins
         */
        virtual void SetFalloffInner(float falloffInner) = 0;

        /**
         * Gets the distance after which the distance-based falloff begins
         */
        virtual float GetFalloffInner() = 0;

        /**
         * Sets the speed of the wind
         */
        virtual void SetSpeed(float speed) = 0;

        /**
         * Gets the speed of the wind
         */
        virtual float GetSpeed() = 0;

        /** 
         * Sets the air resistance causing light physicalised objects to experience a bouyancy force
         */ 
        virtual void SetAirResistance(float airResistance) = 0;

        /** 
         * Gets the air resistance
         */
        virtual float GetAirResistance() = 0;

        /**
         * Sets the air density causing physicalised objects to experience a slow down
         */
        virtual void SetAirDensity(float airDensity) = 0;

        /**
         * Gets the air density
         */
        virtual float GetAirDensity() = 0;

        /**
         * Sets the direction the wind is blowing. If zero, then the direction is considered omnidirectional
         */
        virtual void SetWindDirection(const AZ::Vector3& direction) = 0;

        /**
         * Gets the direction the wind is blowing. If zero, then the direction is considered omnidirectional
         */
        virtual const AZ::Vector3& GetWindDirection() = 0;

        /** 
         * Sets the size of the wind volume. [-size.x, -size.y, -size.z] -> [size.x, size.y, size.z]
         */
        virtual void SetVolumeSize(const AZ::Vector3& size) = 0;

        /** 
         * Gets the size of the wind volume
         */
        virtual const AZ::Vector3& GetVolumeSize() = 0;
    };

    using WindVolumeRequestBus = AZ::EBus<WindVolumeRequests>;
}
