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

#include <ScriptCanvas/Variable/GraphVariable.h>

#include <ScriptCanvas/Core/GraphScopedTypes.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    void ReplicaNetworkProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ReplicaNetworkProperties>()
                ->Version(1)
                ->Field("m_isSynchronized", &ReplicaNetworkProperties::m_isSynchronized)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ReplicaNetworkProperties>("ReplicaNetworkProperties", "Network Properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ReplicaNetworkProperties::m_isSynchronized, "Is Synchronized", "Controls whether or not this value is reflected across the network.")
                    ;
            }
        }
    }

    //////////////////
    // GraphVariable
    //////////////////

    class BehaviorVariableChangedBusHandler : public VariableNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorVariableChangedBusHandler, "{6469646D-EB7A-4F76-89E3-81EF05D2E688}", AZ::SystemAllocator,
            OnVariableValueChanged);

        // Sent when the light is turned on.
        void OnVariableValueChanged() override
        {
            Call(FN_OnVariableValueChanged);
        }
    };

    void GraphVariable::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Don't want to store the scoped id. That will need to be generated at point.
            // For now we focus only on the identifier.
            serializeContext->Class<GraphScopedVariableId>()
                ->Version(1)
                ->Field("Identifier", &GraphScopedVariableId::m_identifier)
            ;


            serializeContext->Class<GraphVariable>()
                ->Version(1)
                ->Field("ExposeAsInput", &GraphVariable::m_exposeAsInput)
                ->Field("ReplicaNetProps", &GraphVariable::m_networkProperties)
                ->Field("InputControlVisibility", &GraphVariable::m_inputControlVisibility)
                ->Field("ExposureCategory", &GraphVariable::m_exposureCategory)
                ->Field("VariableId", &GraphVariable::m_variableId)
                    ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &VariableId::MakeVariableId)
                ->Field("Datum", &GraphVariable::m_datum)
                ->Field("VariableName", &GraphVariable::m_variableName)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphVariable>("Variable", "Represents a Variable field within a Script Canvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChildNameLabelOverride, &GraphVariable::GetDisplayName)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &GraphVariable::GetDisplayName)
                    ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &GraphVariable::GetDescriptionOverride)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GraphVariable::m_exposeAsInput, "Expose On Component", "Controls whether or not this value is configurable from a Script Canvas Component")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetInputControlVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnExposureChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_exposureCategory, "Property Grouping", "Controls which group the specified variable will be exposed into.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetInputControlVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnExposureGroupChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_datum, "Datum", "Datum within Script Canvas Graph")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnValueChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_networkProperties, "Network Properties", "Enables whether or not this value should be network synchronized")
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);

        if (behaviorContext)
        {
            behaviorContext->Class<GraphScopedVariableId>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ;

            behaviorContext->EBus<VariableNotificationBus>(GetVariableNotificationBusName(), "VariableNotificationBus", "Notifications from the Variables in the current Script Canvas graph")
                ->Attribute(AZ::Script::Attributes::Category, "Variables")
                ->Handler<BehaviorVariableChangedBusHandler>()
            ;
        }

        ReplicaNetworkProperties::Reflect(context);
    }

    GraphVariable::GraphVariable()
        : m_exposeAsInput(false)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
    {
    }

    GraphVariable::GraphVariable(const Datum& datum)
        : m_exposeAsInput(false)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
        , m_datum(datum)
    {
    }

    GraphVariable::GraphVariable(Datum&& datum)
        : m_exposeAsInput(false)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
        , m_datum(AZStd::move(datum))
    {
    }

    GraphVariable::GraphVariable(const Datum& variableData, const VariableId& variableId)
        : GraphVariable(variableData)
    {
        m_variableId = variableId;
    }

    GraphVariable::GraphVariable(Deprecated::VariableNameValuePair&& valuePair)
        : GraphVariable(AZStd::move(valuePair.m_varDatum.GetData()))
    {
        SetVariableName(AZStd::move(valuePair.GetVariableName()));
        m_variableId = valuePair.m_varDatum.GetId();

        m_exposeAsInput = valuePair.m_varDatum.ExposeAsComponentInput();
        m_inputControlVisibility = valuePair.m_varDatum.GetInputControlVisibility();
        m_visibility = valuePair.m_varDatum.GetVisibility();

        m_exposureCategory = valuePair.m_varDatum.GetExposureCategory();

        m_signalValueChanges = valuePair.m_varDatum.AllowsSignalOnChange();
    }

    GraphVariable::~GraphVariable()
    {
        DatumNotificationBus::Handler::BusDisconnect();
    }

    void GraphVariable::DeepCopy(const GraphVariable& source)
    {
        *this = source;
        m_datum.DeepCopyDatum(source.m_datum);
    }

    bool GraphVariable::operator==(const GraphVariable& rhs) const
    {
        return GetVariableId() == rhs.GetVariableId();
    }

    bool GraphVariable::operator!=(const GraphVariable& rhs) const
    {
        return !operator==(rhs);
    }

    const Data::Type& GraphVariable::GetDataType() const
    {
        return GetDatum()->GetType();
    }

    const VariableId& GraphVariable::GetVariableId() const
    {
        return m_variableId;
    }

    const Datum* GraphVariable::GetDatum() const
    {
        return &m_datum;
    }

    void GraphVariable::ConfigureDatumView(ModifiableDatumView& datumView)
    {
        datumView.ConfigureView((*this));
    }

    void GraphVariable::SetVariableName(AZStd::string_view variableName)
    {
        m_variableName = variableName;
        SetDisplayName(variableName);
    }

    AZStd::string_view GraphVariable::GetVariableName() const
    {        
        return m_variableName;
    }

    void GraphVariable::SetDisplayName(const AZStd::string& displayName)
    {
        m_datum.SetLabel(displayName);
    }

    AZStd::string_view GraphVariable::GetDisplayName() const
    {
        return m_datum.GetLabel();
    }

    AZ::Crc32 GraphVariable::GetInputControlVisibility() const
    {
        return m_inputControlVisibility;
    }

    void GraphVariable::SetInputControlVisibility(const AZ::Crc32& inputControlVisibility)
    {
        m_inputControlVisibility = inputControlVisibility;
    }

    AZ::Crc32 GraphVariable::GetVisibility() const
    {
        return m_visibility;
    }

    void GraphVariable::SetVisibility(AZ::Crc32 visibility)
    {
        m_visibility = visibility;
    }

    void GraphVariable::GenerateNewId()
    {
        m_variableId = VariableId::MakeVariableId();        
    }

    void GraphVariable::SetAllowSignalOnChange(bool allowSignalChange)
    {
        m_signalValueChanges = allowSignalChange;
    }

    void GraphVariable::SetOwningScriptCanvasId(const ScriptCanvasId& scriptCanvasId)
    {
        if (m_scriptCanvasId != scriptCanvasId)
        {
            m_scriptCanvasId = scriptCanvasId;

            if (!m_datumId.IsValid())
            {
                m_datumId = AZ::Entity::MakeId();
                m_datum.SetNotificationsTarget(m_datumId);
                DatumNotificationBus::Handler::BusConnect(m_datumId);
            }
        }
    }

    GraphScopedVariableId GraphVariable::GetGraphScopedId() const
    {
        return GraphScopedVariableId(m_scriptCanvasId, m_variableId);
    }

    void GraphVariable::OnDatumEdited(const Datum* datum)
    {
        VariableNotificationBus::Event(m_notificationId, &VariableNotifications::OnVariableValueChanged);
    }

    void GraphVariable::OnExposureChanged()
    {
        VariableNotificationBus::Event(m_notificationId, &VariableNotifications::OnVariableExposureChanged);
    }

    void GraphVariable::OnExposureGroupChanged()
    {
        VariableNotificationBus::Event(m_notificationId, &VariableNotifications::OnVariableExposureGroupChanged);
    }

    void GraphVariable::OnValueChanged()
    {
        if (m_signalValueChanges)
        {
            AZ_TracePrintf("OnValueChanged", "OnValueChanged");
            //VariableNotificationBus::Event(GetVariableId(), &VariableNotifications::OnVariableValueChanged);
        }
    }

    AZStd::string GraphVariable::GetDescriptionOverride()
    {
        return Data::GetName(m_datum.GetType());
    }
}
