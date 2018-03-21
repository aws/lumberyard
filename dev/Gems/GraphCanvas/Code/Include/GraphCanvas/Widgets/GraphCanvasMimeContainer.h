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

#include <QString>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>

namespace GraphCanvas
{
    class GraphCanvasMimeContainer
    {
    public:
        AZ_RTTI(GraphCanvasMimeContainer, "{CB8CAB35-B817-4910-AFC2-51881832591E}");
        AZ_CLASS_ALLOCATOR(GraphCanvasMimeContainer, AZ::SystemAllocator, 0);        
        static void Reflect(AZ::ReflectContext* serializeContext);
        
        GraphCanvasMimeContainer() = default;
        virtual ~GraphCanvasMimeContainer()
        {
            for (GraphCanvasMimeEvent* mimeEvent : m_mimeEvents)
            {
                delete mimeEvent;
            }
        }
        
        bool ToBuffer(AZStd::vector<char>& buffer)
        {
            buffer.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<char> > ms(&buffer);
            return AZ::Utils::SaveObjectToStream(ms, AZ::DataStream::ST_BINARY, this);
        }
        
        bool FromBuffer(const char* data, AZStd::size_t size)
        {
            AZ::IO::MemoryStream ms(data, size);

            GraphCanvasMimeContainer* pContainer = AZ::Utils::LoadObjectFromStream<GraphCanvasMimeContainer>(ms, nullptr);
            if (pContainer)
            {
                m_mimeEvents = AZStd::move(pContainer->m_mimeEvents);

                pContainer->m_mimeEvents.clear();
                delete pContainer;

                return true;
            }

            return false;
        }

        bool FromBuffer(const AZStd::vector<char>& buffer)
        {
            return FromBuffer(buffer.data(), buffer.size());
        }

        AZStd::vector< GraphCanvasMimeEvent* > m_mimeEvents;
    };
}