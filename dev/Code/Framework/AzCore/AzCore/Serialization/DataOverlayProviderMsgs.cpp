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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Serialization/DataOverlayProviderMsgs.h>

namespace AZ
{
    //-------------------------------------------------------------------------
    // DataOverlayTarget
    //-------------------------------------------------------------------------
    void DataOverlayTarget::Parse(const void* classPtr, const SerializeContext::ClassData* classData)
    {
        AZStd::vector<SerializeContext::DataElementNode*> nodeStack;
        nodeStack.push_back(m_dataContainer);
        m_sc->EnumerateInstanceConst(classPtr
            , classData->m_typeId
            , AZStd::bind(&DataOverlayTarget::ElementBegin, this, &nodeStack, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3)
            , AZStd::bind(&DataOverlayTarget::ElementEnd, this, &nodeStack)
            , SerializeContext::ENUM_ACCESS_FOR_READ
            , classData
            , nullptr
            , m_errorLogger
            );
    }
    //-------------------------------------------------------------------------
    bool DataOverlayTarget::ElementBegin(NodeStack* nodeStack, const void* elemPtr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* elementData)
    {
        nodeStack->back()->m_subElements.push_back();
        SerializeContext::DataElementNode* node = &nodeStack->back()->m_subElements.back();
        nodeStack->push_back(node);

        node->m_classData = classData;
        node->m_element.m_id = classData->m_typeId;
        node->m_element.m_version = classData->m_version;
        if (elementData)
        {
            node->m_element.m_name = elementData->m_name;
            node->m_element.m_nameCrc = elementData->m_nameCrc;
        }
        if (classData->m_serializer)
        {
            node->m_element.m_dataSize = classData->m_serializer->Save(elemPtr, node->m_element.m_byteStream);
            node->m_element.m_dataType = SerializeContext::DataElement::DT_BINARY;
        }
        else
        {
            node->m_element.m_dataSize = 0;
        }
        return true;
    }
    //-------------------------------------------------------------------------
    bool DataOverlayTarget::ElementEnd(NodeStack* nodeStack)
    {
        nodeStack->pop_back();
        return true;
    }
    //-------------------------------------------------------------------------
}   // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
