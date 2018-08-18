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

// include MCore
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <MysticQt/Source/IntSpinbox.h>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    class CollisionMeshesNodeHierarchyWidget
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(CollisionMeshesNodeHierarchyWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS)

    public:
        CollisionMeshesNodeHierarchyWidget(QWidget* parent);
        virtual ~CollisionMeshesNodeHierarchyWidget();

        void Update(uint32 actorInstanceID);
        void Update();

        MCORE_INLINE QTreeWidget* GetTreeWidget()                                                               { return mHierarchy; }
        MCORE_INLINE AzQtComponents::FilteredSearchWidget* GetSearchWidget()                                    { return m_searchWidget; }
        MCORE_INLINE const AZStd::string& GetSearchWidgetText() const                                           { return m_searchWidgetText; }
        MCORE_INLINE uint32 GetActorInstanceID()                                                                { return mActorInstanceID; }
        MCORE_INLINE MCore::Array<MCore::Array<AZStd::string> >& GetLODNodeList()                               { return mLODNodeList; }

        bool CheckIfNodeVisible(const AZStd::string& nodeName, bool isMeshNode);

    public slots:
        void UpdateSelection();
        void TreeContextMenu(const QPoint& pos);
        void OnTextFilterChanged(const QString& text);
        void LODSpinBoxValueChanged(int value);

    private:
        void RecursiveRemoveUnselectedItems(QTreeWidgetItem* item);
        void RecursivelyAddChilds(QTreeWidgetItem* parent, EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        void AddActorInstance(EMotionFX::ActorInstance* actorInstance);

    private:
        QTreeWidget*                                mHierarchy;
        MysticQt::IntSpinBox*                       mLODSpinBox;
        AzQtComponents::FilteredSearchWidget*       m_searchWidget;
        AZStd::string                               m_searchWidgetText;
        QIcon*                                      mMeshIcon;
        QIcon*                                      mCharacterIcon;
        uint32                                      mActorInstanceID;
        AZStd::string                               mTempString;
        MCore::Array<MCore::Array<AZStd::string> >   mLODNodeList;
        bool                                        mRootSelected;
    };
} // namespace EMStudio
