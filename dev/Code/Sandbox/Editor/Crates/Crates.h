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

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <AzQtComponents/Buses/DragAndDrop.h>

#include <QCoreApplication>
#include <QtCore>

namespace AZ
{

    /**
    *  Crates requests are used to interface with .crate files including crate unpacking.
    */
    class CratesRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /**
        *  UnpackCrate does a series of steps to bring a crate into the current project and enable its gem.
        *  1. Extract the crate (.zip archive) to a temporary directory
        *  2. Verify the new crate's gem.json file for a valid UUID, version and link type for conflicts with existing gems
        *  3. Move the newly extracted gem to the Gems folder
        *  4. Enable the Gem for the current project
        */
        virtual void UnpackCrate(const AZStd::string& cratePath) = 0;
    };
    using CratesRequestsBus = AZ::EBus<CratesRequests>;

    class CratesHandler
        : public AZ::Component
        , public CratesRequestsBus::Handler
        , public AzQtComponents::DragAndDropEventsBus::Handler
    {
        Q_DECLARE_TR_FUNCTIONS(CratesHandler);
        
        enum NewGemState {
            UNKNOWN,
            NEW_GEM,
            NEW_CODE_GEM,
            NEW_GEM_VERSION,
            NEW_GEM_EXISTS_AND_IS_ENABLED,
            NEW_GEM_EXISTS_AND_IS_NOT_ENABLED,
            NEW_CODE_GEM_EXISTS_AND_IS_NOT_ENABLED
        };

        enum ImportActionFlags {
            FLAG_ABORT,
            FLAG_IMPORT,
            FLAG_ENABLE,
            FLAG_OPEN_ASSET_BROWSER,
            FLAG_OPEN_PROJECT_CONFIGURATOR,
            FLAG_OPEN_ASSET_PROCESSOR,

            IMPORT_FLAGS_SIZE
        };

    public:
        AZ_COMPONENT(CratesHandler, "{E2D448FD-EE12-4AC3-B886-5F42C5D3C4DE}")

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    private:
        static void Reflect(AZ::ReflectContext* context);
    public:
        void UnpackCrate(const AZStd::string& cratePath) override;
    public:
        void DragEnter(QDragEnterEvent* event) override;
        void Drop(QDropEvent* event) override;
    };

} // namespace AZ