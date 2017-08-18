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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_SOUNDASSETDBFILTERDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_SOUNDASSETDBFILTERDLG_H
#pragma once
#include "IAssetItemDatabase.h"
#include <QScopedPointer>
#include <QWidget>

// CSoundAssetDbFilterDlg dialog
namespace Ui {
    class SoundAssetDbFilterDlg;
}
class CSoundAssetDbFilterDlg
    : public QWidget
{
    Q_OBJECT
public:
    CSoundAssetDbFilterDlg(QWidget* pParent = NULL);   // standard constructor
    virtual ~CSoundAssetDbFilterDlg();

    void UpdateFilterUI();
    void ApplyFilter();
    void SetAssetViewer(struct IAssetViewer* pViewer)
    {
        m_pAssetViewer = pViewer;
    }

protected:
    struct IAssetViewer* m_pAssetViewer;

protected slots:
    void OnCbnSelchangeComboMinimumLength(int index);
    void OnCbnSelchangeComboMaximumLength(int index);
    void OnBnClickedCheckLoopingSounds();

private:
    QScopedPointer<Ui::SoundAssetDbFilterDlg> ui;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_SOUNDASSETDBFILTERDLG_H
