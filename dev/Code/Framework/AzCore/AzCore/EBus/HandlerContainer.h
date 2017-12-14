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

#include <AzCore/std/containers/intrusive_set.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/typetraits/is_same.h>

#include <AzCore/EBus/Policies.h>

namespace AZ
{
    namespace Internal
    {
        template <class ContainerImpl, class Interface, class Traits>
        struct HandlerContainerBase
        {
            using BusIdType = typename Traits::BusIdType;
            using BusIdOrderCompare = typename Traits::BusIdOrderCompare;
            using AllocatorType = typename Traits::AllocatorType;
            using pointer = AZStd::intrusive_ptr<ContainerImpl>;

            struct hash
                : public AZStd::unary_function<ContainerImpl, AZStd::size_t> 
            {
                AZ_FORCE_INLINE AZStd::size_t operator()(const BusIdType& busId) const { return AZStd::hash<BusIdType>()(busId); }
                AZ_FORCE_INLINE AZStd::size_t operator()(const ContainerImpl& v) const { return AZStd::hash<BusIdType>()(v.m_busId); }
            };
            struct equal_to
                : public AZStd::binary_function<ContainerImpl, ContainerImpl, bool> 
            {
                AZ_FORCE_INLINE bool operator()(const ContainerImpl& left, const ContainerImpl& right) const { return left.m_busId == right.m_busId; }
                AZ_FORCE_INLINE bool operator()(const BusIdType& leftBusId, const ContainerImpl& right) const { return leftBusId == right.m_busId; }
                AZ_FORCE_INLINE bool operator()(const ContainerImpl& left, const BusIdType& rightBusId) const { return left.m_busId == rightBusId; }
            };
            struct ConvertFromBusId 
            {
                typedef BusIdType   key_type;
                const key_type&  to_key(const BusIdType&busId) const { return busId; }
                ContainerImpl    to_value(BusIdType busId) const { return ContainerImpl(busId); }
            };

            HandlerContainerBase(BusIdType id = 0)
                : m_busId(id)
                , m_refCount(0)
            {
            }

            HandlerContainerBase(const HandlerContainerBase& rhs)
                : m_busId(rhs.m_busId)
                , m_refCount(static_cast<unsigned int>(rhs.m_refCount))
            {}

            HandlerContainerBase(HandlerContainerBase&& rhs)
                : m_busId(rhs.m_busId)
                , m_refCount(static_cast<unsigned int>(rhs.m_refCount))
            {
                rhs.m_refCount = 0;
            }

            AZ_FORCE_INLINE operator BusIdType() const { return m_busId; }
            void add_ref()
            {
                ++m_refCount;
            }
            void release();

            BusIdType    m_busId;
            AZStd::atomic_uint m_refCount;
        };

        template <class Interface, class Traits, AZ::EBusHandlerPolicy = Traits::HandlerPolicy>
        struct HandlerContainer;

        template <class Interface, class Traits>
        struct HandlerContainer<Interface, Traits, EBusHandlerPolicy::Single>
            : public HandlerContainerBase<HandlerContainer<Interface, Traits>, Interface, Traits>
        {
            using HandlerNode = Interface*;
            using Base = HandlerContainerBase<HandlerContainer<Interface, Traits>, Interface, Traits>;
            using BusIdType = typename Base::BusIdType;
            using iterator = Interface**;
            using reverse_iterator = Interface**;

            Interface*  m_eventGroup;
            Interface** m_eventGroupEnd;

            HandlerContainer(BusIdType id = 0)
                : Base(id)
                , m_eventGroup(0)
            {
                m_eventGroupEnd = &m_eventGroup;
            };

            HandlerContainer(const HandlerContainer& rhs)
                : Base(rhs)
                , m_eventGroup(rhs.m_eventGroup)
            {
                m_eventGroupEnd = &m_eventGroup + rhs.size();
            }

            HandlerContainer(HandlerContainer&& rhs)
                : Base(AZStd::forward<HandlerContainer>(rhs))
                , m_eventGroup(rhs.m_eventGroup)
            {
                m_eventGroupEnd = &m_eventGroup + rhs.size();
                // invalidate the source
                rhs.m_eventGroupEnd = &rhs.m_eventGroup;
            }

            iterator begin() { return &m_eventGroup; }
            iterator end() { return m_eventGroupEnd; }
            reverse_iterator rbegin() { return &m_eventGroup; }
            reverse_iterator rend() { return m_eventGroupEnd; }
            AZStd::size_t size() { return m_eventGroupEnd - &m_eventGroup; }
            void insert(HandlerNode node) 
            { 
                AZ_Assert(size() == 0, "Bus is already connected!"); 
                m_eventGroup = node; 
                m_eventGroupEnd = &m_eventGroup + 1; 
            }
            void erase(HandlerNode node) 
            { 
                (void)node; 
                AZ_Assert(m_eventGroup == node, "You are passing a different event group pointer!"); 
                m_eventGroupEnd = &m_eventGroup; 
            }
        };

        template <class ContainerNode, class InterfaceType>
        struct MultiHandlerNode
            : public ContainerNode
        {
            InterfaceType*  m_handler;

            inline InterfaceType* operator->() const
            {
                return m_handler;
            }

            inline operator InterfaceType*() const
            {
                return m_handler;
            }
        };

        template <class Interface, class Traits>
        struct HandlerContainer<Interface, Traits, EBusHandlerPolicy::Multiple>
            : public HandlerContainerBase<HandlerContainer<Interface, Traits>, Interface, Traits>
        {
            struct HandlerNode
                : public MultiHandlerNode<AZStd::intrusive_list_node<HandlerNode>, Interface>
            {
                inline HandlerNode& operator=(Interface* handler)
                {
                    this->m_handler = handler;
                    return *this;
                }
            };

            using Base = HandlerContainerBase<HandlerContainer<Interface, Traits>, Interface, Traits>;
            using BusIdType = typename Base::BusIdType;
            using ListType = AZStd::intrusive_list<HandlerNode, AZStd::list_base_hook<HandlerNode>>;
            using iterator = typename ListType::iterator;
            using reverse_iterator = typename ListType::reverse_iterator;  // technically we can use the just iterator as the list is unsorted!

            ListType  m_handlers;

            HandlerContainer(BusIdType id = 0)
                : Base(id)
            {}

            iterator begin() { return m_handlers.begin(); }
            iterator end() { return m_handlers.end(); }
            reverse_iterator rbegin() { return m_handlers.rbegin(); }
            reverse_iterator rend() { return m_handlers.rend(); }
            AZStd::size_t size() { return m_handlers.size(); }
            void  insert(HandlerNode& node) { m_handlers.push_front(node); } // always insert first so we don't call till next message
            void  erase(HandlerNode& node) { m_handlers.erase(node); }
        };

        /**
        * This is the default bus event compare operator. If used it your bus traits and
        * you use ordered handlers (HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered) you will need to implement
        * a function 'bool Compare(const MessageBus* rhs) const' in you message handler.
        */
        struct BusHandlerCompareDefault;

        template<class Interface>
        struct BusHandlerCompareDefaultImpl
            : public AZStd::binary_function<Interface*, Interface*, bool>
        {
            AZ_FORCE_INLINE bool operator()(const Interface* left, const Interface* right) const { return left->Compare(right); }
        };

        template <class Interface, class Traits>
        struct HandlerContainer<Interface, Traits, EBusHandlerPolicy::MultipleAndOrdered>
            : public HandlerContainerBase<HandlerContainer<Interface, Traits>, Interface, Traits>
        {
            struct HandlerNode
                : public MultiHandlerNode<AZStd::intrusive_multiset_node<HandlerNode>, Interface>
            {
                inline HandlerNode& operator=(Interface* handler)
                {
                    this->m_handler = handler;
                    return *this;
                }
            };

            typedef typename AZStd::Utils::if_c<AZStd::is_same<typename Traits::BusHandlerOrderCompare, BusHandlerCompareDefault>::value,
                Internal::BusHandlerCompareDefaultImpl<Interface>, typename Traits::BusHandlerOrderCompare>::type HandlerOrderCompare;

            using Base = HandlerContainerBase<HandlerContainer<Interface, Traits>, Interface, Traits>;
            using BusIdType = typename Base::BusIdType;
            using SetType = AZStd::intrusive_multiset<HandlerNode, AZStd::intrusive_multiset_base_hook<HandlerNode>, HandlerOrderCompare>;
            using iterator = typename SetType::iterator;
            using reverse_iterator = typename SetType::reverse_iterator;

            SetType m_set;

            HandlerContainer(BusIdType id = 0)
                : Base(id)
            {}

            iterator begin() { return m_set.begin(); }
            iterator end() { return m_set.end(); }
            reverse_iterator rbegin() { return m_set.rbegin(); }
            reverse_iterator rend() { return m_set.rend(); }
            AZStd::size_t size() { return m_set.size(); }
            void  insert(HandlerNode& node) { m_set.insert(node); }
            void  erase(HandlerNode& node) { m_set.erase(node); }
        };
    } // namespace Internal
} // namespace AZ
