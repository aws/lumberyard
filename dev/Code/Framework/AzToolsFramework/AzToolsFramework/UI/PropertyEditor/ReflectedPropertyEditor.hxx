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

#ifndef REFLECTEDPROPERTYEDITOR_H
#define REFLECTEDPROPERTYEDITOR_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "PropertyEditorAPI.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>

#pragma once

class QScrollArea;
class QLayout;
class QSpacerItem;
class QVBoxLayout;

namespace AZ
{
    struct ClassDataReflection;
    class SerializeContext;
}

namespace AzToolsFramework
{
    class ReflectedPropertyEditorState;
    class PropertyRowWidget;

    /**
     * the Reflected Property Editor is a Qt Control which you can place inside a GUI, which you then feed
     * a series of object(s) and instances.  Any object or instance with Editor reflection will then be editable
     * in the Reflected Property editor control, with the GUI arrangement specified in the edit reflection for
     * those objects.
     */
    class ReflectedPropertyEditor 
        : public QFrame
        , private PropertyEditorGUIMessages::Bus::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(ReflectedPropertyEditor, AZ::SystemAllocator, 0);

        typedef AZStd::unordered_map<InstanceDataNode*, PropertyRowWidget*> WidgetList;

        ReflectedPropertyEditor(QWidget* pParent);
        virtual ~ReflectedPropertyEditor();

        void Setup(AZ::SerializeContext* context, IPropertyEditorNotify* ptrNotify, bool enableScrollbars, int propertyLabelWidth = 200);

        void SetReadOnly(bool readOnly);

        // allows disabling display of root container property widgets
        void SetHideRootProperties(bool hideRootProperties);

        bool AddInstance(void* instance, const AZ::Uuid& classId, void* aggregateInstance = nullptr, void* compareInstance = nullptr);
        void SetCompareInstance(void* instance, const AZ::Uuid& classId);
        void ClearInstances();
        template<class T>
        bool AddInstance(T* instance, void* aggregateInstance = nullptr, void* compareInstance = nullptr)
        {
            return AddInstance(instance, AZ::AzTypeInfo<T>().Uuid(), aggregateInstance, compareInstance);
        }

        void InvalidateAll(); // recreates the entire tree of properties.
        void InvalidateAttributesAndValues(); // re-reads all attributes, and all values.
        void InvalidateValues(); // just updates the values inside properties.

        void SetSavedStateKey(AZ::u32 key); // a settings key which is used to store and load the set of things that are expanded or not and other settings

        void QueueInvalidation(PropertyModificationRefreshLevel level);
        //will force any queued invalidations to happen immediately
        void ForceQueuedInvalidation();

        void SetAutoResizeLabels(bool autoResizeLabels);

        InstanceDataNode* GetNodeFromWidget(QWidget* pTarget) const;
        PropertyRowWidget* GetWidgetFromNode(InstanceDataNode* node) const;

        void ExpandAll();
        void CollapseAll();

        const WidgetList& GetWidgets() const { return m_widgets; }

        int GetContentHeight() const;
        int GetMaxLabelWidth() const;

        void SetLabelWidth(int labelWidth);

        void SetSelectionEnabled(bool selectionEnabled);
        void SelectInstance(InstanceDataNode* node);
        InstanceDataNode* GetSelectedInstance() const;

        QSize sizeHint() const override;

        using InstanceDataHierarchyCallBack = AZStd::function<void(AzToolsFramework::InstanceDataHierarchy& /*hierarchy*/)>;
        void EnumerateInstances(InstanceDataHierarchyCallBack enumerationCallback);

        void SetValueComparisonFunction(const InstanceDataHierarchy::ValueComparisonFunction& valueComparisonFunction);

        // if you want it to save its state, you need to give it a user settings label:
        //void SetSavedStateLabel(AZ::u32 label);
        //static void Reflect(const AZ::ClassDataReflection& reflection);

        void SetDynamicEditDataProvider(DynamicEditDataProvider provider);

    signals:
        void OnExpansionContractionDone();
    private:
        AZ::SerializeContext*               m_context;
        IPropertyEditorNotify*              m_ptrNotify;
        typedef AZStd::list<InstanceDataHierarchy> InstanceDataHierarchyList;
        typedef AZStd::unordered_map<QWidget*, InstanceDataNode*> UserWidgetToDataMap;
        typedef AZStd::vector<PropertyRowWidget*> RowContainerType;
        InstanceDataHierarchyList           m_instances; ///< List of instance sets to display, other one can aggregate other instances.
        InstanceDataHierarchy::ValueComparisonFunction m_valueComparisonFunction;
        WidgetList m_widgets;
        RowContainerType m_widgetsInDisplayOrder;
        UserWidgetToDataMap m_userWidgetsToData;
        void AddProperty(InstanceDataNode* node, PropertyRowWidget* pParent, int depth);

        ////////////////////////////////////////////////////////////////////////////////////////
        // Support for logical property groups / visual hierarchy.
        PropertyRowWidget* GetOrCreateLogicalGroupWidget(InstanceDataNode* node, PropertyRowWidget* parent, int depth);
        size_t CountRowsInAllDescendents(PropertyRowWidget* pParent);

        using GroupWidgetList = AZStd::unordered_map<AZ::Crc32, PropertyRowWidget*>;
        GroupWidgetList m_groupWidgets;
        ////////////////////////////////////////////////////////////////////////////////////////

        QVBoxLayout* m_rowLayout;
        QScrollArea* m_mainScrollArea;
        QWidget* m_containerWidget;

        QSpacerItem* m_spacer;
        AZStd::vector<PropertyRowWidget*> m_widgetPool;

        PropertyRowWidget* CreateOrPullFromPool();
        void ReturnAllToPool();

        void SaveExpansion();
        bool ShouldRowAutoExpand(PropertyRowWidget* widget) const;

        void AdjustLabelWidth();

        AZ::u32 m_savedStateKey;

        int m_propertyLabelWidth;
        bool m_autoResizeLabels;

        PropertyRowWidget *m_selectedRow;
        bool m_selectionEnabled;

        //////////////////////////////////////////////////////////////////////
        // PropertyEditorGUIMessages::Bus::Handler
        virtual void RequestWrite(QWidget* editorGUI) override;
        virtual void RequestRefresh(PropertyModificationRefreshLevel) override;
        void RequestPropertyNotify(QWidget* editorGUI) override;
        void OnEditingFinished(QWidget* editorGUI) override;
        //////////////////////////////////////////////////////////////////////

        AZStd::intrusive_ptr<ReflectedPropertyEditorState> m_savedState;

        AZ::u32 CreatePathKey(PropertyRowWidget* widget) const;
        bool CheckSavedExpandState(AZ::u32 pathKey) const;
        bool HasSavedExpandState(AZ::u32 pathKey) const;

        PropertyModificationRefreshLevel m_queuedRefreshLevel;
        virtual void paintEvent(QPaintEvent* event) override;

        bool m_readOnly;
        bool m_hideRootProperties;
        bool m_queuedTabOrderRefresh;
        int m_expansionDepth;
        DynamicEditDataProvider m_dynamicEditDataProvider;
    private slots:
        void OnPropertyRowExpandedOrContracted(PropertyRowWidget* widget, InstanceDataNode* node, bool expanded, bool fromUserInteraction);
        void DoRefresh();
        void RecreateTabOrder();
        void OnPropertyRowRequestClear(PropertyRowWidget* widget, InstanceDataNode* node);
        void OnPropertyRowRequestContainerRemoveItem(PropertyRowWidget* widget, InstanceDataNode* node);
        void OnPropertyRowRequestContainerAddItem(PropertyRowWidget* widget, InstanceDataNode* node);
    };
}

#endif
