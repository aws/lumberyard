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
#include <MCore/Source/Array.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/Source/AnimGraphObject.h>
#include <Source/Editor/ObjectEditor.h>

#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

#include <QDialog>
#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>

QT_FORWARD_DECLARE_CLASS(QGridLayout)
QT_FORWARD_DECLARE_CLASS(QScrollArea)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class AttributesWindow;


    class PasteConditionsWindow
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(PasteConditionsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        PasteConditionsWindow(AttributesWindow* attributeWindow);
        virtual ~PasteConditionsWindow();
        bool GetIsConditionSelected(uint32 index) const;
    private:
        QPushButton*                mOKButton;
        QPushButton*                mCancelButton;
        MCore::Array<QCheckBox*>    mCheckboxes;
    };


    class AttributesWindow
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(AttributesWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT
        friend class AnimGraphPlugin;
        friend class AnimGraphEventHandler;

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

        void InitForAnimGraphObject(EMotionFX::AnimGraphObject* object);
        void Reinit();
        EMotionFX::AnimGraphObject* GetObject() const      { return mObject; }
        const MCore::Array<CopyPasteConditionObject>& GetCopyPasteConditionClipboard() const { return mCopyPasteClipboard; }

    public slots:
        void OnAddCondition();
        void OnRemoveCondition();
        void ReInitCurrentAnimGraphObject();
        void OnCopyConditions();
        void OnPasteConditions();
        void OnPasteConditionsSelective();

        void OnConditionContextMenu(const QPoint& position);

    private:
        void contextMenuEvent(QContextMenuEvent* event);

        AzQtComponents::Card* CreateAnimGraphCard(AZ::SerializeContext* serializeContext, EMotionFX::AnimGraph* animGraph);

        AnimGraphPlugin*                        mPlugin;
        QWidget*                                mMainWidget;
        QGridLayout*                            mGridLayout;
        QVBoxLayout*                            mMainLayout;
        QScrollArea*                            mScrollArea;
        EMotionFX::AnimGraphObject*             mObject;

        AzQtComponents::Card*                   m_objectCard;
        EMotionFX::ObjectEditor*                m_objectEditor;

        PasteConditionsWindow*                  mPasteConditionsWindow;
        
        // copy & paste conditions
        MCore::Array<CopyPasteConditionObject> mCopyPasteClipboard;

        void CreateConditionsGUI(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, QVBoxLayout* mainLayout);
        QIcon GetIconForObject(EMotionFX::AnimGraphObject* object);
    };
} // namespace EMStudio