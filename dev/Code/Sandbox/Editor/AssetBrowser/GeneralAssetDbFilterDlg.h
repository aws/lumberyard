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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_GENERALASSETDBFILTERDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_GENERALASSETDBFILTERDLG_H
#pragma once
#include "IAssetItemDatabase.h"

#include <QWidget>
#include <QScopedPointer>

namespace Ui {
    class GeneralAssetDbFilterDlg;
}
class CGeneralAssetDbFilterDlg
    : public QWidget
{
    Q_OBJECT
public:
    typedef std::map<QString/*preset name*/, SFieldFiltersPreset> TPresetNamePresetMap;

    CGeneralAssetDbFilterDlg(QWidget* pParent = NULL);   // standard constructor
    virtual ~CGeneralAssetDbFilterDlg();

    void UpdateFilterUI();
    void ApplyFilter();
    void SetAssetViewer(struct IAssetViewer* pViewer)
    {
        m_pAssetViewer = pViewer;
    }

    bool LoadFilterPresets();
    bool SaveFilterPresets();
    void SaveCurrentPreset();
    void FillPresetList();
    void FillDatabases();
    bool LoadFilterPresetsTo(TPresetNamePresetMap& rOutFilters, bool bClearMap = true);
    bool SaveFilterPresetsFrom(TPresetNamePresetMap& rFilters);
    bool AddFilterPreset(const char* pPresetName);
    bool UpdateFilterPreset(const char* pPresetName);
    bool DeleteFilterPreset(const char* pPresetName);
    SFieldFiltersPreset* GetFilterPresetByName(const char* pPresetName);
    void SelectPresetByName(const char* pPresetName);
    void UpdateVisibleDatabases();
    void SelectDatabase(const char* pDbName);

public slots:
    void OnCbnSelchangeComboPreset();
    void OnBnClickedButtonSavePreset();
    void OnBnClickedButtonRemovePreset();
    void OnBnClickedCheckUsedInLevel();
    void OnCbnSelchangeComboMinimumFilesize(int);
    void OnCbnSelchangeComboMaximumFilesize(int);
    void OnLvnItemchangedListDatabases();
    void OnBnClickedButtonUpdateUsedInLevel();

protected:
    struct IAssetViewer* m_pAssetViewer;
    TPresetNamePresetMap m_filterPresets;
    QString m_currentPresetName;

    QScopedPointer<Ui::GeneralAssetDbFilterDlg> ui;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_GENERALASSETDBFILTERDLG_H
