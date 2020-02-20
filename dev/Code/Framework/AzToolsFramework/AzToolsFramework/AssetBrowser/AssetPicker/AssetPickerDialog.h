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
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <QDialog>
#include <QScopedPointer>

class QKeyEvent;
class QModelIndex;
class QItemSelection;

namespace Ui
{
    class AzAssetBrowserDialogClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class AssetBrowserFilterModel;
        class AssetBrowserModel;
        class AssetSelectionModel;
    }

    class QWidgetSavedState;
}

class SANDBOX_API AzAssetBrowserDialog
    : public QDialog
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(AzAssetBrowserDialog, AZ::SystemAllocator, 0);

    explicit AzAssetBrowserDialog(AzToolsFramework::AssetBrowser::AssetSelectionModel& selection, QWidget* parent = nullptr);
    virtual ~AzAssetBrowserDialog();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void reject() override;

private:
    QScopedPointer<Ui::AzAssetBrowserDialogClass> m_ui;
    AzToolsFramework::AssetBrowser::AssetBrowserModel* m_assetBrowserModel;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
    AzToolsFramework::AssetBrowser::AssetSelectionModel& m_selection;

    //! Evaluate whether current selection is valid
    /*!
        Valid selection requires exactly one item to be selected,
        must be source or product type,
        and must match the wildcard filter
    */
    bool EvaluateSelection() const;

    void UpdatePreview() const;

private Q_SLOTS:
    void DoubleClickedSlot(const QModelIndex& index);
    void SelectionChangedSlot();
    void RestoreState();

    void OnFilterUpdated();

private:

    bool m_hasFilter;
    AZStd::unique_ptr<AzToolsFramework::TreeViewState> m_filterStateSaver;
    AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> m_persistentState;
};
