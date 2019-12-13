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

#pragma once

#include "QTreeWidgetFilter.h"
#include <QString>


//-----------------------------------------------------------------------------------------------//
struct SImplNameFilter
    : public ITreeWidgetItemFilter
{
    SImplNameFilter(const QString& filter = "");
    bool IsItemValid(QTreeWidgetItem* pItem) override;
    void SetFilter(QString filter);
    bool IsNameValid(const QString& name);

private:
    QString m_filter;
};

//-----------------------------------------------------------------------------------------------//
struct SImplTypeFilter
    : public ITreeWidgetItemFilter
{
    SImplTypeFilter()
        : m_nAllowedControlsMask(std::numeric_limits<uint>::max()) {}
    bool IsItemValid(QTreeWidgetItem* pItem) override;
    void SetAllowedControlsMask(uint nAllowedControlsMask) { m_nAllowedControlsMask = nAllowedControlsMask; }
    uint m_nAllowedControlsMask;
};

//-----------------------------------------------------------------------------------------------//
struct SHideConnectedFilter
    : public ITreeWidgetItemFilter
{
    SHideConnectedFilter();
    bool IsItemValid(QTreeWidgetItem* pItem) override;
    void SetHideConnected(bool bHideConnected);

private:
    bool m_bHideConnected;
};
