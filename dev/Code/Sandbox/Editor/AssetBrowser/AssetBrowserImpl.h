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
#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_INTERFACE_ASSETBROWSER_IMPL_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_INTERFACE_ASSETBROWSER_IMPL_H
#pragma once

#include <Include/IAssetBrowser.h>

class CAssetBrowserImpl
    : public IAssetBrowser
{
public:
    CAssetBrowserImpl();
    virtual ~CAssetBrowserImpl();

    void Open(const char* pFilename = "", const char* pAssetDbName = "") override;

    QString GetFirstSelectedFilename() override;

    virtual bool isAvailable() override;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_INTERFACE_ASSETBROWSER_IMPL_H
