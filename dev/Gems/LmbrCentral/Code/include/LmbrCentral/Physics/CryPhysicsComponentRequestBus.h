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
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/std/containers/array.h>

struct IPhysicalEntity;
struct pe_params;
struct pe_action;
struct pe_status;

namespace LmbrCentral
{
    /*!
     * Messages (specific to CryPhysics) serviced by the in-game physics component.
     */
    class CryPhysicsComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~CryPhysicsComponentRequests() = default;

        virtual IPhysicalEntity* GetPhysicalEntity() = 0;
        virtual void GetPhysicsParameters(pe_params& outParameters) = 0;
        virtual void SetPhysicsParameters(const pe_params& parameters) = 0;
        virtual void GetPhysicsStatus(pe_status& outStatus) = 0;
        virtual void ApplyPhysicsAction(const pe_action& action, bool threadSafe) = 0;
        //! Get the entity's uniform damping coefficient while submerged in water.
        virtual float GetWaterDamping() { return 0.0f; }

        //! Set the entity's uniform damping coefficient while submerged in water
        //! A cheaper alternative/addition to water resistance (applies uniform damping when in water).
        //! Sets the strength of the damping on an object's movement as soon as it is situated underwater. 
        //! Most objects can work with 0 damping. If an object has trouble coming to rest, try values like 0.2-0.3.
        //! Values of 0.5 and higher appear visually as overdamping. Note that when several objects are in contact, 
        //! the highest damping is used for the entire group.
        //! \param waterDamping
        virtual void SetWaterDamping(float /*waterDamping*/) { }

        //! Get the entity's density applied when it interacts with water
        virtual float GetWaterDensity() { return 0.0f; }

        //! Set the entity's density applied when it interacts with water
        //! Can be used to override the default water density(1000).Lower values assume that the body is floating in 
        //! the water that's less dense than it actually is, and thus it will sink easier.
        //! (100..1000) This parameter could be used to specify that the object's physical geometry can leak. For 
        //! instance, ground vehicles usually have quite large geometry volumes, but they are not waterproof, thus 
        //! Archimedean force acting on them will be less than submerged_volume 1000 (with 1000 being the actual 
        //! density underwater).
        //! Decreasing per object effective water density will allow such objects to sink(as they would in reality) 
        //! while still having large - volume physical geometry.
        //! Important note : if you are changing the default value(1000), it is highly recommended that you also 
        //! change water_resistance in the same way(a rule of thumb might be to always keep them equal).
        //! \param waterDensity 
        virtual void SetWaterDensity(float /*waterDensity*/) { }

        //! Get the entity's medium resistance while it's in water
        virtual float GetWaterResistance() { return 0.0f; }

        //! Set the entity's medium resistance while it's in water
        //! Can be used to override the default water resistance (1000). Sets how strongly the water affects the body 
        //! (this applies to both water flow and neutral state).
        //! (0..2000) Water resistance coefficient.If non - 0, precise water resistance is calculated.Otherwise only 
        //! water_damping(proportional to the submerged volume) is used to uniformly damp the movement.The former is 
        //! somewhat slower, but not prohibitively, so it is advised to always set the water resistance. Although water 
        //! resistance is not too visible on a general object, setting it to a suitable value will prevent very light 
        //! objects from jumping in the water, and water flow will affect things more realistically.Note that water 
        //! damping is used regardless of whether water resistance is 0, so it is better to set damping to 0 when 
        //! resistance is turned on.
        //! \param waterResistance 
        virtual void SetWaterResistance(float /*waterResistance*/) { }

        //! Get the density of the entity, in kg/m^3, (Volume / Mass). Water density affects the way objects interact
        //! with other objects and float in the water (they sink if their density is more than that of the water). 
        virtual float GetDensity() { return 0.0f; }

        //! Set the density of the entity, in kg/m^3
        //! \param density
        virtual void SetDensity(float /*density*/) { }

        //! Get the angular acceleration of the entity
        virtual AZ::Vector3 GetAngularAcceleration() { return AZ::Vector3::CreateZero(); }

        //! Get the linear acceleration of the entity
        virtual AZ::Vector3 GetAcceleration() { return AZ::Vector3::CreateZero(); }

        //! Whether the IPhysicalEntity is completely set up.
        //! We don't consider things "fully enabled" until the IPhysicalEntity
        //! has collision geometry.
        virtual bool IsPhysicsFullyEnabled() { return false; }
    };
    using CryPhysicsComponentRequestBus = AZ::EBus<CryPhysicsComponentRequests>;

} // namespace LmbrCentral
