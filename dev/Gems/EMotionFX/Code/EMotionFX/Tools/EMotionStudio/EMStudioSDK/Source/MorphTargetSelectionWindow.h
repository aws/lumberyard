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

#ifndef __EMSTUDIO_MORPHTARGETSELECTIONWINDOW_H
#define __EMSTUDIO_MORPHTARGETSELECTIONWINDOW_H

// include MCore
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/Source/MorphSetup.h>
#include "EMStudioConfig.h"
#include <QListWidget>
#include <QDialog>



namespace EMStudio
{
    class EMSTUDIO_API MorphTargetSelectionWindow
        : public QDialog
    {
        Q_OBJECT
                       MCORE_MEMORYOBJECTCATEGORY(MorphTargetSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MorphTargetSelectionWindow(QWidget* parent);
        virtual ~MorphTargetSelectionWindow();

        void Update(EMotionFX::MorphSetup* morphSetup, const MCore::Array<uint32>& selection);
        const MCore::Array<uint32>& GetMorphTargetIDs() const                                               { return mSelection; }

    public slots:
        void OnSelectionChanged();

    private:
        QListWidget*        mListWidget;
        QPushButton*        mOKButton;
        QPushButton*        mCancelButton;
        MCore::Array<uint32> mSelection;
    };
} // namespace EMStudio

#endif
