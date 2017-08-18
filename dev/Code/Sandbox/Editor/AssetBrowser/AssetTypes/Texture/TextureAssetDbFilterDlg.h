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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_TEXTUREASSETDBFILTERDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_TEXTUREASSETDBFILTERDLG_H
#pragma once

#include "IAssetItemDatabase.h"

#include <QScopedPointer>
#include <QWidget>

// CTextureAssetDbFilterDlg dialog
class QComboBox;

namespace Ui {
    class TextureAssetDbFilterDlg;
}
class CTextureAssetDbFilterDlg
    : public QWidget
{
    Q_OBJECT
public:
    CTextureAssetDbFilterDlg(QWidget* pParent = NULL);   // standard constructor
    virtual ~CTextureAssetDbFilterDlg();

    void SetAssetViewer(struct IAssetViewer* pViewer)
    {
        m_pAssetViewer = pViewer;
    }

    void UpdateFilterUI();
    void ApplyFilter();

protected:
    struct IAssetViewer* m_pAssetViewer;

public slots:
    void OnBnClickedOk();
    void OnBnClickedCancel();
    void OnCbnSelchangeComboMinimumWidth();
    void OnCbnSelchangeComboMaximumWidth();
    void OnCbnSelchangeComboMinimumHeight();
    void OnCbnSelchangeComboMaximumHeight();
    void OnCbnSelchangeComboMinimumMips();
    void OnCbnSelchangeComboMaximumMips();
    void OnCbnSelchangeComboTextureType();
    void OnCbnSelchangeComboTextureUsage();

private:
    enum COMBO_CHANGED
    {
        MIN, MAX
    };
    void SetComboToValidValue(QComboBox*, QComboBox*, COMBO_CHANGED);
    QScopedPointer<Ui::TextureAssetDbFilterDlg> ui;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_TEXTUREASSETDBFILTERDLG_H
