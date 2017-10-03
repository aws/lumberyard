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
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <MysticQt/Source/SearchButton.h>
#include <MysticQt/Source/IntSpinbox.h>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)


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
        MCORE_INLINE MysticQt::SearchButton* GetSearchButton()                                                  { return mFindWidget; }
        MCORE_INLINE MCore::String GetFilterString()                                                            { return FromQtString(mFindWidget->GetSearchEdit()->text()); }
        MCORE_INLINE uint32 GetActorInstanceID()                                                                { return mActorInstanceID; }
        MCORE_INLINE MCore::Array<MCore::Array<MCore::String> >& GetLODNodeList()                                { return mLODNodeList; }

        bool CheckIfNodeVisible(const MCore::String& nodeName, bool isMeshNode);

    public slots:
        void UpdateSelection();
        void TreeContextMenu(const QPoint& pos);
        void TextChanged(const QString& text);
        void LODSpinBoxValueChanged(int value);

    private:
        void RecursiveRemoveUnselectedItems(QTreeWidgetItem* item);
        void RecursivelyAddChilds(QTreeWidgetItem* parent, EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        void AddActorInstance(EMotionFX::ActorInstance* actorInstance);

    private:
        QTreeWidget*                                mHierarchy;
        MysticQt::IntSpinBox*                       mLODSpinBox;
        MysticQt::SearchButton*                     mFindWidget;
        QIcon*                                      mMeshIcon;
        QIcon*                                      mCharacterIcon;
        MCore::String                               mFindString;
        uint32                                      mActorInstanceID;
        MCore::String                               mTempString;
        MCore::Array<MCore::Array<MCore::String> >   mLODNodeList;
        bool                                        mRootSelected;
    };
} // namespace EMStudio
