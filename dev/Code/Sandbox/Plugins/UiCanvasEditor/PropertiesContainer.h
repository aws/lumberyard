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

class PropertiesContainer
    : public QScrollArea
{
    Q_OBJECT

public:

    PropertiesContainer(PropertiesWidget* propertiesWidget,
        EditorWindow* editorWindow);

    void Refresh(AzToolsFramework::PropertyModificationRefreshLevel refreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree, const AZ::Uuid* componentType = nullptr);
    void SelectionChanged(HierarchyItemRawPtrList* items);
    void SelectedEntityPointersChanged();
    bool IsCanvasSelected() { return m_isCanvasSelected; }

    void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& globalPos);

    void SetSelectedEntityDisplayNameWidget(QLabel* selectedEntityDisplayNameWidget);

private:

    // A SharedComponentInfo represents one component
    // which all selected entities have in common.
    // If entities have multiple of the same component-type
    // then there will be a SharedComponentInfo for each.
    // Example: Say 3 entities are selected and each entity has 2 MeshComponents.
    // There will be 2 SharedComponentInfo, one for each MeshComponent.
    // Each SharedComponentInfo::m_instances has 3 entries,
    // one for the Nth MeshComponent in each entity.
    struct SharedComponentInfo
    {
        SharedComponentInfo()
            : m_classData(nullptr)
            , m_compareInstance(nullptr)
        {}
        const AZ::SerializeContext::ClassData* m_classData;

        /// Components instanced (one from each entity).
        AZStd::vector<AZ::Component*> m_instances;

        /// Canonical instance to compare others against
        AZ::Component* m_compareInstance;
    };

    // A collection of SharedComponentInfo.
    // The map is keyed on the component-type.
    // In the case of /ref GenericComponentWrapper,
    // the type corresponds to the component-type being wrapped,
    // though SharedComponentInfo::m_instances still point to the
    // GenericComponentWrapper*.
    using ComponentTypeMap = AZStd::unordered_map<AZ::Uuid, AZStd::vector<SharedComponentInfo>>;

    void BuildSharedComponentList(ComponentTypeMap&, const AzToolsFramework::EntityIdList& entitiesShown);
    void BuildSharedComponentUI(ComponentTypeMap&, const AzToolsFramework::EntityIdList& entitiesShown);
    AzToolsFramework::ReflectedPropertyEditor* CreatePropertyEditor();

    void Update();

    void SetHeightOfContentRect();

    PropertiesWidget* m_propertiesWidget;
    EditorWindow* m_editorWindow;

    QWidget* m_containerWidget;
    QVBoxLayout* m_rowLayout;
    QLabel* m_selectedEntityDisplayNameWidget;

    using ComponentPropertyEditorMap = AZStd::unordered_map<AZ::Uuid, AZStd::vector<AzToolsFramework::ReflectedPropertyEditor*>>;
    ComponentPropertyEditorMap m_componentEditorsByType;

    bool m_selectionHasChanged;
    AZStd::vector<AZ::EntityId> m_selectedEntities;

    bool m_isCanvasSelected;

    // Pointer to entity that first entity is compared against for the purpose of rendering deltas vs. slice in the property grid.
    AZStd::unique_ptr<AZ::Entity> m_compareToEntity;

    // Global app serialization context, cached for internal usage during the life of the control.
    AZ::SerializeContext* m_serializeContext;
};
