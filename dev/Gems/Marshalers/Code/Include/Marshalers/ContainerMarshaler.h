// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include "IntMarshaler.h"
#include <GridMate/Serialize/Buffer.h>

namespace FragLab
{
    namespace Marshalers
    {
        template<class DataType, AZStd::size_t ExpectedMaxSize, class ValueMarshaler = GridMate::Marshaler<typename DataType::value_type>>
        class ContainerMarshaler
        {
            using SizeMarshaler = AdaptiveIntMarshaler<AZStd::size_t, 0, ExpectedMaxSize>;

        public:

            void Marshal(GridMate::WriteBuffer& wb, const DataType& container) const
            {
                m_sizeMarshaler.Marshal(wb, container.size());

                for (const auto& element : container)
                {
                    m_valueMarshaler.Marshal(wb, element);
                }
            }

            void Unmarshal(DataType& container, GridMate::ReadBuffer& rb) const
            {
                AZStd::size_t size = 0;
                m_sizeMarshaler.Unmarshal(size, rb);

                container.resize(size);
                for (AZStd::size_t i = 0; i < size; ++i)
                {
                    m_valueMarshaler.Unmarshal(container[i], rb);
                }
            }

        private:
            SizeMarshaler m_sizeMarshaler;
            ValueMarshaler m_valueMarshaler;
        };

        template<class DataType, AZStd::size_t ExpectedMaxSize, class KeyMarshaler = GridMate::Marshaler<typename DataType::key_type>, class ValueMarshaler = GridMate::Marshaler<typename DataType::mapped_type>>
        class MapMarshaler
        {
            using SizeMarshaler = AdaptiveIntMarshaler<AZStd::size_t, 0, ExpectedMaxSize>;

        public:

            void Marshal(GridMate::WriteBuffer& wb, const DataType& container) const
            {
                m_sizeMarshaler.Marshal(wb, container.size());

                for (const auto& element : container)
                {
                    m_keyMarshaler.Marshal(wb, element.first);
                    m_valueMarshaler.Marshal(wb, element.second);
                }
            }

            void Unmarshal(DataType& container, GridMate::ReadBuffer& rb) const
            {
                AZStd::size_t size = 0;
                m_sizeMarshaler.Unmarshal(size, rb);

                container.clear();
                typename DataType::key_type key;
                typename DataType::mapped_type value;
                for (AZStd::size_t i = 0; i < size; ++i)
                {
                    m_keyMarshaler.Unmarshal(key, rb);
                    m_valueMarshaler.Unmarshal(value, rb);
                    container.emplace(AZStd::make_pair(key, value));
                }
            }

        private:
            SizeMarshaler m_sizeMarshaler;
            KeyMarshaler m_keyMarshaler;
            ValueMarshaler m_valueMarshaler;
        };
    } // namespace Marshalers
}     // namespace FragLab
