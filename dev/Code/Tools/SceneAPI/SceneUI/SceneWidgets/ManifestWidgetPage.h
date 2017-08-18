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

#include <QWidget.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Uuid.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

namespace AZ
{
    class SerializeContext;

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IManifestObject;
        }

        namespace UI
        {
            // QT space
            namespace Ui
            {
                class ManifestWidgetPage;
            }
            class ManifestWidgetPage : public QWidget, public AzToolsFramework::IPropertyEditorNotify
            {
                Q_OBJECT
            public:
                ManifestWidgetPage(SerializeContext* context, AZStd::vector<AZ::Uuid>&& classTypeIds);

                // Sets the number of entries the user can add through this widget. It doesn't limit
                //      the amount of entries that can be stored.
                void SetCapSize(size_t size);
                size_t GetCapSize() const;

                bool SupportsType(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);
                bool AddObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);
                bool RemoveObject(const AZStd::shared_ptr<DataTypes::IManifestObject>& object);

                size_t ObjectCount() const;
                void Clear();

                void ScrollToBottom();

                // IPropertyEditorNotify Interface Methods
                void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
                void SealUndoStack() override;

            signals:
                void PageUpdated();

            protected slots:
                // Callback that's triggered when the add button only has 1 entry.
                void OnSingleGroupAdd();

            protected:
                // Callback that's triggered when the add button has multiple entries.
                void OnMultiGroupAdd(const Uuid& id);

                void BuildAndConnectAddButton();
                
                AZStd::string ClassIdToName(const Uuid& id) const;
                void AddNewObject(const Uuid& id);

                AZStd::vector<AZ::Uuid> m_classTypeIds;
                AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>> m_objects;
                QScopedPointer<Ui::ManifestWidgetPage> ui;
                AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
                SerializeContext* m_context;
                size_t m_capSize;
            };
        } // UI
    } // SceneAPI
} // AZ
