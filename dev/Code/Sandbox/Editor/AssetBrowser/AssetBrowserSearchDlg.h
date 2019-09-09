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

// Description : The dialog used for searching assets, contains the search editbox


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERSEARCHDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERSEARCHDLG_H
#pragma once
#include <vector>
#include "Include/IAssetItem.h"
#include "Include/IAssetItemDatabase.h"

#include <QLineEdit>

class CAssetBrowserDialog;

namespace Ui
{
    class AssetBrowserSearchDlg;
}

class CAssetBrowserSearchDlg;

class CSearchEditCtrl
    : public QLineEdit
{
    Q_OBJECT
public:
    bool m_bNotFound;

    CSearchEditCtrl(QWidget* parent = nullptr);

    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
};

class CAssetBrowserSearchDlg
    : public QWidget
{
    Q_OBJECT
public:
    enum AssetBrowserSearchTypes
    {
        AssetBrowserSearchTypes_Name,
        AssetBrowserSearchTypes_Tags,
        AssetBrowserSearchTypes_All
    };

    CAssetBrowserSearchDlg(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAssetBrowserSearchDlg();

    AssetBrowserSearchTypes GetSearchType() const
    {
        return m_assetSearchType;
    }

protected:
    void HandleDebugCommand(const QString& command);

public:
    CAssetBrowserDialog* m_pAssetBrowserDlg;
    AssetBrowserSearchTypes m_assetSearchType;
    IAssetItemDatabase::TAssetFields m_fields;
    bool m_bForceFilterUsedInLevel, m_bForceShowFavorites, m_bForceHideLods;
    bool m_bAllowOnlyFilenames;
    QString m_allowedFilenames;

    void OnInitDialog();
    void OnEnChangeEditSearchText();
    void Search();

private:
    QScopedPointer<Ui::AssetBrowserSearchDlg> m_ui;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERSEARCHDLG_H
