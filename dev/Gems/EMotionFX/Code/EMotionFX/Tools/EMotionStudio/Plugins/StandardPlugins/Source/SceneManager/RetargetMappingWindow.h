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
#include <QWidget>
#include <MysticQt/Source/SearchButton.h>

QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)

EMFX_FORWARD_DECLARE(Actor)


namespace EMStudio
{
    // forward declaration
    class ActorSetupPlugin;

    class RetargetMappingWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(RetargetMappingWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        RetargetMappingWindow(QWidget* parent, ActorSetupPlugin* plugin);
        ~RetargetMappingWindow();

        void Reinit(bool reInitMap = true);

    public slots:
        void OnCurrentListDoubleClicked(QTableWidgetItem* item);
        void OnMappingTableDoubleClicked(QTableWidgetItem* item);
        void OnMappingTableSelectionChanged();
        void OnCurrentListSelectionChanged();
        void OnSourceListSelectionChanged();
        void CurrentFilterStringChanged(const QString& text);
        void SourceFilterStringChanged(const QString& text);
        void OnLoadSourceActor();
        void OnLinkPressed();
        void OnLoadMapping();
        void OnSaveMapping();
        void OnClearMapping();
        void OnBestGuess();

    private:
        ActorSetupPlugin*       mPlugin;
        QTableWidget*           mSourceList;
        QTableWidget*           mCurrentList;
        QTableWidget*           mMappingTable;
        QPushButton*            mButtonOpen;
        QPushButton*            mButtonSave;
        QPushButton*            mButtonClear;
        QPushButton*            mButtonGuess;
        MysticQt::SearchButton* mCurrentSearchButton;
        MysticQt::SearchButton* mSourceSearchButton;
        EMotionFX::Actor*       mSourceActor;
        QIcon*                  mBoneIcon;
        QIcon*                  mNodeIcon;
        QIcon*                  mMeshIcon;
        QIcon*                  mMappedIcon;
        MCore::Array<uint32>    mCurrentBoneList;
        MCore::Array<uint32>    mSourceBoneList;
        MCore::Array<uint32>    mMap;

        void FillCurrentListWidget(EMotionFX::Actor* actor, const QString& filterString);
        void FillSourceListWidget(EMotionFX::Actor* actor, const QString& filterString);
        void FillMappingTable(EMotionFX::Actor* currentActor, EMotionFX::Actor* sourceActor);
        void PerformMapping(uint32 currentNodeIndex, uint32 sourceNodeIndex);
        void RemoveCurrentSelectedMapping();
        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
        bool CheckIfIsMapEmpty() const;
        void UpdateToolBar();
    };
} // namespace EMStudio

