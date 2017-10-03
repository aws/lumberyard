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

#ifndef __EMSTUDIO_ATTRIBUTESWINDOW_H
#define __EMSTUDIO_ATTRIBUTESWINDOW_H

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/Source/AnimGraphObject.h>
#include <MysticQt/Source/AttributeWidgetFactory.h>
#include <MysticQt/Source/PropertyWidget.h>
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
            MCore::String   mAttributes;
            MCore::String   mSummary;
            uint32          mConditionType;
        };

        void InitForAnimGraphObject(EMotionFX::AnimGraphObject* object);
        void Reinit();
        EMotionFX::AnimGraphObject* GetObject() const      { return mObject; }
        const MCore::Array<CopyPasteConditionObject>& GetCopyPasteConditionClipboard() const { return mCopyPasteClipboard; }

        void UpdateAttributeWidgetStates();

        void OnAttributeChanged(MCore::Attribute* attribute, MCore::AttributeSettings* settings);

    public slots:
        void OnAddCondition();
        void OnRemoveCondition();
        void ReInitCurrentAnimGraphObject();
        void OnCopyConditions();
        void OnPasteConditions();
        void OnPasteConditionsSelective();

    private:
        void contextMenuEvent(QContextMenuEvent* event);

        struct AttributeLink
        {
            MysticQt::AttributeWidget*      mWidget;
            EMotionFX::AnimGraphObject*    mObject;
            uint32                          mAttributeIndex;
        };

        AnimGraphPlugin*                       mPlugin;
        MysticQt::PropertyWidget*               mAttributes;
        QWidget*                                mMainWidget;
        QGridLayout*                            mGridLayout;
        QVBoxLayout*                            mMainLayout;
        QScrollArea*                            mScrollArea;
        EMotionFX::AnimGraphObject*            mObject;
        PasteConditionsWindow*                  mPasteConditionsWindow;
        MCore::Array<AttributeLink>             mAttributeLinks;

        // copy & paste conditions
        MCore::Array<CopyPasteConditionObject> mCopyPasteClipboard;

        struct ButtonLookup
        {
            MCORE_MEMORYOBJECTCATEGORY(AttributesWindow::ButtonLookup, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
            QObject*    mButton;
            uint32      mIndex;
        };
        MCore::Array<ButtonLookup>              mRemoveButtonTable;

        void AddConditions(EMotionFX::AnimGraphObject* object, QVBoxLayout* mainLayout, bool readOnly);
        uint32 FindRemoveButtonIndex(QObject* button) const;
    };
} // namespace EMStudio


#endif
