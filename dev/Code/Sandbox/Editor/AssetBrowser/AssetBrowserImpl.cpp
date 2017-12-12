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
#include "StdAfx.h"
#include "AssetBrowserImpl.h"

#include "AssetBrowser/AssetBrowserDialog.h"

CAssetBrowserImpl::CAssetBrowserImpl()
{
}

CAssetBrowserImpl::~CAssetBrowserImpl()
{
}

void CAssetBrowserImpl::Open(const char* pFilename, const char* pAssetDbName)
{
    CAssetBrowserDialog::Open(pFilename, pAssetDbName);
}

QString CAssetBrowserImpl::GetFirstSelectedFilename()
{
    assert(isAvailable());
    if (!isAvailable())
    {
        return "";
    }

    return CAssetBrowserDialog::Instance()->GetFirstSelectedFilename();
}

bool CAssetBrowserImpl::isAvailable()
{
    return CAssetBrowserDialog::Instance() != NULL;
}