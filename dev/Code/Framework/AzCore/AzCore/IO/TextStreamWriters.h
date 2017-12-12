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
#ifndef AZCORE_IO_TEXTSTREAMWRITERS_H
#define AZCORE_IO_TEXTSTREAMWRITERS_H

#include <AzCore/base.h>
#include <AzCore/IO/GenericStreams.h>

namespace AZ
{
    namespace IO
    {
        /**
         * For use with rapidxml::print.
         */
        class RapidXMLStreamWriter
        {
        public:
            RapidXMLStreamWriter(IO::GenericStream* stream)
                : m_stream(stream)
                , m_errorCount(0) {}

            RapidXMLStreamWriter& operator*() { return *this; }
            RapidXMLStreamWriter& operator++() { return *this; }
            RapidXMLStreamWriter& operator++(int) { return *this; }
            RapidXMLStreamWriter& operator=(char c)
            {
                IO::SizeType bytes = m_stream->Write(1, &c);
                if (bytes != 1)
                {
                    AZ_Error("Serializer", false, "Failed writing 1 byte to stream!");
                    ++m_errorCount;
                }
                return *this;
            }

            IO::GenericStream* m_stream;
            int m_errorCount;
        };

        /**
         * For use with rapidxml::Writer/PrettyWriter.
         */
        class RapidJSONStreamWriter
        {
        public:
            typedef char Ch;    //!< Character type. Only support char.

            RapidJSONStreamWriter(AZ::IO::GenericStream* stream)
                : m_stream(stream)
            {
            }

            RapidJSONStreamWriter(const RapidJSONStreamWriter&) = delete;
            RapidJSONStreamWriter& operator=(const RapidJSONStreamWriter&) = delete;

            void Put(char c)
            {
                m_stream->Write(1, &c);
            }

            void PutN(char c, size_t n)
            {
                while (n > 0)
                {
                    m_stream->Write(1, &c);
                    --n;
                }
            }

            void Flush()
            {
            }

            // Not implemented
            char Peek() const
            {
                AZ_Assert(false, "RapidJSONStreamWriter Peek not supported.");
                return 0;
            }

            char Take()
            {
                AZ_Assert(false, "RapidJSONStreamWriter Take not supported.");
                return 0;
            }
            size_t Tell() const
            {
                AZ_Assert(false, "RapidJSONStreamWriter Tell not supported.");
                return 0;
            }
            char* PutBegin()
            {
                AZ_Assert(false, "RapidJSONStreamWriter PutBegin not supported.");
                return 0;
            }
            size_t PutEnd(char*)
            {
                AZ_Assert(false, "RapidJSONStreamWriter PutEnd not supported.");
                return 0;
            }

            AZ::IO::GenericStream* m_stream;
        };
    }   // namespace IO
}   // namespace AZ

#endif  // AZCORE_IO_TEXTSTREAMWRITERS_H
#pragma once
