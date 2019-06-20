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

class QWidget;

namespace GraphCanvas
{
    class EntityIdDataInterface 
        : public DataInterface
    {
    public:	
		virtual AZ::EntityId GetEntityId() const = 0;
		virtual void SetEntityId(const AZ::EntityId& entityId) = 0;        
		
        virtual const AZStd::string GetNameOverride() const = 0;        
        virtual void OnShowContextMenu(QWidget* nodePropertyDisplay, const QPoint& pos) = 0;
    };
}