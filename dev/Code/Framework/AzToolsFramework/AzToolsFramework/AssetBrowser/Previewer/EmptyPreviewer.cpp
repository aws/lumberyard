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

#include <AzCore/base.h>
#include <AzToolsFramework/AssetBrowser/Previewer/EmptyPreviewer.h>
#include "AssetBrowser/Previewer/ui_EmptyPreviewer.h"

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const QString EmptyPreviewer::Name{ QStringLiteral("EmptyPreviewer") };

        EmptyPreviewer::EmptyPreviewer(QWidget* parent)
            : Previewer(parent)
            , m_ui(new Ui::EmptyPreviewerClass())
        {
            m_ui->setupUi(this);
        }

        void EmptyPreviewer::Display(const AssetBrowserEntry* entry)
        {
            AZ_UNUSED(entry);
        }

        const QString& EmptyPreviewer::GetName() const
        {
            return Name;
        }
    }
}

#include <AssetBrowser/Previewer/EmptyPreviewer.moc>