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

#include <AzCore/Math/Color.h>

#include <GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasColorDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::VectorDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasColorDataInterface, AZ::SystemAllocator, 0);
        ScriptCanvasColorDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasColorDataInterface() = default;
        
        int GetElementCount() const override
        {
            return 4;
        }
        
        double GetValue(int index) const override
        {
            if (index < GetElementCount())
            {
                const ScriptCanvas::Datum* object = GetSlotObject();

                if (object)
                {
                    const AZ::Color* retVal = object->GetAs<AZ::Color>();

                    if (retVal)
                    {
                        AZ::VectorFloat resultValue = AZ::VectorFloat(0);
                        
                        switch (index)
                        {
                        case 0:
                            resultValue = static_cast<int>(static_cast<float>(retVal->GetR()) * GetMaximum(index));
                            break;
                        case 1:
                            resultValue = static_cast<int>(static_cast<float>(retVal->GetG()) * GetMaximum(index));
                            break;
                        case 2:
                            resultValue = static_cast<int>(static_cast<float>(retVal->GetB()) * GetMaximum(index));
                            break;
                        case 3:
                            resultValue = static_cast<int>(static_cast<float>(retVal->GetA()) * GetMaximum(index));
                            break;
                        default:
                            break;
                        }
                        
                        return aznumeric_cast<double>(static_cast<float>(resultValue));
                    }
                }
            }
            
            return 0.0;
        }

        void SetValue(int index, double value) override
        {
            if (index < GetElementCount())
            {
                ScriptCanvas::Datum* object = GetSlotObject();

                if (object)
                {
                    AZ::Color* currentType = const_cast<AZ::Color*>(object->GetAs<AZ::Color>());
                    
                    switch (index)
                    {
                    case 0:
                        currentType->SetR(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    case 1:
                        currentType->SetG(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    case 2:
                        currentType->SetB(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    case 3:
                        currentType->SetA(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    default:
                        break;
                    }
                    
                    PostUndoPoint();
                    PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                }
            }
        }        
        
        const char* GetLabel(int index) const override
        {
            switch (index)
            {
            case 0:
                return "R";
            case 1:
                return "G";
            case 2:
                return "B";
            case 3:
                return "A";
            default:
                return "???";
            }
        }
        
        AZStd::string GetStyle() const override
        {
            return "vectorized";
        }
        
        virtual AZStd::string GetElementStyle(int index) const
        {
            return AZStd::string::format("vector_%i", index);
        }

        int GetDecimalPlaces(int index) const override
        {
            return 0;
        }
        
        double GetMinimum(int index) const override
        {
            return 0.0;
        }
        
        double GetMaximum(int index) const override
        {
            return 255.0;
        }
    };
}