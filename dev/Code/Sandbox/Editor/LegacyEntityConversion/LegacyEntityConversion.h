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

#include <AzCore/Memory/SystemAllocator.h>
#include <LegacyEntityConversion/LegacyEntityConversionBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
//Some headers needed for LegacyEntityConversionUtil.inl
#include <QString>
#include <Editor/Objects/BaseObject.h>
#include <Objects/EntityObject.h>
#include <Util/Variable.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AZ
{
    namespace LegacyConversion
    {
        /**
        * This class handles the conversion of some of the CryEntity types that Editor knows about into
        * equivalent AZ::Component entities.  There may be more Handlers of this bus in other parts of the code
        * or in gems.
        */
        class Converter
            : private LegacyConversionEventBus::Handler
            , private AzToolsFramework::EditorEntityContextNotificationBus::Handler

        {
        public:
            AZ_CLASS_ALLOCATOR(Converter, AZ::SystemAllocator, 0);
            Converter();
            // ------------------- LegacyConversionEventBus::Handler -----------------------------
            LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
            bool BeforeConversionBegins() override;
            bool AfterConversionEnds() override;
            // END ----------------LegacyConversionEventBus::Handler ------------------------------

            //! Fired before the EditorEntityContext exports the root level slice to the game stream
            void OnSaveStreamForGameBegin(AZ::IO::GenericStream& gameStream, AZ::DataStream::StreamType streamType, AZStd::vector<AZStd::unique_ptr<AZ::Entity>>& levelEntities) override;

        private:
            void ConvertIngameDesignerObjects(AZStd::vector<AZStd::unique_ptr<AZ::Entity>>& levelEntities);
        };
    }
}

#include "LegacyEntityConversionUtil.inl"
