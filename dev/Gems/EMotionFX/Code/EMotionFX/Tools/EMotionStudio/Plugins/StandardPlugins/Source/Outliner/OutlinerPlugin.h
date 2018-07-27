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

#include <MCore/Source/Array.h>
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/OutlinerManager.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"

QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QListWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    class OutlinerPlugin
        : public DockWidgetPlugin
        , public OutlinerCallback
    {
        Q_OBJECT
                MCORE_MEMORYOBJECTCATEGORY(OutlinerPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        // class ID
        enum
        {
            CLASS_ID = 0x00000038
        };

        // constructor/destructor
        OutlinerPlugin();
        ~OutlinerPlugin();

        // overloaded
        bool Init() override;

        // overloaded
        const char* GetCompileDate() const override     { return MCORE_DATE; }
        const char* GetName() const override            { return "Outliner"; }
        uint32 GetClassID() const override              { return CLASS_ID; }
        const char* GetCreatorName() const override     { return "MysticGD"; }
        float GetVersion() const override               { return 1.0f; }
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }
        EMStudioPlugin* Clone() override                { return new OutlinerPlugin(); }
        QSize GetInitialWindowSize() const override     { return QSize(480, 650); }

    private slots:
        void OnCategoryItemSelectionChanged();
        void OnTextFilterChanged(const QString& text);
        void OnCategoryTreeContextMenu(const QPoint& pos);
        void OnCategoryItemListContextMenu(const QPoint& pos);
        void OnCategoryItemTableContextMenu(const QPoint& pos);
        void OnItemTableSelectionChanged();
        void OnItemListSelectionChanged();
        void OnListMode();
        void OnIconMode();

    private:
        void OnAddItem(OutlinerCategoryItem* item) override;
        void OnRemoveItem(OutlinerCategoryItem* item) override;
        void OnItemModified() override;
        void OnRegisterCategory(OutlinerCategory* category) override;
        void OnUnregisterCategory(const QString& name) override;
        void UpdateViewerAndRestoreOldSelection();
        void UpdateViewer();

    private:
        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        AZStd::string           m_searchWidgetText;
        QTreeWidgetItem*        mCategoryRootItem;
        QTreeWidget*            mCategoryTreeWidget;
        QTableWidget*           mViewerTableWidget;
        QListWidget*            mViewerListWidget;
        QLabel*                 mNumItemsLabel;
        bool                    mIconViewMode;
    };
} // namespace EMStudio