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

#include <QMimeData>

#include <GraphCanvas/Components/NodePropertyDisplay/DataInterface.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxModel.h>

namespace GraphCanvas
{
    class ComboBoxDataInterface
        : public DataInterface
    {
    public:
        // Returns the EnumModel used to populate the DropDown and AutoCompleter Menu
        virtual ComboBoxModelInterface* GetItemInterface() = 0;

        // Interfaces for manipulating the values. Indexes will refer to the elements within the ComboBoxModel
        virtual void AssignIndex(const QModelIndex& index) = 0;
        virtual QModelIndex GetAssignedIndex() const = 0;

        // Returns the string used to display the currently selected value[Used in the non-editable format]
        virtual const AZStd::string& GetDisplayString() const = 0;

        virtual void OnShowContextMenu(QWidget* nodePropertyDisplay, const QPoint& pos)
        {
            AZ_UNUSED(nodePropertyDisplay);
            AZ_UNUSED(pos);
        }

        virtual bool EnableDropHandling() const
        {
            return false;
        }

        virtual bool ShouldAcceptMimeData(const QMimeData* mimeData)
        {
            AZ_UNUSED(mimeData);
            return false;
        }

        virtual bool HandleMimeData(const QMimeData* mimeData)
        {
            AZ_UNUSED(mimeData);
            return false;
        }
    };
}