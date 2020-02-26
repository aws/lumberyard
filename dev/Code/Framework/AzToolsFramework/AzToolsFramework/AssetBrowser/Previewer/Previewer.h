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

#include <QWidget>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        //! A base class for Asset Browser previewer.
        //! To implement your custom previewer:
        //! 1. Derive your previewer widget from this class.
        //! 2. Implement custom PreviewerFactory.
        //! 3. Register PreviewerFactory with PreviewerRequestBus::RegisterFactory EBus.
        //! Note: if there are multiple factories handling same entry type, last one registered will be selected.
        class Previewer
            : public QWidget
        {
            Q_OBJECT
        public:
            Previewer(QWidget* parent = nullptr);

            //! Clear previewer
            virtual void Clear() const = 0;
            //! Display asset preview for specific asset browser entry
            virtual void Display(const AssetBrowserEntry* entry) = 0;
            //! Get name of the previewer (this should be unique to other previewers)
            virtual const QString& GetName() const = 0;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework