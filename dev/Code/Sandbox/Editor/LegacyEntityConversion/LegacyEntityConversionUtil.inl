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
            inline bool IsClassOf(CBaseObject* check, const char* className)
            {
                CObjectClassDesc* classDesc = check->GetClassDesc();
                if (!classDesc)
                {
                    return false;
                }

                return (GetIEditor()->GetClassFactory()->FindClass(className) == classDesc);
            }

            // map from CryEngine ESystemConfigSpec to LY EngineSpec enum
            inline EngineSpec SpecConversion(ESystemConfigSpec spec)
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
                case CONFIG_AUTO_SPEC: //Auto is displayed in the editor as All
                    return EngineSpec::Low;
                default:
                    AZ_Error("Legacy Conversion", false, "Unhandled ESystemConfigSpec: %d", spec);
                    return EngineSpec::Never;
                }
            }

            // recursive debug function to print all properties of a particular VariableContainer (example: CVarBlock)
            inline void DebugPrintVariableContainer(const char* typeName, const IVariableContainer* varContainer)
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
            inline void DebugPrintInstanceDataHierarchy(AzToolsFramework::InstanceDataHierarchy::InstanceDataNode* node)
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
            inline void DebugPrintInstanceDataHierarchyForComponent(Component* component)
            {
                SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

                AzToolsFramework::InstanceDataHierarchy instanceDataHierarchy;
                instanceDataHierarchy.AddRootInstance(component);
                instanceDataHierarchy.Build(serializeContext, SerializeContext::ENUM_ACCESS_FOR_READ);

                DebugPrintInstanceDataHierarchy(instanceDataHierarchy.GetRoot());
            }

            // used to simply map from Cry to LY type (e.g. int -> u32)
            template<typename OldVarType, typename NewVarType>
            NewVarType DefaultAdapter(const OldVarType& varType)
            {
                return static_cast<NewVarType>(varType);
            }

            // here we map a QString to a AZStd::string used by a new component
            inline AZStd::string QStringAdapter(const QString& string)
            {
                return AZStd::string(string.toUtf8().constData());
            }

            // helper to read value from CryEntity using IVariableContainer api (FindVariable) - return success if value is found
            template<typename OldVarType, typename NewVarType>
            bool ConvertOldVar(const char* variableName, const IVariableContainer* variableContainer, NewVarType* newVarOut, NewVarType(*variableAdapter)(const OldVarType&) = DefaultAdapter)
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

            // Reads a vec3 from a variable block. Handles both array (lua) and regular represenations
            inline bool GetVec3(const char* variableName, const IVariableContainer* variableContainer, Vec3& out)
            {
                if (auto var = variableContainer->FindVariable(variableName, true, false))
                {
                    // Vec3 in lua is represented as an array
                    if (var->GetType() == IVariable::ARRAY)
                    {
                        int numVars = var->GetNumVariables();
                        AZ_Assert(numVars == 3, "%s is not a Vec3", variableName);

                        if (numVars == 3)
                        {
                            var->GetVariable(0)->Get(out.x);
                            var->GetVariable(1)->Get(out.y);
                            var->GetVariable(2)->Get(out.z);
                            return true;
                        }
                    }
                    // Otherwise just fallback to default conversion
                    else
                    {
                        var->Get(out);
                        return true;
                    }
                }
                return false;
            }

            // Specialization for Vec3. In lua it's represented as an array rather than a standard vector type.
            template<>
            inline bool ConvertOldVar(const char* variableName, const IVariableContainer* variableContainer, Vector3* newVarOut, Vector3(*variableAdapter)(const Vec3&))
            {
                Vec3 out;
                if (GetVec3(variableName, variableContainer, out))
                {
                    *newVarOut = variableAdapter(out);
                    return true;
                }
                
                return false;
            }

            // specialization of ConvertOldVar to simplify calling when we know the type doesn't change during conversion (e.g. with float/bool)
            template<typename VarType>
            bool ConvertOldVar(
                const char* variableName, const IVariableContainer* variableContainer, 
                VarType* newVarOut, VarType(*variableAdapter)(const VarType&) = DefaultAdapter)
            {
                return ConvertOldVar<VarType, VarType>(variableName, variableContainer, newVarOut, variableAdapter);
            }

            // helper to write a value to a given field on a component directly (using InstanceDataHierarchy api)
            template<typename VarType>
            bool SetVarHierarchy(
                Entity* entity, Uuid componentId, 
                const AzToolsFramework::InstanceDataHierarchy::InstanceDataNode::Address& instanceAddress, 
                const VarType& value)
            {
                SerializeContext* serializeContext = nullptr;
                EBUS_EVENT_RESULT(serializeContext, ComponentApplicationBus, GetSerializeContext);

                Component* component = entity->FindComponent(componentId);
                AZ_Assert(component, "Component: %s not found on entity", componentId.ToString<AZStd::string>().c_str());

                // object to serialize at root of hierarchy (component)
                AzToolsFramework::InstanceDataHierarchy instanceDataHierarchy;
                instanceDataHierarchy.AddRootInstance(component);
                instanceDataHierarchy.Build(serializeContext, SerializeContext::ENUM_ACCESS_FOR_READ);

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
            bool ConvertVarBus(
                const char* variableName, const IVariableContainer* variableContainer, Func func, 
                EntityId entityId, NewVarType(*variableAdapter)(const OldVarType&) = DefaultAdapter)
            {
                NewVarType variable;
                if (ConvertOldVar<OldVarType, NewVarType>(variableName, variableContainer, &variable, variableAdapter))
                {
                    Bus::Event(entityId, func, variable);
                    return true;
                }

                AZ_Warning("Legacy Entity Converter", false, "Conversion failed for variable: %s", variableName);
                return false;
            }

            // specialization of ConvertVarBus to simplify calling when we know the type doesn't change during conversion (e.g. with float/bool)
            template<typename EventBus, typename VarType, typename Func>
            bool ConvertVarBus(
                const char* variable, const IVariableContainer* variableContainer, Func func, 
                EntityId entityId, VarType(*variableAdapter)(const VarType&) = DefaultAdapter)
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
            bool ConvertVarHierarchy(
                Entity* entity, Uuid componentId, const AzToolsFramework::InstanceDataHierarchy::InstanceDataNode::Address& instanceAddress,
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
            bool ConvertVarHierarchy(
                Entity* entity, Uuid componentId, const AzToolsFramework::InstanceDataHierarchy::InstanceDataNode::Address& instanceAddress,
                const char* oldVariableName, const IVariableContainer* variableContainer, VarType(*variableAdapter)(const VarType&) = DefaultAdapter)
            {
                return ConvertVarHierarchy<VarType, VarType>(entity, componentId, instanceAddress, oldVariableName, variableContainer, variableAdapter);
            }

            // util to create converted entity - return Outcome, if success, EntityId will be set, otherwise return LegacyConversionResult as error
            inline Outcome<EntityId, LegacyConversionResult> CreateEntityForConversion(
                CBaseObject* entityToConvert, const ComponentTypeList& componentsToAdd)
            {
                Outcome<Entity*, CreateEntityResult> entityResult(nullptr);
                LegacyConversionRequestBus::BroadcastResult(entityResult, &LegacyConversionRequests::CreateConvertedEntity, entityToConvert, true, componentsToAdd);

                // if we have a parent, and the parent is not yet converted, we ignore this entity!
                // this is not ALWAYS the case - there may be types you wish to create anyway and just put them in the world, so we cannot enforce this
                // inside the CreateConvertedEntity function.

                if (!entityResult.IsSuccess())
                {
                    // if we failed due to no parent, we keep processing
                    return entityResult.GetError() == CreateEntityResult::FailedNoParent
                        ? Failure(LegacyConversionResult::Ignored)
                        : Failure(LegacyConversionResult::Failed);
                }

                if (Entity* newEntity = entityResult.GetValue())
                {
                    return Success(newEntity->GetId());
                }
                
                AZ_Error("Legacy Conversion", false, "Failed to create a new entity during legacy conversion.");
                return Failure(LegacyConversionResult::Failed);
            }

            inline bool FindVector3(
                const CVarBlock* varBlock, const char* nameX, const char* nameY, const char* nameZ, 
                bool bRecursive, bool bHumanName, Vector3& outVec3)
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

                outVec3 = Vector3(x, y, z);
                
                return true;
            }

            inline AZ::Color LegacyColorConverter(const Vec3& vec)
            {
                QColor col = ColorLinearToGamma(ColorF(
                    vec[0],
                    vec[1],
                    vec[2]));

                AZ::Color color;
                color.SetR8(col.red());
                color.SetG8(col.green());
                color.SetB8(col.blue());
                color.SetA8(col.alpha());
                return color;
            }
        } //namespace Util
    } //namespace LegacyEntityConversion
}// namespace AZ
