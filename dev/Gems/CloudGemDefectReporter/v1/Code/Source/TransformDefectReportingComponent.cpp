
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

#include "CloudGemDefectReporter_precompiled.h"

#include <TransformDefectReportingComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Math//MathUtils.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>

namespace CloudGemDefectReporter
{
    void TransformDefectReportingComponent::Init()
    {
    }

    void TransformDefectReportingComponent::Activate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusConnect();
    }
    void TransformDefectReportingComponent::Deactivate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusDisconnect();
    }

    void TransformDefectReportingComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TransformDefectReportingComponent, AZ::Component>()
                ->Version(1)
                ->Field("Identifier", &TransformDefectReportingComponent::m_identifier);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TransformDefectReportingComponent>("Transform Defect Reporting", 
                    "Reports transform information for this entity when a defect report it triggered")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Category, "Defect Reporter")
                    ->DataElement(AZ::Edit::UIHandlers::Default,
                        &TransformDefectReportingComponent::m_identifier,
                        "Identifier",
                        "String to identify this entity in the defect report, should be unique to this entity")
                    ;
            }
        }
    }

    void TransformDefectReportingComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
    }


    void TransformDefectReportingComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void TransformDefectReportingComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void TransformDefectReportingComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TransformDefectReportingComponent"));
    }

    static rapidjson::Value SerializeVector3TOJSONValue(const AZ::Vector3 vec, rapidjson::Document::AllocatorType& allocator)
    {
        rapidjson::Value vecObj(rapidjson::kObjectType);

        vecObj.AddMember("x", vec.GetX(), allocator);
        vecObj.AddMember("y", vec.GetY(), allocator);
        vecObj.AddMember("z", vec.GetZ(), allocator);

        return vecObj;
    }

    static AZStd::string SerializeTransformToJSONString(const AZStd::string identifier, AZ::Transform& local, AZ::Transform& world)
    {
        const AZ::Vector3 localScale = local.ExtractScaleExact();
        const AZ::Vector3 localTranslation = local.GetTranslation();
        const AZ::Vector3 localRotation = AZ::ConvertTransformToEulerDegrees(local);

        const AZ::Vector3 worldScale = world.ExtractScaleExact();
        const AZ::Vector3 worldTranslation = world.GetTranslation();
        const AZ::Vector3 worldRotation = AZ::ConvertTransformToEulerDegrees(world);

        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("Identifier", rapidjson::StringRef(identifier.c_str()), allocator);

        doc.AddMember("LocalScale", SerializeVector3TOJSONValue(localScale, allocator), allocator);
        doc.AddMember("LocalTranslation", SerializeVector3TOJSONValue(localTranslation, allocator), allocator);
        doc.AddMember("LocalRotation", SerializeVector3TOJSONValue(localRotation, allocator), allocator);

        doc.AddMember("WorldScale", SerializeVector3TOJSONValue(worldScale, allocator), allocator);
        doc.AddMember("WorldTranslation", SerializeVector3TOJSONValue(worldTranslation, allocator), allocator);
        doc.AddMember("WorldRotation", SerializeVector3TOJSONValue(worldRotation, allocator), allocator);

        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        doc.Accept(writer);
        return strbuf.GetString();
    }

    void TransformDefectReportingComponent::OnCollectDefectReporterData(int reportID)
    {
        int handlerID = CloudGemDefectReporter::INVALID_ID;
        CloudGemDefectReporterRequestBus::BroadcastResult(handlerID, &CloudGemDefectReporterRequestBus::Events::GetHandlerID, reportID);

        AZ::Transform localTM;
        AZ::Transform worldTM;

        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::GetLocalAndWorld, localTM, worldTM);

        DefectReport::MetricsList metrics;

        MetricDesc transformMetricDesc;
        transformMetricDesc.m_key = "transform";
        transformMetricDesc.m_data = SerializeTransformToJSONString(m_identifier, localTM, worldTM);
        metrics.push_back(transformMetricDesc);

        {
            MetricDesc translationXMetricDesc;
            double translationxValue = worldTM.GetTranslation().GetX();

            translationXMetricDesc.m_key = "translationx";
            translationXMetricDesc.m_data = AZStd::to_string(translationxValue);
            translationXMetricDesc.m_doubleVal = translationxValue;
            metrics.push_back(translationXMetricDesc);
        }

        {
            MetricDesc translationYMetricDesc;
            double translationyValue = worldTM.GetTranslation().GetY();

            translationYMetricDesc.m_key = "translationy";
            translationYMetricDesc.m_data = AZStd::to_string(translationyValue);
            translationYMetricDesc.m_doubleVal = translationyValue;
            metrics.push_back(translationYMetricDesc);
        }

        {
            MetricDesc translationZMetricDesc;
            double translationzValue = worldTM.GetTranslation().GetZ();

            translationZMetricDesc.m_key = "translationz";
            translationZMetricDesc.m_data = AZStd::to_string(translationzValue);
            translationZMetricDesc.m_doubleVal = translationzValue;
            metrics.push_back(translationZMetricDesc);
        }

        CloudGemDefectReporterRequestBus::Broadcast(&CloudGemDefectReporterRequestBus::Events::ReportData,
            reportID,
            handlerID,
            metrics,
            DefectReport::AttachmentList{});

    }

}

