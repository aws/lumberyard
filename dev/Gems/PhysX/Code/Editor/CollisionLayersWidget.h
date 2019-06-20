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

#include <AzFramework/Physics/Collision.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QWidget>

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace PhysX
{
    namespace Editor
    {
        class CollisionLayersWidget
            : public QWidget
            , private AzToolsFramework::IPropertyEditorNotify
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionLayersWidget, AZ::SystemAllocator, 0);

            static const AZ::u32 s_maxCollisionLayerNameLength = 32;
            static const AZStd::string s_defaultCollisionLayerName;

            explicit CollisionLayersWidget(QWidget* parent = nullptr);

            void SetValue(const Physics::CollisionLayers& layers);
            const Physics::CollisionLayers& GetValue() const;

        signals:
            void onValueChanged(const Physics::CollisionLayers& newValue);

        private:
            void CreatePropertyEditor(QWidget* parent);

            void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SealUndoStack() override;
            void SetWidgetParameters();

            /**
             * Checks if layer name at specified index is unique in the given array of layer names.
             * @return true if the  layer name modified to be unique, false otherwise.
             */
            bool ForceUniqueLayerName(AZ::u32 layerIndex
                , const AZStd::array<AZStd::string, Physics::CollisionLayers::s_maxCollisionLayers>& layerNames
                , AZStd::string& uniqueLayerNameOut);

            /**
             * Checks if layer name at specified index is unique in the given array of layer names.
             * @return true if the layer name is unique, false otherwise.
             */
            bool IsLayerNameUnique(AZ::u32 layerIndex,
                const AZStd::array<AZStd::string, Physics::CollisionLayers::s_maxCollisionLayers>& layerNames);

            AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
            Physics::CollisionLayers m_value;
        };
    } // namespace Editor
} // namespace PhysX
