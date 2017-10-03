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

namespace AZ
{
    namespace LegacyConversion
    {
        namespace Util
        {
            // is the object that kind of class?
            static bool IsClassOf(CBaseObject* check, const char* className)
            {
                CObjectClassDesc* classDesc = check->GetClassDesc();
                if (!classDesc)
                {
                    return false;
                }

                return (GetIEditor()->GetClassFactory()->FindClass(className) == classDesc);
            }

            // map from CryEngine ESystemConfigSpec to LY EngineSpec enum
            static EngineSpec SpecConversion(ESystemConfigSpec spec)
            {
                switch (spec)
                {
                case CONFIG_LOW_SPEC:
                    return EngineSpec::Low;
                case CONFIG_MEDIUM_SPEC:
                    return EngineSpec::Medium;
                case CONFIG_HIGH_SPEC:
                    return EngineSpec::High;
                case CONFIG_VERYHIGH_SPEC:
                    return EngineSpec::VeryHigh;
                case CONFIG_AUTO_SPEC:
                    return EngineSpec::Never;
                default:
                    AZ_Error("Legacy Conversion", false, "Unhandled ESystemConfigSpec: %d", spec);
                    return EngineSpec::Never;
                }
            }

            // recursive debug function to print all properties of a particular VariableContainer (example: CVarBlock)
            static void DebugPrintVariableContainer(const char* typeName, const IVariableContainer* varContainer)
            {
                for (size_t i = 0; i < varContainer->GetNumVariables(); ++i)
                {
                    auto variable = varContainer->GetVariable(i);

                    if (variable->GetNumVariables() == 0)
                    {
                        IVariable* iVariable = static_cast<IVariable*>(variable);
                        AZ_Printf("%s Conversion Properties", "%s Var - Human Name: %s, Dev Name: %s, Type: %d, DataType: %d", typeName, variable->GetHumanName().toUtf8().constData(), variable->GetName().toUtf8().constData(), iVariable->GetType(), iVariable->GetDataType());
                    }
                    else
                    {
                        AZ_Printf("%s Conversion Properties", "%s Group: %s", typeName, variable->GetHumanName().toUtf8().constData());
                        DebugPrintVariableContainer(typeName, varContainer->GetVariable(i));
                    }
                }
            }

            // recursive debug print function to visualize InstanceDataHierarchy for a all fields on a component
            static void DebugPrintInstanceDataHierarchy(AzToolsFramework::InstanceDataHierarchy::InstanceDataNode* node)
            {
                // only want to print info when we know we are at a leaf node in the hierarchy (a field)
                if (auto editElement = node->GetElementEditMetadata())
                {
                    AZ_Printf("InstanceDataHierarchy", "FieldName: %s", editElement->m_name);

                    auto fullAddr = node->ComputeAddress();
                    for (auto addr : fullAddr)
                    {
                        AZ_Printf("InstanceDataHierarchy", "CRC: %llu", addr);
                    }

                    AZ_Printf("InstanceDataHierarchy", "----------------------");
                }

                for (AzToolsFramework::InstanceDataHierarchy::NodeContainer::iterator it = node->GetChildren().begin(); it != node->GetChildren().end(); ++it)
                {
                    DebugPrintInstanceDataHierarchy(&*it);
                }
            }

            // convenience/util function for printing all fields on a component
            // note: can use entity->FindComponent(uuid) for param
            static void DebugPrintInstanceDataHierarchyForComponent(AZ::Component* component)
            {
                AZ::SerializeContext* serializeContext = NULL;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

                AzToolsFramework::InstanceDataHierarchy instanceDataHierarchy;
                instanceDataHierarchy.AddRootInstance(component);
                instanceDataHierarchy.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                DebugPrintInstanceDataHierarchy(instanceDataHierarchy.GetRoot());
            }

            // used to simply map from Cry to LY type (e.g. int -> AZ::u32)
            template<typename OldVarType, typename NewVarType>
            static NewVarType DefaultAdapter(const OldVarType& varType)
            {
                return static_cast<NewVarType>(varType);
            }

            // here we map a QString to a AZStd::string used by a new component
            static AZStd::string QStringAdapter(const QString& string)
            {
                return AZStd::string(string.toUtf8().constData());
            }

            // helper to read value from CryEntity using IVariableContainer api (FindVariable) - return success if value is found
            template<typename OldVarType, typename NewVarType>
            static bool ConvertOldVar(const char* variableName, const IVariableContainer* variableContainer, NewVarType* newVarOut, NewVarType(*variableAdapter)(const OldVarType&) = DefaultAdapter)
            {
                if (const IVariable* var = variableContainer->FindVariable(variableName, true, false))
                {
                    OldVarType variable;
                    var->Get(variable);

                    *newVarOut = variableAdapter(variable);

                    return true;
                }

                return false;
            }

            // Specialization for Vec3. In lua it's represented as an array rather than a standard vector type.
            template<>
            static bool ConvertOldVar(const char* variableName, const IVariableContainer* variableContainer, AZ::Vector3* newVarOut, AZ::Vector3(*variableAdapter)(const Vec3&))
            {
                if (auto var = variableContainer->FindVariable(variableName, true, true))
                {
                    // Vec3 in lua is represented as an array
                    if (var->GetType() == IVariable::ARRAY)
                    {
                        int numVars = var->GetNumVariables();
                        AZ_Assert(numVars == 3, "%s is not a Vec3", variableName);

                        if (numVars == 3)
                        {
                            Vec3 vec3;
                            var->GetVariable(0)->Get(vec3.x);
                            var->GetVariable(1)->Get(vec3.y);
                            var->GetVariable(2)->Get(vec3.z);
                            *newVarOut = variableAdapter(vec3);
                            return true;
                        }
                    }
                    // Otherwise just fallback to default conversion
                    else
                    {
                        Vec3 vec3;
                        var->Get(vec3);
                        *newVarOut = variableAdapter(vec3);
                        return true;
                    }
                }
                return false;
            }

            // specialization of ConvertOldVar to simplify calling when we know the type doesn't change during conversion (e.g. with float/bool)
            template<typename VarType>
            static bool ConvertOldVar(const char* variableName, const IVariableContainer* variableContainer, VarType* newVarOut, VarType(*variableAdapter)(const VarType&) = DefaultAdapter)
            {
                return ConvertOldVar<VarType, VarType>(variableName, variableContainer, newVarOut, variableAdapter);
            }

            // helper to write a value to a given field on a component directly (using InstanceDataHierarchy api)
            template<typename VarType>
            static bool SetVarHierarchy(AZ::Entity* entity, AZ::Uuid componentId, const AzToolsFramework::InstanceDataHierarchy::InstanceDataNode::Address& instanceAddress, const VarType& value)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

                AZ::Component* component = entity->FindComponent(componentId);
                AZ_Assert(component, "Component: %s not found on entity", componentId.ToString<AZStd::string>().c_str());

                // object to serialize at root of hierarchy (component)
                AzToolsFramework::InstanceDataHierarchy instanceDataHierarchy;
                instanceDataHierarchy.AddRootInstance(component);
                instanceDataHierarchy.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                // search for field instance in hierarchy - return success if value was found and written
                if (AzToolsFramework::InstanceDataHierarchy::InstanceDataNode* instanceNode = instanceDataHierarchy.FindNodeByPartialAddress(instanceAddress))
                {
                    instanceNode->Write(value);
                    return true;
                }

                return false;
            }

            // wrapper function to take CryEntity value and post to event bus to set on new component
            // @variableName is name of old cry entity variable (NOT human readable name)
            // @variableContainer is Cry structre holding variable to convert
            // @func is bus conversion function
            // @entityId is id of newly created LY entity
            // @variableAdapter is a function pointer to map between Cry and LY types
            template<typename Bus, typename OldVarType, typename NewVarType, typename Func>
            static bool ConvertVarBus(const char* variableName, const IVariableContainer* variableContainer, Func func, AZ::EntityId entityId, NewVarType(*variableAdapter)(const OldVarType&) = DefaultAdapter)
            {
                NewVarType variable;
                if (ConvertOldVar<OldVarType, NewVarType>(variableName, variableContainer, &variable, variableAdapter))
                {
                    Bus::Event(entityId, func, variable);
                    return true;
                }

                AZ_Warning("Legacy Entity Converter", "Conversion failed for variable: %s", variableName);
                return false;
            }

            // specialization of ConvertVarBus to simplify calling when we know the type doesn't change during conversion (e.g. with float/bool)
            template<typename EventBus, typename VarType, typename Func>
            static bool ConvertVarBus(const char* variable, const IVariableContainer* variableContainer, Func func, AZ::EntityId entityId, VarType(*variableAdapter)(const VarType&) = DefaultAdapter)
            {
                return ConvertVarBus<EventBus, VarType, VarType, Func>(variable, variableContainer, func, entityId, variableAdapter);
            }

            // wrapper function to take CryEntity value and write to a component directly (using InstanceDataHierarchy)
            // @entity - entity the component exists on that will be updated (specific instance)
            // @componentId - the UUID of the component to find on the entity
            // @instanceAddress - vector of crc values of fields to find on component - values pushed in reverse order (MyVariable, MyStructure, MyComponent)
            // note: component can be omitted as we have a specific component as the root
            // vector should usually contain - OwningType, Field - e.g. "EditorLightConfiguration", "OnInitially"
            // @oldVariableName is name of old cry entity variable (NOT human readable name)
            // @variableContainer is Cry structre holding variable to convert
            // @variableAdapter is a function pointer to map between Cry and LY types
            template<typename OldVarType, typename NewVarType>
            static bool ConvertVarHierarchy(AZ::Entity* entity, AZ::Uuid componentId, const AzToolsFramework::InstanceDataHierarchy::InstanceDataNode::Address& instanceAddress,
                const char* oldVariableName, const IVariableContainer* variableContainer, NewVarType(*variableAdapter)(const OldVarType&) = DefaultAdapter)
            {
                NewVarType variable;
                if (ConvertOldVar<OldVarType, NewVarType>(oldVariableName, variableContainer, &variable, variableAdapter))
                {
                    return SetVarHierarchy(entity, componentId, instanceAddress, variable);
                }

                return false;
            }

            // specialization of ConvertVarHierarchy to simplify calling when we know the type doesn't change during conversion (e.g. with float/bool)
            template<typename VarType>
            static bool ConvertVarHierarchy(AZ::Entity* entity, AZ::Uuid componentId, const AzToolsFramework::InstanceDataHierarchy::InstanceDataNode::Address& instanceAddress,
                const char* oldVariableName, const IVariableContainer* variableContainer, VarType(*variableAdapter)(const VarType&) = DefaultAdapter)
            {
                return ConvertVarHierarchy<VarType, VarType>(entity, componentId, instanceAddress, oldVariableName, variableContainer, variableAdapter);
            }

            // util to create converted entity - return AZ::Outcome, if success, EntityId will be set, otherwise return LegacyConversionResult as error
            static AZ::Outcome<AZ::EntityId, LegacyConversionResult> CreateEntityForConversion(CBaseObject* entityToConvert, const AZ::ComponentTypeList& componentsToAdd)
            {
                AZ::Outcome<AZ::Entity*, CreateEntityResult> entityResult(nullptr);
                LegacyConversionRequestBus::BroadcastResult(entityResult, &LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentsToAdd);

                // if we have a parent, and the parent is not yet converted, we ignore this entity!
                // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
                // inside the CreateConvertedEntity function.

                if (!entityResult.IsSuccess())
                {
                    // if we failed due to no parent, we keep processing
                    return entityResult.GetError() == CreateEntityResult::FailedNoParent
                        ? AZ::Failure(LegacyConversionResult::Ignored)
                        : AZ::Failure(LegacyConversionResult::Failed);
                }

                if (AZ::Entity* newEntity = entityResult.GetValue())
                {
                    return AZ::Success(newEntity->GetId());
                }
                else
                {
                    AZ_Error("Legacy Conversion", false, "Failed to create a new entity during legacy conversion.");
                    return AZ::Failure(LegacyConversionResult::Failed);
                }
            }

            static bool FindVector3(const CVarBlock* varBlock, const char* nameX, const char* nameY, const char* nameZ, bool bRecursive, bool bHumanName, AZ::Vector3& outVec3)
            {
                const IVariable* varX = varBlock->FindVariable(nameX, bRecursive, bHumanName);
                const IVariable* varY = varBlock->FindVariable(nameY, bRecursive, bHumanName);
                const IVariable* varZ = varBlock->FindVariable(nameZ, bRecursive, bHumanName);

                if (!varX || !varY || !varZ)
                {
                    return false;
                }

                float x, y, z;
                varX->Get(x);
                varY->Get(y);
                varZ->Get(z);

                outVec3 = AZ::Vector3(x, y, z);
                return true;
            }
        } //namespace Util
    } //namespace LegacyEntityConversion
}// namespace AZ