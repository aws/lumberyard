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
#include <QDialog>
#include <MysticQt/Source/SearchButton.h>
#include <MysticQt/Source/DialogStack.h>

QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)

//EMFX_FORWARD_DECLARE( Actor )


namespace EMStudio
{
    // forward declaration
    class SceneManagerPlugin;

    class RetargetSetupWindow
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(RetargetSetupWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        RetargetSetupWindow(QWidget* parent, SceneManagerPlugin* plugin);
        ~RetargetSetupWindow();

        void Reinit();

    public slots:
        void OnAddSourceActor();
        void OnRemoveSourceActor();
        void OnClearSourceActors();
        void OnLoadSourceActor();

    private:
        SceneManagerPlugin*     mPlugin         = nullptr;
        QPushButton*            mButtonAdd      = nullptr;
        QPushButton*            mButtonRemove   = nullptr;
        QPushButton*            mButtonClear    = nullptr;
        QTableWidget*           mTableWidget    = nullptr;
        MysticQt::DialogStack*  mDialogStack    = nullptr;
    };
} // namespace EMStudio

