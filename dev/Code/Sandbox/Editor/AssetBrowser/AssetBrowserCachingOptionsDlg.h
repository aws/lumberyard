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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERCACHINGOPTIONSDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERCACHINGOPTIONSDLG_H
#pragma once
#include "AssetBrowser/AssetBrowserManager.h"

// CAssetBrowserCachingOptionsDlg dialog

#include <QDialog>

namespace Ui
{
    class AssetBrowserCachingOptionsDlg;
}

class CAssetBrowserCachingOptionsDlg
    : public QDialog
{
    Q_OBJECT
public:
    CAssetBrowserCachingOptionsDlg(QWidget* parent = nullptr);   // standard constructor
    virtual ~CAssetBrowserCachingOptionsDlg();

    TAssetDatabases GetSelectedDatabases() const;
    bool IsForceCache() const;
    UINT GetThumbSize() const;

protected:
    bool m_bForceCache;
    UINT m_thumbSize;
    std::vector<struct IAssetItemDatabase*> m_databases, m_selectedDBs;

    void OnInitDialog();

public:
    void accept() override;

private:
    QScopedPointer<Ui::AssetBrowserCachingOptionsDlg> m_ui;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERCACHINGOPTIONSDLG_H
