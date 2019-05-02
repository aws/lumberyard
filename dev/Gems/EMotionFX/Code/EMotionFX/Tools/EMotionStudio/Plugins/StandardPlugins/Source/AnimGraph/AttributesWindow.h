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

#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <Editor/TypeChoiceButton.h>

#include <QDialog>
#include <QModelIndex>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QScrollArea)
QT_FORWARD_DECLARE_CLASS(QItemSelection)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace AzQtComponents
{
    class Card;
}

namespace EMotionFX
{
    class AnimGraphEditor;
    class AnimGraphObject;
    class ObjectEditor;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class AttributesWindow;

    class AddConditionButton
        : public EMotionFX::TypeChoiceButton
    {
        Q_OBJECT //AUTOMOC

    public:
        AddConditionButton(AnimGraphPlugin* plugin, QWidget* parent);    
    };

    class AddActionButton
        : public EMotionFX::TypeChoiceButton
    {
        Q_OBJECT //AUTOMOC

    public:
        AddActionButton(AnimGraphPlugin* plugin, QWidget* parent);
    };

    class PasteConditionsWindow
        : public QDialog
    {
        Q_OBJECT //AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(PasteConditionsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        PasteConditionsWindow(AttributesWindow* attributeWindow);
        virtual ~PasteConditionsWindow();
        bool GetIsConditionSelected(size_t index) const;
    private:
        QPushButton*                mOKButton;
        QPushButton*                mCancelButton;
        AZStd::vector<QCheckBox*>   mCheckboxes;
    };


    class AttributesWindow
        : public QWidget
    {
        Q_OBJECT //AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(AttributesWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        AttributesWindow(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~AttributesWindow();

        // copy & paste conditions
        struct CopyPasteConditionObject
        {
            AZStd::string   mContents;
            AZStd::string   mSummary;
            AZ::TypeId      mConditionType;
        };

        const AZStd::vector<CopyPasteConditionObject>& GetCopyPasteConditionClipboard() const { return m_copyPasteClipboard; }

        void Init(const QModelIndex& modelIndex = QModelIndex(), bool forceUpdate = false);

    public slots:
        void OnCopyConditions();
        void OnPasteConditions();
        void OnPasteConditionsSelective();

    private slots:
        void AddCondition(const AZ::TypeId& conditionType);
        void OnRemoveCondition();
        void OnConditionContextMenu(const QPoint& position);

        void OnActionContextMenu(const QPoint& position);

        void AddTransitionAction(const AZ::TypeId& actionType);
        void AddStateAction(const AZ::TypeId& actionType);
        void OnRemoveTransitionAction();
        void OnRemoveStateAction();

        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

    private:
        void contextMenuEvent(QContextMenuEvent* event);

        AnimGraphPlugin*                        mPlugin;
        QScrollArea*                            mScrollArea;
        QPersistentModelIndex                   m_displayingModelIndex;

        QWidget*                                m_mainReflectedWidget;
        AzQtComponents::Card*                   m_objectCard;
        EMotionFX::AnimGraphEditor*             m_animGraphEditor;
        EMotionFX::ObjectEditor*                m_objectEditor;

        struct CachedWidgets
        {
            CachedWidgets(AzQtComponents::Card* card, EMotionFX::ObjectEditor* objectEditor)
                : m_card(card)
                , m_objectEditor(objectEditor)
            {}

            AzQtComponents::Card*    m_card;
            EMotionFX::ObjectEditor* m_objectEditor;
        };

        // Condition widgets
        QWidget*                                m_conditionsWidget;
        QLayout*                                m_conditionsLayout;
        AZStd::vector<CachedWidgets>            m_conditionsCachedWidgets;

        // Action widgets
        QWidget*                                m_actionsWidget;
        QLayout*                                m_actionsLayout;
        AZStd::vector<CachedWidgets>            m_actionsCachedWidgets;

        PasteConditionsWindow*                  mPasteConditionsWindow;
        
        // copy & paste conditions
        AZStd::vector<CopyPasteConditionObject> m_copyPasteClipboard;

        void UpdateConditions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate = false);
        void UpdateActions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate = false);
        QIcon GetIconForObject(EMotionFX::AnimGraphObject* object);
    };
} // namespace EMStudio