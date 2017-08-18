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

#ifndef CRYINCLUDE_EDITOR_CLOUD_CLOUDGROUPPANEL_H
#define CRYINCLUDE_EDITOR_CLOUD_CLOUDGROUPPANEL_H
#pragma once


#include <QWidget>
#include <QScopedPointer>

class CCloudGroup;
namespace Ui {
    class CCloudGroupPanel;
}


// CCloudGroupPanel dialog
class CCloudGroupPanel
    : public QWidget
{
public:
    CCloudGroupPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CCloudGroupPanel();

    // Dialog Data
    void Init(CCloudGroup* cloud);
    void FillBox(CBaseObject* pBox);
    void SetFileBrowseText();

protected:
    void OnCloudsGenerate();
    void OnCloudsImport();
    void OnCloudsExport();
    void OnCloudsBrowse();

private:
    CCloudGroup* m_cloud;
    QScopedPointer<Ui::CCloudGroupPanel> ui;
};

#endif // CRYINCLUDE_EDITOR_CLOUD_CLOUDGROUPPANEL_H
