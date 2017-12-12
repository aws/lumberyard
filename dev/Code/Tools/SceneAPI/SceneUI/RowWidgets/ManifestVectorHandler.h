#pragma once

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

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorWidget.h>


class QWidget;

namespace AZ
{
    class SerializeContext;

    namespace SceneAPI
    {
        namespace UI
        {
            using ManifestObjectSharedPointer = AZStd::shared_ptr<DataTypes::IManifestObject>;
            class ManifestVectorHandler
                : public QObject
                , public AzToolsFramework::PropertyHandler<AZStd::vector<ManifestObjectSharedPointer>, ManifestVectorWidget>
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                u32 GetHandlerName() const override;
                bool AutoDelete() const override;

                void ConsumeAttribute(ManifestVectorWidget* widget, u32 attrib,
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
                void WriteGUIValuesIntoProperty(size_t index, ManifestVectorWidget* GUI, property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;
                bool ReadValuesIntoGUI(size_t index, ManifestVectorWidget* GUI, const property_t& instance,
                    AzToolsFramework::InstanceDataNode* node) override;

                static void Register();
                static void Unregister();

            private:
                static SerializeContext* s_serializeContext;
                static ManifestVectorHandler* s_instance;
            };
        } // UI
    } // SceneAPI
} // AZ