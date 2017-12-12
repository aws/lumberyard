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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERMETATAGGINGDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERMETATAGGINGDLG_H
#pragma once
#include "AssetBrowserCommon.h"

#include <QDialog>
#include <QLineEdit>

class QLabel;
class QComboBox;

namespace Ui
{
    class AssetBrowserMetaTaggingDlg;
};

class CAutoCompleteEdit
    : public QLineEdit
{
public:
    CAutoCompleteEdit(QWidget* parent = nullptr);
    ~CAutoCompleteEdit();

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

// CAssetBrowserMetaTagging dialog

class CAssetBrowserMetaTaggingDlg
    : public QDialog
{
    Q_OBJECT
public:
    CAssetBrowserMetaTaggingDlg(TAssetItems* assetlist, QWidget* pParent = NULL);    // standard constructor
    CAssetBrowserMetaTaggingDlg(const QString& filename, QWidget* pParent = NULL);
    virtual ~CAssetBrowserMetaTaggingDlg();

// Dialog Data
protected:
    void OnInitDialog();

    void accept() override;
    void OnEditChanged();

    bool Validated();

private:
    std::vector<QLabel*> m_staticControls;
    std::vector<QComboBox*> m_comboControls;
    QStringList m_assetList;
    bool m_bAttemptAutocomplete;

    QScopedPointer<Ui::AssetBrowserMetaTaggingDlg> m_ui;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERMETATAGGINGDLG_H
