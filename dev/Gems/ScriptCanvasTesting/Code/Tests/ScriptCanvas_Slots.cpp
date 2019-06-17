/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>

#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Slot.h>

using namespace ScriptCanvasTests;
using namespace TestNodes;

TEST_F(ScriptCanvasTestFixture, TypeMatching_NumericType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Number();

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(ScriptCanvas::Data::Type::Number()));

        for (auto type : GetTypes())
        {
            if (type == ScriptCanvas::Data::Type::Number())
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_RandomizedFixedType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomPrimitiveType();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dataType = randomType;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_FixedBehaviorObject)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dataType = m_dataSlotConfigurationType;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(m_dataSlotConfigurationType));

        for (auto type : GetTypes())
        {
            if (type == m_dataSlotConfigurationType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_RandomizedFixedBehaviorObject)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomObjectType();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dataType = randomType;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_FixedContainer)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dataType = m_entityIdSetType;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(m_entityIdSetType));

        for (auto type : GetTypes())
        {
            if (type == m_entityIdSetType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_RandomizedFixedContainer)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomContainerType();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dataType = randomType;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicSlotCreation_NoDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        for (auto dynamicDataType : { DynamicDataType::Any, DynamicDataType::Value, DynamicDataType::Container })
        {
            DataSlotConfiguration dataSlotConfiguration;

            dataSlotConfiguration.m_name = GenerateSlotName();
            dataSlotConfiguration.m_slotType = slotType;
            dataSlotConfiguration.m_dynamicDataType = dynamicDataType;

            Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);
            EXPECT_FALSE(slot->HasDisplayType());
            EXPECT_TRUE(slot->IsDynamicSlot());
            EXPECT_TRUE(slot->GetDynamicDataType() == dynamicDataType);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicSlotCreation_WithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        for (auto dynamicDataType : { DynamicDataType::Any, DynamicDataType::Value, DynamicDataType::Container })
        {
            DataSlotConfiguration dataSlotConfiguration;

            dataSlotConfiguration.m_name = GenerateSlotName();
            dataSlotConfiguration.m_slotType = slotType;
            dataSlotConfiguration.m_dynamicDataType = dynamicDataType;

            ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::Type::Invalid();

            if (dynamicDataType == DynamicDataType::Any)
            {
                dataType = GetRandomType();
            }
            else if (dynamicDataType == DynamicDataType::Value)
            {
                if (rand() % 2 == 0)
                {
                    dataType = GetRandomPrimitiveType();
                }
                else
                {
                    dataType = GetRandomObjectType();
                }
            }
            else if (dynamicDataType == DynamicDataType::Container)
            {
                dataType = GetRandomContainerType();
            }

            dataSlotConfiguration.m_displayType = dataType;

            Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);
            
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_TRUE(slot->IsDynamicSlot());
            EXPECT_TRUE(slot->GetDynamicDataType() == dynamicDataType);
            EXPECT_TRUE(slot->GetDataType() == dataType);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicTypingDisplayType_Any)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();    

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);
        EXPECT_FALSE(slot->HasDisplayType());

        for (auto type : GetTypes())
        {
            slot->SetDisplayType(type);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), type);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicTypingDisplayType_Value)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_FALSE(slot->HasDisplayType());

        for (auto primitiveType : GetPrimitiveTypes())
        {
            slot->SetDisplayType(primitiveType);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), primitiveType);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto containerType : GetContainerDataTypes())
        {
            slot->SetDisplayType(containerType);
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            slot->SetDisplayType(objectType);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), objectType);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicTypingDisplayType_Container)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        EXPECT_FALSE(slot->HasDisplayType());

        for (auto primitiveType : GetPrimitiveTypes())
        {
            slot->SetDisplayType(primitiveType);
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto containerType : GetContainerDataTypes())
        {
            slot->SetDisplayType(containerType);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), containerType);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            slot->SetDisplayType(objectType);
            EXPECT_FALSE(slot->HasDisplayType());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_Any)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        for (auto type : GetTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_AnyWithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        slot->SetDisplayType(ScriptCanvas::Data::Type::Number());
        
        EXPECT_TRUE(slot->IsTypeMatchFor(ScriptCanvas::Data::Type::Number()));

        for (auto type : GetTypes())
        {
            if (type == ScriptCanvas::Data::Type::Number())
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_AnyWithRandomizedDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomType();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        slot->SetDisplayType(randomType);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicValue)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        for (auto primitiveType : GetPrimitiveTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(primitiveType));
        }

        for (auto containerType : GetContainerDataTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicValueWithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        slot->SetDisplayType(ScriptCanvas::Data::Type::EntityID());

        EXPECT_TRUE(slot->IsTypeMatchFor(ScriptCanvas::Data::Type::EntityID()));
        
        for (auto primitiveType : GetPrimitiveTypes())
        {
            if (primitiveType == ScriptCanvas::Data::Type::EntityID())
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(primitiveType));
        }

        for (auto containerType : GetContainerDataTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicContainer)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        for (auto primitiveType : GetPrimitiveTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(primitiveType));
        }

        for (auto containerType : GetContainerDataTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicContainerWithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableNode* emptyNode = CreateConfigurableNode();

    for (auto slotType : { SlotType::DataIn, SlotType::DataOut })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = slotType;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* slot = emptyNode->AddTestingDataSlot(dataSlotConfiguration);

        slot->SetDisplayType(m_stringToNumberMapType);

        for (auto primitiveType : GetPrimitiveTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(primitiveType));
        }

        EXPECT_TRUE(slot->IsTypeMatchFor(m_stringToNumberMapType));

        for (auto containerType : GetContainerDataTypes())
        {
            if (containerType == m_stringToNumberMapType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToFixedPrimitiveSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Number();

        sourceSlot = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* validTargetSlot = nullptr;
    Slot* invalidTargetSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Number();

        validTargetSlot = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Boolean();

        invalidTargetSlot = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*validTargetSlot)));
    EXPECT_TRUE(validTargetSlot->IsTypeMatchFor((*sourceSlot)));

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*invalidTargetSlot)));
    EXPECT_FALSE(invalidTargetSlot->IsTypeMatchFor((*sourceSlot)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedObjectSlotToFixedObjectSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dataType = m_dataSlotConfigurationType;

        sourceSlot = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* validTargetSlot = nullptr;
    Slot* invalidTargetSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dataType = m_dataSlotConfigurationType;

        validTargetSlot = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Boolean();

        invalidTargetSlot = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*validTargetSlot)));
    EXPECT_TRUE(validTargetSlot->IsTypeMatchFor((*sourceSlot)));

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*invalidTargetSlot)));
    EXPECT_FALSE(invalidTargetSlot->IsTypeMatchFor((*sourceSlot)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedContainerSlotToFixedContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dataType = m_stringToNumberMapType;

        sourceSlot = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* validTargetSlot = nullptr;
    Slot* invalidTargetSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dataType = m_stringToNumberMapType;

        validTargetSlot = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dataType = m_entityIdSetType;

        invalidTargetSlot = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*validTargetSlot)));
    EXPECT_TRUE(validTargetSlot->IsTypeMatchFor((*sourceSlot)));

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*invalidTargetSlot)));
    EXPECT_FALSE(invalidTargetSlot->IsTypeMatchFor((*sourceSlot)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToDynamicAnySlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Number();

        sourceSlot = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;    

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    for (auto type : GetTypes())
    {
        if (type == ScriptCanvas::Data::Type::Number())
        {
            continue;
        }

        dynamicTarget->SetDisplayType(type);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToDynamicValueSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Number();

        sourceSlot = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    for (auto primitiveType : GetPrimitiveTypes())
    {
        if (primitiveType == ScriptCanvas::Data::Type::Number())
        {
            continue;
        }

        dynamicTarget->SetDisplayType(primitiveType);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }

    for (auto objectType : GetBehaviorObjectTypes())
    {
        dynamicTarget->SetDisplayType(objectType);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dataType = ScriptCanvas::Data::Type::Number();

        sourceSlot = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    for (auto containerType : GetContainerDataTypes())
    {
        dynamicTarget->SetDisplayType(containerType);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicAnySlotToDynamicValueSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        dynamicSource = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    // Any : Value
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
    
    // Any[Number] : Value
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Number] : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Boolean] : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Boolean());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Display Container] : Dynamic Value
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
    
    // Any[Display Container] : Value[Display Object]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_dataSlotConfigurationType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicAnySlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        dynamicSource = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    // Any : Container
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Number] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Vector<Number>] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Vector<Number>] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Set<EntityId>] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_entityIdSetType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Object] : Container[Map<String,Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_stringToNumberMapType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicValueSlotToDynamicValueSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicSource = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    // Value : Value
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Value
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());    

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Object] : Value[Object]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_dataSlotConfigurationType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Value[Boolean]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Boolean());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Object] : Value[Boolean]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Boolean());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicValueSlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicSource = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    // Value : Container
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Object] : Container[Map<String,Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_stringToNumberMapType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicContainerSlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicSource = sourceNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    ConfigurableNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.m_slotType = SlotType::DataOut;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingDataSlot(dataSlotConfiguration);
    }

    // Container : Container
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container[Vector<Number>] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container[Vector<Number>] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container[Vector<Number>] : Container[Map<String,Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_stringToNumberMapType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}