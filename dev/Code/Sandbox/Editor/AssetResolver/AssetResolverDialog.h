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

#ifndef CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVERDIALOG_H
#define CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVERDIALOG_H
#pragma once


#include "AssetResolverReport.h"

#include <QWidget>

class QTreeView;
class MissingAssetModel;

class CMissingAssetDialog
    : public QWidget
{
    Q_OBJECT
public:
    explicit CMissingAssetDialog(QWidget* parent = nullptr);   // standard constructor
    virtual ~CMissingAssetDialog();

    static void RegisterViewClass();

    static void Open();
    static void Close();
    static void Update();

    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {52977ea0-8791-4840-9af4-94bc9646bdb3}
        static const GUID guid =
        {
            0x52977EA0, 0x8791, 0x4840, { 0x9A, 0xF4, 0x94, 0xBC, 0x96, 0x46, 0xBD, 0xB3 }
        };
        return guid;
    }

protected:
    void OnReportItemRClick();
    void OnReportItemDblClick(const QModelIndex& index);

private:
    void Clear();
    void UpdateReport();
    CMissingAssetReport* GetReport();

    typedef std::vector<CMissingAssetRecord*> TRecords;
    void GetSelectedRecords(TRecords& records);

    void AcceptRecort(CMissingAssetRecord* pRecord, int idx = 0);
    void CancelRecort(CMissingAssetRecord* pRecord);
    void ResolveRecord(CMissingAssetRecord* pRecord);

private:
    static CMissingAssetDialog* m_instance;

    QTreeView* m_treeView;
    MissingAssetModel* m_model;
};

#endif // CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVERDIALOG_H
