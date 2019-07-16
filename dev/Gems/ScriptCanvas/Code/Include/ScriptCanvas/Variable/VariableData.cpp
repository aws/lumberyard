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

#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Variable/VariableData.h>

namespace ScriptCanvas
{
    static bool VariableDataVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() == 0)
        {
            AZStd::unordered_map<AZ::Uuid, VariableNameValuePair> uuidToVariableMap;
            if (!rootElementNode.GetChildData(AZ_CRC("m_nameVariableMap", 0xc4de98e7), uuidToVariableMap))
            {
                AZ_Error("Script Canvas", false, "Variable id in version 0 VariableData element should be AZ::Uuid");
                return false;
            }

            rootElementNode.RemoveElementByName(AZ_CRC("m_nameVariableMap", 0xc4de98e7));
            AZStd::unordered_map<VariableId, VariableNameValuePair> nameToVariableMap;
            for (const auto& uuidToVariableNamePair : uuidToVariableMap)
            {
                nameToVariableMap.emplace(uuidToVariableNamePair.first, uuidToVariableNamePair.second);
            }

            rootElementNode.AddElementWithData(context, "m_nameVariableMap", nameToVariableMap);
            return true;
        }

        return true;
    }
    void VariableData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VariableData>()
                ->Version(1, &VariableDataVersionConverter)
                ->Field("m_nameVariableMap", &VariableData::m_variableMap)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<VariableData>("Variables", "Variables exposed by the attached Script Canvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Variable Fields")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VariableData::m_variableMap, "Variables", "Table of Variables within the Script Canvas Graph")
                    ;
            }
        }
    }

    VariableData::VariableData(VariableData&& other)
        : m_variableMap(AZStd::move(other.m_variableMap))
    {
        other.m_variableMap.clear();
    }

    VariableData& VariableData::operator=(VariableData&& other)
    {
        if (this != &other)
        {
            m_variableMap = AZStd::move(other.m_variableMap);
            other.m_variableMap.clear();
        }

        return *this;
    }

    AZ::Outcome<VariableId, AZStd::string> VariableData::AddVariable(AZStd::string_view varName, const VariableDatum& varDatum)
    {
        auto insertIt = m_variableMap.emplace(varDatum.GetId(), VariableNameValuePair{ varName, varDatum });
        if (insertIt.second)
        {
            return AZ::Success(insertIt.first->first);
        }

        return AZ::Failure(AZStd::string::format("Variable with id %s already exist in variable map. The Variable name is %s", insertIt.first->first, insertIt.first->second.GetVariableName().data()));
    }

    VariableDatum* VariableData::FindVariable(AZStd::string_view variableName)
    {
        auto foundIt = AZStd::find_if(m_variableMap.begin(), m_variableMap.end(), [&variableName](const AZStd::pair<VariableId, VariableNameValuePair>& varPair)
        {
            return variableName == varPair.second.GetVariableName();
        });

        return foundIt != m_variableMap.end() ? &foundIt->second.m_varDatum : nullptr;
    }

    VariableNameValuePair* VariableData::FindVariable(VariableId variableId)
    {
        AZStd::pair<AZStd::string_view, VariableDatum*> resultPair;
        auto foundIt = m_variableMap.find(variableId);
        return foundIt != m_variableMap.end() ? &foundIt->second : nullptr;
    }

    void VariableData::Clear()
    {
        m_variableMap.clear();
    }

    size_t VariableData::RemoveVariable(AZStd::string_view variableName)
    {
        size_t removedVars = 0U;
        for (auto varIt = m_variableMap.begin(); varIt != m_variableMap.end();)
        {
            if (varIt->second.GetVariableName() == variableName)
            {
                ++removedVars;
                varIt = m_variableMap.erase(varIt);
            }
            else
            {
                ++varIt;
            }
        }
        return removedVars;
    }

    bool VariableData::RemoveVariable(const VariableId& variableId)
    {
        return m_variableMap.erase(variableId) != 0;
    }

    bool VariableData::RenameVariable(const VariableId& variableId, AZStd::string_view newVarName)
    {
        auto foundIt = m_variableMap.find(variableId);
        if (foundIt != m_variableMap.end())
        {
            foundIt->second.SetVariableName(newVarName);
            return true;
        }

        return false;
    }

    static bool EditableVariableDataConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() <= 1)
        {
            AZStd::list<VariableNameValuePair> varNameValueVariableList;
            if (!rootElementNode.GetChildData(AZ_CRC("m_properties", 0x4227dbda), varNameValueVariableList))
            {
                AZ_Error("ScriptCanvas", false, "Unable to find m_properties list of VariableNameValuePairs on EditableVariableData version %d", rootElementNode.GetVersion());
                return false;
            }

            AZStd::list<EditableVariableConfiguration> editableVariableConfigurationList;
            for (auto varNameValuePair : varNameValueVariableList)
            {
                editableVariableConfigurationList.push_back({ varNameValuePair, varNameValuePair.m_varDatum.GetData() });
            }

            rootElementNode.RemoveElementByName(AZ_CRC("m_properties", 0x4227dbda));
            rootElementNode.AddElementWithData(serializeContext, "m_variables", editableVariableConfigurationList);
        }
        return true;
    }

    void EditableVariableData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::list<VariableNameValuePair>>::GetGenericInfo())
            {
                genericClassInfo->Reflect(serializeContext);
            }

            serializeContext->Class<EditableVariableData>()
                ->Version(2, &EditableVariableDataConverter)
                ->Field("m_variables", &EditableVariableData::m_variables)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditableVariableData>("Variables", "Variables exposed by the attached Script Canvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Variable Fields")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditableVariableData::m_variables, "Variables", "Array of Variables within Script Canvas Graph")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    EditableVariableData::EditableVariableData()
    {
    }

    AZ::Outcome<void, AZStd::string> EditableVariableData::AddVariable(AZStd::string_view varName, const VariableDatum& varDatum)
    {
        if (FindVariable(varDatum.GetId()))
        {
            return AZ::Failure(AZStd::string::format("Variable %s already exist", varName.data()));
        }

        EditableVariableConfiguration newVarConfig{ { AZStd::string(varName), varDatum }, varDatum.GetData() };
        newVarConfig.m_varNameValuePair.m_varDatum.GetData().SetLabel(varName);
        m_variables.push_back(AZStd::move(newVarConfig));

        return AZ::Success();
    }

    EditableVariableConfiguration* EditableVariableData::FindVariable(AZStd::string_view variableName)
    {
        auto foundIt = AZStd::find_if(m_variables.begin(), m_variables.end(), [&variableName](const EditableVariableConfiguration& variablePair)
        {
            return variableName == variablePair.m_varNameValuePair.GetVariableName();
        });

        return foundIt != m_variables.end() ? &*foundIt : nullptr;
    }

    EditableVariableConfiguration* EditableVariableData::FindVariable(VariableId variableId)
    {
        auto foundIt = AZStd::find_if(m_variables.begin(), m_variables.end(), [&variableId](const EditableVariableConfiguration& variablePair)
        {
            return variableId == variablePair.m_varNameValuePair.m_varDatum.GetId();
        });

        return foundIt != m_variables.end() ? &*foundIt : nullptr;
    }

    void EditableVariableData::Clear()
    {
        m_variables.clear();
    }

    size_t EditableVariableData::RemoveVariable(AZStd::string_view variableName)
    {
        size_t removedCount = 0;
        auto removeIt = m_variables.begin();
        while (removeIt != m_variables.end())
        {
            if (removeIt->m_varNameValuePair.GetVariableName() == variableName)
            {
                ++removedCount;
                removeIt = m_variables.erase(removeIt);
            }
            else
            {
                ++removeIt;
            }
        }

        return removedCount;
    }

    bool EditableVariableData::RemoveVariable(const VariableId& variableId)
    {
        for (auto removeIt = m_variables.begin(); removeIt != m_variables.end(); ++removeIt)
        {
            if (removeIt->m_varNameValuePair.m_varDatum.GetId() == variableId)
            {
                m_variables.erase(removeIt);
                return true;
            }
        }

        return false;
    }

    void EditableVariableConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditableVariableConfiguration>()
                ->Version(0)
                ->Field("m_variableNameValuePair", &EditableVariableConfiguration::m_varNameValuePair)
                ->Field("m_defaultValue", &EditableVariableConfiguration::m_defaultValue)
                ;
            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditableVariableConfiguration>("Variable Element", "Represents a mapping of name to value")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditableVariableConfiguration::m_varNameValuePair, "Name,Value", "Variable Name and value")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }
}