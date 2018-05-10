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

#include "DataInterface.h"

class QAbstractItemModel;

namespace GraphCanvas
{
    class ItemModelDataInterface 
        : public DataInterface
    {
    public:
        // Returns the ListModel to be used with the QCompleter
        virtual QAbstractItemModel* GetItemModel() const = 0;
        
        // Signals that the user wants to assign the specified model index to the underlying data.
        virtual void AssignIndex(const QModelIndex& modelIndex) = 0;
        
        // Returns the string used as placeholder text in the selector widget
        virtual AZStd::string GetPlaceholderString() const = 0;
        
        // Returns the string used to display the currently selected value.
        virtual AZStd::string GetDisplayString() const = 0;

        // Returns whether the currently selected value is valid (i.e. should we use the display string)
        virtual bool IsItemValid() const = 0;

        // Returns the display name to use in association with the display model
        virtual QString GetModelName() const = 0;

        // Returns a widget that can be used to inspect the entire model is a more robus fashion.
        //
        // A nullptr will default back to whatever the underlying widget wants to use.
        virtual QWidget* InspectModel() const { return nullptr; };
    };
}