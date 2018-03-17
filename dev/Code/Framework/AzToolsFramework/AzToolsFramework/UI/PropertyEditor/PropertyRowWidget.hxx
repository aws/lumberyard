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

#ifndef PROPERTY_ROW_WIDGET_H
#define PROPERTY_ROW_WIDGET_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtCore/QPointer>
#include <QtCore/QElapsedTimer>
#include "PropertyEditorAPI.h"

#pragma once

namespace AzToolsFramework
{

    // Renders the property names with an ellipsis (...) when they are too long.
    // In that scenario, the tooltip is modified to include the full name as well
    // as the description.
    // e.g.
    // Description "Some stuff that explains it!"
    // Displayed: "A Really Long Nam..."
    // Tooltip:
    // A Really Long Name
    // Some stuff that explains it!
    class ElidingLabel : public QWidget
    {
        Q_OBJECT

    public:
        explicit ElidingLabel(QWidget* parent = nullptr);

        void SetText(const QString& text);
        const QString& Text() const { return m_text; }

        void SetDescription(const QString& description);
        const QString& Description() const { return m_description; }

        void RefreshStyle();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void Elide();

        QString m_text;
        QString m_elidedText;
        QString m_description;

        QLabel* m_label;
    };

    class PropertyHandlerBase;
    class PropertyAttributeReader;
    // the purpose of a Property Row Widget is to house the user's property GUI
    // and serve as a conduit to talk to the system.  You should never need to do anything
    // with this guy, except tell him to expand, collapse, etc.
    class PropertyRowWidget : public QFrame
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(PropertyRowWidget, AZ::SystemAllocator, 0)
        PropertyRowWidget(QWidget* pParent);
        virtual ~PropertyRowWidget();

        virtual void Initialize(PropertyRowWidget* pParent, InstanceDataNode* dataNode, int depth, int labelWidth = 200);
        virtual void Initialize(const char* groupName, PropertyRowWidget* pParent, int depth, int labelWidth = 200);
        virtual void Clear(); // for pooling

        // --- NOT A UNIQUE IDENTIFIER ---
        virtual AZ::u32 GetIdentifier() const; // retrieve a stable identifier that identifies this node (note: Does not include heirarchy).  Use only for attempts to restore state.

        bool IsForbidExpansion() const; 
        bool AutoExpand() const { return m_autoExpand; }
        bool IsContainer() const { return m_isContainer; }
        bool IsContainerEditable() const { return m_isContainer && m_containerEditable; }
        
        int GetDepth() const { return m_treeDepth; }

        virtual void AddedChild(PropertyRowWidget* child);

        virtual void SetExpanded(bool expanded, bool fromUserInteraction = false);
        virtual bool IsExpanded() const { return m_expanded; }

        void DoExpandOrContract(bool expand, bool includeDescendents = false);

        PropertyHandlerBase* GetHandler() const;
        InstanceDataNode* GetNode() const { return m_sourceNode; }
        bool HasChildWidgetAlready() const; 
        void ConsumeChildWidget(QWidget* pChild);
        QWidget* GetChildWidget() const { return m_childWidget; }

        void SetParentRow(PropertyRowWidget* pParentRowWidget) { m_parentRow = pParentRowWidget; }
        PropertyRowWidget* GetParentRow() const { return m_parentRow; }

        AZStd::vector<PropertyRowWidget*>& GetChildrenRows() { return m_childrenRows; }
        bool HasChildRows() const;

        // check if theres a notification function.
        PropertyModificationRefreshLevel DoPropertyNotify();

        void RefreshAttributesFromNode(bool initial);
        void ConsumeAttribute(AZ::u32 attributeName, PropertyAttributeReader& reader, bool initial, QString* descriptionOut=nullptr, bool* foundDescriptionOut=nullptr);

        /// Repaint the control style, which is required any time object properties used
        /// by .qss are modified.
        void RefreshStyle();

        /// Post-process based on source node data.
        void OnValuesUpdated();

        QString label() const;

        QWidget* GetFirstTabWidget();
        void UpdateWidgetInternalTabbing();
        QWidget* GetLastTabWidget();

        //return size hint for left-hand side layout including the name label and any indentation
        QSize LabelSizeHint() const;
        void SetLabelWidth(int width);

        void SetSelectionEnabled(bool selectionEnabled);
        void SetSelected(bool selected);
        bool eventFilter(QObject *watched, QEvent *event) override;

        /// Apply tooltip to widget and some of its children.
        void SetDescription(const QString& text);

        void HideContent();

    protected:
        int CalculateLabelWidth() const;

        bool IsHidden(InstanceDataNode* node) const;
        void HandleChangeNotifyAttribute(PropertyAttributeReader& reader, InstanceDataNode* node);

        QHBoxLayout* m_mainLayout;
        QHBoxLayout* m_leftHandSideLayout;
        QHBoxLayout* m_middleLayout;
        QHBoxLayout* m_rightHandSideLayout;
        
        QPointer <QPushButton> m_dropDownArrow;
        QPointer <QPushButton> m_containerClearButton;
        QPointer <QPushButton> m_containerAddButton;
        QPointer <QPushButton> m_elementRemoveButton;

        QWidget* m_leftAreaContainer;

        ElidingLabel* m_nameLabel;
        QLabel* m_defaultLabel; // if there is no handler, we use a m_defaultLabel label
        InstanceDataNode* m_sourceNode;

        struct ChangeNotification
        {
            ChangeNotification(InstanceDataNode* node, AZ::Edit::Attribute* attribute)
                : m_attribute(attribute)
                , m_node(node)
            { }

            InstanceDataNode* m_node = nullptr;
            AZ::Edit::Attribute* m_attribute = nullptr;
        };

        AZStd::vector<ChangeNotification> m_changeNotifiers;
        QSpacerItem* m_indent;
        PropertyHandlerBase* m_handler; // the CURRENT handler.
        PropertyRowWidget* m_parentRow; // not the parent widget.
        AZStd::vector<PropertyRowWidget*> m_childrenRows; // children rows of this row.

        QWidget* m_childWidget;

        bool m_forbidExpansion;
        bool m_autoExpand;
        bool m_expanded;
        bool m_containerEditable;
        bool m_isContainer;
        bool m_initialized;
        bool m_isMultiSizeContainer;
        bool m_isFixedSizeOrSmartPtrContainer;

        bool m_isSelected;
        bool m_selectionEnabled;
        bool m_readOnly; //holds whether the ReadOnly attribute was set

        QElapsedTimer m_clickStartTimer;
        QPoint m_clickPos;

        int m_containerSize;
        int m_requestedLabelWidth;
        AZ::u32 m_identifier;
        AZ::u32 m_handlerName;
        AZStd::string m_defaultValueString;
        bool m_hadChildren;
        int m_treeDepth;

        QIcon m_iconOpen;
        QIcon m_iconClosed;

        void SetNameLabel(const char* text);


        /// Marks the field to be visualized as "overridden".
        void SetOverridden(bool overridden);

        void contextMenuEvent(QContextMenuEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

        void UpdateDropDownArrow();
        void UpdateDefaultLabel(InstanceDataNode* node);

        void createContainerButtons();

        void UpdateEnabledState();

    signals:
        void onExpandedOrContracted(InstanceDataNode* node, bool expanded, bool fromUserInteraction = false);
        void onRequestedContainerClear(InstanceDataNode* node);
        void onRequestedContainerElementRemove(InstanceDataNode* node);
        void onRequestedContainerAdd(InstanceDataNode* node);
        void onRequestedContextMenu(InstanceDataNode* node, const QPoint& point);
        void onRequestedSelection(InstanceDataNode* node);

    private slots:
        void OnClickedExpansionButton();

        void OnClickedAddElementButton();
        void OnClickedRemoveElementButton();
        void OnClickedClearContainerButton();
        void OnContextMenuRequested(const QPoint&);

    };
}

#endif