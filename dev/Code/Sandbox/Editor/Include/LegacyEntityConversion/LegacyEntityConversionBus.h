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

/** @file
* Contains the busses that are used to convert legacy CryEngine data to Lumberyard data (Components and Entities)
* Only editor plugins and editor gems should listen to this bus and service it.
*/

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>

class CBaseObject;

namespace AZ
{
    struct Uuid;
    namespace LegacyConversion
    {
        /**
        * The Conversion Result is returned by all listeners when asked to convert an entity
        * and indicates what they'd like to do with the entity being converted.
        * If any respondent returns HandledDeleteEntity, entities will only be autodeleted if all respondents return either Ignored
        * or also return HandledDeleteEntity
        */
        enum class LegacyConversionResult
        {
            Ignored,                ///< My handler does not handle this kind of entity - don't do anything specific
            Failed,                 ///< My handler processed this entity but failed to convert it.  (Keep the old entity!)
            HandledKeepEntity,      ///< My handler processed this entity successfully, but keep the old entity about
            HandledDeleteEntity,    ///< My handler processed this entity successfully, remove the old entity.
        };

        /**
        * This event bus will transmit information about legacy entities needing conversion.
        * If you intend to implement legacy conversion, you will need to listen to this bus and handle messages appropriately.
        */
        class LegacyConversionEvents : public AZ::EBusTraits
        {
        public:
            // -------------------- EBUS CONFIGURATION -------------------
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // Unaddressed bus
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple; // any number of handlers.
            // END ---------------- EBUS CONFIGURATION -------------------

            virtual ~LegacyConversionEvents() {}

            /**
            * Implement this to actually convert the entity to your new system.
            * You should return Ignored unless you actually convert this
            * If you fully converted the entity to the new system and there is nothing left behind, consider returning HandledDeleteEntity
            * in order to clean up the old CryEntity and leave only the new Component Entity in its place.
            * during conversion you may create new AZ::Entities, add component(s) to the, fill out the properties, etc.
            * You should emit errors and warnings using AZ_Warning and AZ_Error, and debug information using AZ_TracePrintf.
            */
            virtual LegacyConversionResult ConvertEntity(CBaseObject* entityToConvert) = 0;

            /**
            * Before any conversion begins of any entity, the BeforeConversionBegins function is called.
            * If any system responds with false, conversion will not occur.  Note that no change to the data should be performed
            * since this happens before we save a backup.  Once everyone returns true, backup will be made, and ConvertEntity will be called
            */
            virtual bool BeforeConversionBegins() = 0;
            /**
            * This function is called once conversion is complete.
            * If you generated state during conversion and need to add entities to the level that you have been accumulating
            * during conversion, this is your opportunity to do so.
            * If any listener returns false here, we will assume the entire conversion process has failed, and will UNDO the conversion.
            * otherwise, we will finish the conversion and seal the undo step.
            */
            virtual bool AfterConversionEnds() = 0;
        };

        using LegacyConversionEventBus = AZ::EBus<LegacyConversionEvents>;

        /**
        * Create Entity Result is used to respond to CreateConvertedEntity
        * It differentiates between the various reasons you could fail to create the entity.
        */
        enum class CreateEntityResult
        {
            Failed, ///< total failure - something went wrong with the entity system itself.
            FailedNoParent, ///< You requested that we fail if no parent was found and no parent was found
            FailedInvalidComponent, ///< You requested a component be added, but we could not find/add it.
        };

        /**
        * This request bus talks to the singleton inside the editor responsible for doing things to the world.
        * In general, it contains a number of utility functions you can use during entity conversion that make
        * it safer and easier to perform conversion tasks.
        */
        class LegacyConversionRequests : public AZ::EBusTraits
        {
        public:
            // -------------------- EBUS CONFIGURATION -------------------
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // Unaddressed bus
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single; // a singleton handles these requests.
            /// END ---------------- EBUS CONFIGURATION -------------------

            virtual ~LegacyConversionRequests() {}

            /**
            * Returns an ACTIVATED Entity based on the existing source object
            * Remember to deactivate it first if you intend to add components to it beyond the one you passed into this function.
            * It automatically adds required components to it and hint tags to indicate its a converted entity so that it can be
            * found later.   It automatically converts the transform and the parent for you, adds layer tags, etc.
            * @param sourceObject - the object to create the entity for
            * @param failIfParentNotFound - if set to true, this call will fail with FailedNoParent if this entity has a parent but it wasn't convertable.
            * @param componentsToAdd (optional - set to an empty list if you don't want any) - any number of components you want to add to the entity automatically.
            */
            virtual AZ::Outcome<AZ::Entity*, CreateEntityResult> CreateConvertedEntity(CBaseObject* sourceObject, bool failIfParentNotFound, const AZ::ComponentTypeList& componentsToAdd) = 0;

            /**
            * Find (if any) the entity that was already converted from a given source object.
            * @param sourceObjectUUID The UUID (From entity->GetId()) of the Cry Entity
            * @param sourceObjectName The name of the original object, for disambiguation
            * @return The found entityID - this could be an Invalid entity ID if no entity exists that matches.
            */
            virtual AZ::EntityId FindCreatedEntity(const AZ::Uuid& sourceObjectUUID, const char* sourceObjectName) = 0;

            /**
            * Find (if any) the entity that was already converted from a given source object.
            * This is a helper function that makes your code cleaner, internally it will call the above API.
            * @param sourceObject A pointer to the existing object.
            * @return The found entityID - this could be NULL if no entity exists that matches.
            */
            virtual AZ::EntityId FindCreatedEntityByExistingObject(const CBaseObject* sourceObject) = 0;

            /**
            * Generates a physics material library based on the data from Cry Engine surface types
            * @param targetFilePath - Full path to the file the material library should be written to
            * @return The success result.
            */
            virtual bool CreateSurfaceTypeMaterialLibrary(const AZStd::string& targetFilePath) = 0;
        };

        using LegacyConversionRequestBus = AZ::EBus<LegacyConversionRequests>;

    } // namespace LegacyConversion
} // namespace AZ
