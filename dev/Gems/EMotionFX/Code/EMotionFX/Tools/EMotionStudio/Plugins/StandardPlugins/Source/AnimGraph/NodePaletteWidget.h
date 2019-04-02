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

#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EventHandler.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>
#include <QListWidget>

QT_FORWARD_DECLARE_CLASS(QTabBar)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)


namespace EMStudio
{
    class AnimGraphPlugin;

    class NodePaletteList
        : public QListWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NodePaletteList, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
    public:
        enum
        {
            NODETYPE_STATE      = 0,
            NODETYPE_BLENDNODE  = 1
        };

        NodePaletteList(QWidget* parent = nullptr)
            : QListWidget(parent) {}
        ~NodePaletteList() {}

    protected:
        QMimeData* mimeData(const QList<QListWidgetItem*> items) const;
        QStringList mimeTypes() const;
        Qt::DropActions supportedDropActions() const;
    };



    class NodePaletteWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NodePaletteWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT // AUTOMOC

    public:
        class EventHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            EventHandler(NodePaletteWidget* widget);
            ~EventHandler() override = default;

            const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_CREATED_NODE, EMotionFX::EVENT_TYPE_ON_REMOVED_CHILD_NODE }; }
            void OnCreatedNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node) override;
            void OnRemovedChildNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* parentNode) override;

        private:
            NodePaletteWidget*  mWidget;
        };

        NodePaletteWidget(AnimGraphPlugin* plugin);
        ~NodePaletteWidget();

        void Init(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node);

        static AZStd::string GetNodeIconFileName(const EMotionFX::AnimGraphNode* node);

    private slots:
        void OnChangeCategoryTab(int index);
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);

    private:
        AZStd::vector<AZStd::pair<EMotionFX::AnimGraphNode::ECategory, QString>> m_categories;
        AnimGraphPlugin*            mPlugin;
        NodePaletteList*            mList;
        QTabBar*                    mTabBar;
        EMotionFX::AnimGraphNode*   mNode;
        EventHandler*               mEventHandler;
        QVBoxLayout*                mLayout;
        QLabel*                     mInitialText;

        void RegisterItems(EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category);
    };
}   // namespace EMStudio