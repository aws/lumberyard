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

#include "precompiled.h"

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Graph.h>

namespace ScriptCanvas
{
    const char* CopiedVariableData::k_variableKey = "ScriptCanvas::CopiedVariableData";

    GraphVariableManagerComponent::GraphVariableManagerComponent()
    {
    }

    GraphVariableManagerComponent::GraphVariableManagerComponent(AZ::EntityId graphUniqueId)
        : m_graphUniqueId(graphUniqueId)
    {
    }

    GraphVariableManagerComponent::~GraphVariableManagerComponent()
    {
        GraphVariableManagerRequestBus::MultiHandler::BusDisconnect();
        VariableRequestBus::MultiHandler::BusDisconnect();
    }

    void GraphVariableManagerComponent::Reflect(AZ::ReflectContext* context)
    {
        VariableId::Reflect(context);
        VariableDatumBase::Reflect(context);
        VariableDatum::Reflect(context);
        VariableNameValuePair::Reflect(context);
        VariableData::Reflect(context);
        EditableVariableConfiguration::Reflect(context);
        EditableVariableData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CopiedVariableData>()
                ->Version(1)
                ->Field("Mapping", &CopiedVariableData::m_variableMapping)
                ;

            serializeContext->Class<GraphVariableManagerComponent, AZ::Component>()
                ->Version(2)
                ->Field("m_variableData", &GraphVariableManagerComponent::m_variableData)
                ->Field("m_uniqueId", &GraphVariableManagerComponent::m_graphUniqueId)
                ->Field("CopiedVariableRemapping", &GraphVariableManagerComponent::m_copiedVariableRemapping)
                ;
        }
    }

    void GraphVariableManagerComponent::Init()
    {
        SetUniqueId(m_graphUniqueId);

        for (const auto& varPair : m_variableData.GetVariables())
        {
            VariableRequestBus::MultiHandler::BusConnect(varPair.first);
        }
    }

    void GraphVariableManagerComponent::SetUniqueId(AZ::EntityId uniqueId)
    {
        GraphVariableManagerRequestBus::MultiHandler::BusDisconnect(m_graphUniqueId);
        m_graphUniqueId = uniqueId;
        if (m_graphUniqueId.IsValid())
        {
            GraphVariableManagerRequestBus::MultiHandler::BusConnect(m_graphUniqueId);
        }
    }

    void GraphVariableManagerComponent::Activate()
    {
        // Attaches to the EntityId of the containing component for servicing Variable Request
        GraphVariableManagerRequestBus::MultiHandler::BusConnect(GetEntityId());
    }

    void GraphVariableManagerComponent::Deactivate()
    {
        GraphVariableManagerRequestBus::MultiHandler::BusDisconnect(GetEntityId());
    }

    VariableDatum* GraphVariableManagerComponent::GetVariableDatum()
    {
        VariableNameValuePair* variableNameValuePair{};
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            variableNameValuePair = m_variableData.FindVariable(*variableId);
        }

        return variableNameValuePair ? &variableNameValuePair->m_varDatum : nullptr;
    }

    Data::Type GraphVariableManagerComponent::GetType() const
    {
        const VariableNameValuePair* variableNameValuePair{};
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            variableNameValuePair = m_variableData.FindVariable(*variableId);
        }

        return variableNameValuePair ? variableNameValuePair->m_varDatum.GetData().GetType() : Data::Type::Invalid();
    }

    AZStd::string_view GraphVariableManagerComponent::GetName() const
    {
        const VariableNameValuePair* variableNameValuePair{};
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            variableNameValuePair = m_variableData.FindVariable(*variableId);
        }

        return variableNameValuePair ? AZStd::string_view(variableNameValuePair->m_varName) : "";
    }

    AZ::Outcome<void, AZStd::string> GraphVariableManagerComponent::RenameVariable(AZStd::string_view newVarName)
    {
        const VariableDatum* varDatum{};
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            return RenameVariable(*variableId, newVarName);
        }

        return AZ::Failure(AZStd::string::format("No variable id was found, cannot rename variable to %s", newVarName.data()));
    }

    AZ::Outcome<VariableId, AZStd::string> GraphVariableManagerComponent::RemapVariable(const VariableNameValuePair& variableConfiguration)
    {
        if (FindVariableById(variableConfiguration.m_varDatum.GetId()))
        {
            return AZ::Success(variableConfiguration.m_varDatum.GetId());
        }

        ScriptCanvas::VariableId remappedId = FindCopiedVariableRemapping(variableConfiguration.m_varDatum.GetId());

        if (remappedId.IsValid())
        {
            return AZ::Success(remappedId);
        }

        VariableNameValuePair copyConfiguration = variableConfiguration;
        copyConfiguration.m_varDatum.GenerateNewId();

        if (FindVariable(copyConfiguration.m_varName))
        {
            copyConfiguration.m_varName.append(" Copy");

            if (FindVariable(copyConfiguration.m_varName))
            {
                AZStd::string originalName = copyConfiguration.m_varName;

                int counter = 0;

                do
                {
                    ++counter;
                    copyConfiguration.m_varName = AZStd::string::format("%s (%i)", originalName.c_str(), counter);
                } while (FindVariable(copyConfiguration.m_varName));
            }
        }

        auto variableOutcome = m_variableData.AddVariable(copyConfiguration.m_varName, copyConfiguration.m_varDatum);
        if (!variableOutcome)
        {
            return variableOutcome;
        }

        const VariableId& newId = variableOutcome.GetValue();
        VariableRequestBus::MultiHandler::BusConnect(newId);
        GraphVariableManagerNotificationBus::Event(GetEntityId(), &GraphVariableManagerNotifications::OnVariableAddedToGraph, newId, copyConfiguration.m_varName);

        RegisterCopiedVariableRemapping(variableConfiguration.m_varDatum.GetId(), newId);

        return AZ::Success(newId);
    }

    AZ::Outcome<VariableId, AZStd::string> GraphVariableManagerComponent::AddVariable(AZStd::string_view name, const Datum& value)
    {
        if (FindVariable(name))
        {
            return AZ::Failure(AZStd::string::format("Variable %s already exists", name.data()));
        }

        auto addVariableOutcome = m_variableData.AddVariable(name, VariableDatum(value));
        if (!addVariableOutcome)
        {
            return addVariableOutcome;
        }

        const VariableId& newId = addVariableOutcome.GetValue();
        VariableRequestBus::MultiHandler::BusConnect(newId);
        GraphVariableManagerNotificationBus::Event(GetEntityId(), &GraphVariableManagerNotifications::OnVariableAddedToGraph, newId, name);

        return AZ::Success(newId);
    }

    AZ::Outcome<VariableId, AZStd::string> GraphVariableManagerComponent::AddVariablePair(const AZStd::pair<AZStd::string_view, Datum>& keyValuePair)
    {
        return AddVariable(keyValuePair.first, keyValuePair.second);
    }

    bool GraphVariableManagerComponent::RemoveVariable(const VariableId& variableId)
    {
        auto varNamePair = m_variableData.FindVariable(variableId);

        if (varNamePair)
        {
            VariableRequestBus::MultiHandler::BusDisconnect(variableId);
            VariableNotificationBus::Event(variableId, &VariableNotifications::OnVariableRemoved);
            GraphVariableManagerNotificationBus::Event(GetEntityId(), &GraphVariableManagerNotifications::OnVariableRemovedFromGraph, variableId, varNamePair->m_varName);

            // Bookkeeping for the copied Variable remapping
            UnregisterUncopiedVariableRemapping(variableId);

            return m_variableData.RemoveVariable(variableId);
        }

        return false;
    }

    AZStd::size_t GraphVariableManagerComponent::RemoveVariableByName(AZStd::string_view varName)
    {
        AZStd::size_t removedVars = 0U;
        for (auto varIt = m_variableData.GetVariables().begin(); varIt != m_variableData.GetVariables().end();)
        {
            if (varIt->second.m_varName == varName)
            {
                ScriptCanvas::VariableId variableId = varIt->first;

                // Bookkeeping for the copied Variable remapping
                UnregisterUncopiedVariableRemapping(variableId);

                ++removedVars;
                VariableRequestBus::MultiHandler::BusDisconnect(variableId);

                VariableNotificationBus::Event(variableId, &VariableNotifications::OnVariableRemoved);
                GraphVariableManagerNotificationBus::Event(GetEntityId(), &GraphVariableManagerNotifications::OnVariableRemovedFromGraph, variableId, varName);
                varIt = m_variableData.GetVariables().erase(varIt);
            }
            else
            {
                ++varIt;
            }
        }
        return removedVars;
    }

    VariableDatum* GraphVariableManagerComponent::FindVariable(AZStd::string_view varName)
    {
        return m_variableData.FindVariable(varName);
    }

    Data::Type GraphVariableManagerComponent::GetVariableType(const VariableId& variableId)
    {
        auto variableNameValuePair = FindVariableById(variableId);
        return variableNameValuePair ? variableNameValuePair->m_varDatum.GetData().GetType() : Data::Type::Invalid();
    }

    VariableNameValuePair* GraphVariableManagerComponent::FindVariableById(const VariableId& variableId)
    {
        VariableNameValuePair* variableNameValuePair = m_variableData.FindVariable(variableId);
        return variableNameValuePair;
    }

    const AZStd::unordered_map<VariableId, VariableNameValuePair>* GraphVariableManagerComponent::GetVariables() const
    {
        return &m_variableData.GetVariables();
    }

    AZStd::unordered_map<VariableId, VariableNameValuePair>* GraphVariableManagerComponent::GetVariables()
    {
        return &m_variableData.GetVariables();
    }

    AZStd::string_view GraphVariableManagerComponent::GetVariableName(const VariableId& variableId) const
    {
        auto foundIt = m_variableData.GetVariables().find(variableId);

        return foundIt != m_variableData.GetVariables().end() ? AZStd::string_view(foundIt->second.m_varName) : AZStd::string_view();
    }

    AZ::Outcome<void, AZStd::string> GraphVariableManagerComponent::RenameVariable(const VariableId& variableId, AZStd::string_view newVarName)
    {
        auto varDatumPair = FindVariableById(variableId);

        if (!varDatumPair)
        {
            return AZ::Failure(AZStd::string::format("Unable to find variable with Id %s on Entity %s. Cannot rename",
                variableId.ToString().data(), GetEntityId().ToString().data()));
        }

        VariableDatum* newVariableDatum = FindVariable(newVarName);
        if (newVariableDatum && newVariableDatum->GetId() != variableId)
        {
            return AZ::Failure(AZStd::string::format("A variable with name %s already exists on Entity %s. Cannot rename",
                newVarName.data(), GetEntityId().ToString().data()));
        }

        if (!m_variableData.RenameVariable(variableId, newVarName))
        {
            return AZ::Failure(AZStd::string::format("Unable to rename variable with id %s to %s.",
                variableId.ToString().data(), newVarName.data()));
        }

        GraphVariableManagerNotificationBus::Event(GetEntityId(), &GraphVariableManagerNotifications::OnVariableNameChangedInGraph, variableId, newVarName);
        VariableNotificationBus::Event(variableId, &VariableNotifications::OnVariableRenamed, newVarName);

        return AZ::Success();
    }

    void GraphVariableManagerComponent::SetVariableData(const VariableData& variableData)
    {
        VariableRequestBus::MultiHandler::BusDisconnect();
        DeleteVariableData(m_variableData);
        for (const auto& varPair : variableData.GetVariables())
        {
            m_variableData.GetVariables().emplace(varPair.first, varPair.second);
            VariableRequestBus::MultiHandler::BusConnect(varPair.first);
            if (GetEntity())
            {
                GraphVariableManagerNotificationBus::Event(GetEntityId(), &GraphVariableManagerNotifications::OnVariableAddedToGraph, varPair.first, varPair.second.m_varName);
            }
        }

        if (GetEntity())
        {
            GraphVariableManagerNotificationBus::Event(GetEntityId(), &GraphVariableManagerNotifications::OnVariableDataSet);
        }
    }

    void GraphVariableManagerComponent::DeleteVariableData(const VariableData& variableData)
    {
        // Temporary vector to store the VariableIds in case the &variableData == &m_variableData
        AZStd::vector<VariableId> variableIds;
        variableIds.reserve(variableData.GetVariables().size());

        for (const auto& varPair : variableData.GetVariables())
        {
            variableIds.push_back(varPair.first);
        }
        for (const auto& variableId : variableIds)
        {
            RemoveVariable(variableId);
        }
    }

    VariableId GraphVariableManagerComponent::FindCopiedVariableRemapping(const VariableId& variableId) const
    {
        VariableId retVal;

        auto mapIter = m_copiedVariableRemapping.find(variableId);

        if (mapIter != m_copiedVariableRemapping.end())
        {
            retVal = mapIter->second;
        }

        return retVal;
    }

    void GraphVariableManagerComponent::RegisterCopiedVariableRemapping(const VariableId& originalValue, const VariableId& remappedId)
    {
        AZ_Error("ScriptCanvas", m_copiedVariableRemapping.find(originalValue) == m_copiedVariableRemapping.end(), "GraphVariableManagerComponent is trying to remap an original value twice");
        m_copiedVariableRemapping[originalValue] = remappedId;
    }

    void GraphVariableManagerComponent::UnregisterUncopiedVariableRemapping(const VariableId& remappedId)
    {
        auto eraseIter = AZStd::find_if(m_copiedVariableRemapping.begin(), m_copiedVariableRemapping.end(), [remappedId](const AZStd::pair<VariableId, VariableId>& otherId) { return remappedId == otherId.second; });

        if (eraseIter != m_copiedVariableRemapping.end())
        {
            m_copiedVariableRemapping.erase(eraseIter);
        }
    }
}