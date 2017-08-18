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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_SEQUENCEBROWSER_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_SEQUENCEBROWSER_H
#pragma once


#include <QDockWidget>
#include <QScopedPointer>
#include <Mannequin/Controls/ui_SequenceBrowser.h>

class CFolderTreeCtrl;
class CPreviewerPage;
class QTreeWidgetItem;

namespace Ui
{
    class CSequenceBrowser;
}

class CSequenceBrowser
    : public QWidget
{
    Q_OBJECT
public:
    CSequenceBrowser(CPreviewerPage& previewerPage, QWidget* parent = nullptr);
    virtual ~CSequenceBrowser() {};

private slots:
    void OnTreeDoubleClicked();
    void OnOpen();

private:
    CFolderTreeCtrl* m_pTree;
    CPreviewerPage& m_previewerPage;
    QScopedPointer<Ui::CSequenceBrowser> ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_SEQUENCEBROWSER_H
