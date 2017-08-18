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
#ifndef AZ_EBUS_HANDLER_CONTAINER_H
#define AZ_EBUS_HANDLER_CONTAINER_H

#include <AzCore/std/containers/intrusive_set.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/typetraits/is_same.h>

namespace AZ
{
#define AZ_EBUS_CONTAINER_COMMON(_EB_CONTAINER_IMPL, _Interface, _Traits)                                                                                \
    typedef typename _Traits::BusIdType BusIdType;                                                                                                       \
    typedef typename _Traits::BusIdOrderCompare BusIdOrderCompare;                                                                                       \
    typedef typename _Traits::AllocatorType AllocatorType;                                                                                               \
    typedef _Interface InterfaceType;                                                                                                                    \
    BusIdType    m_busId;                                                                                                                                \
    unsigned int m_refCount;                                                                                                                             \
    struct hash                                                                                                                                          \
        : public AZStd::unary_function<_EB_CONTAINER_IMPL, AZStd::size_t> {                                                                              \
        AZ_FORCE_INLINE AZStd::size_t operator()(const BusIdType& busId) const { return AZStd::hash<BusIdType>()(busId); }                               \
        AZ_FORCE_INLINE AZStd::size_t operator()(const _EB_CONTAINER_IMPL& v) const { return AZStd::hash<BusIdType>()(v.m_busId); }                      \
    };                                                                                                                                                   \
    struct equal_to                                                                                                                                      \
        : public AZStd::binary_function<_EB_CONTAINER_IMPL, _EB_CONTAINER_IMPL, bool> {                                                                  \
        AZ_FORCE_INLINE bool operator()(const _EB_CONTAINER_IMPL& left, const _EB_CONTAINER_IMPL& right) const { return left.m_busId == right.m_busId; } \
        AZ_FORCE_INLINE bool operator()(const BusIdType& leftBusId, const _EB_CONTAINER_IMPL& right) const { return leftBusId == right.m_busId; }        \
        AZ_FORCE_INLINE bool operator()(const _EB_CONTAINER_IMPL& left, const BusIdType& rightBusId) const { return left.m_busId == rightBusId; }        \
    };                                                                                                                                                   \
    struct ConvertFromBusId {                                                                                                                            \
        typedef BusIdType   key_type;                                                                                                                    \
        const key_type&     to_key(const BusIdType&busId) const { return busId; }                                                                        \
        _EB_CONTAINER_IMPL  to_value(BusIdType busId) const { return _EB_CONTAINER_IMPL(busId); }                                                        \
    };                                                                                                                                                   \
    AZ_FORCE_INLINE operator BusIdType() const { return m_busId; }                                                                                       \
    void add_ref();                                                                                                                                      \
    void release();                                                                                                                                      \
    void lock();                                                                                                                                         \
    void unlock();                                                                                                                                       \
    typedef AZStd::intrusive_ptr<_EB_CONTAINER_IMPL>  pointer;

    /// @cond EXCLUDE

    template<class Interface, class Traits>
    struct EBECSingle
    {
        AZ_EBUS_CONTAINER_COMMON(EBECSingle, Interface, Traits)
        InterfaceType *  m_eventGroup;
        InterfaceType** m_eventGroupEnd;
        EBECSingle(BusIdType id = 0)
            : m_busId(id)
            , m_refCount(0)
            , m_eventGroup(0)
        {
            m_eventGroupEnd = &m_eventGroup;
        };

        EBECSingle(const EBECSingle& rhs)
            : m_busId(rhs.m_busId)
            , m_refCount(0)
            , m_eventGroup(rhs.m_eventGroup)
        {
            if (rhs.m_eventGroupEnd == &rhs.m_eventGroup)
            {
                m_eventGroupEnd = &m_eventGroup;
            }
            else
            {
                m_eventGroupEnd = &m_eventGroup + 1;
            }
        }

        EBECSingle(EBECSingle&& rhs)
            : m_busId(rhs.m_busId)
            , m_refCount(rhs.m_refCount)
            , m_eventGroup(rhs.m_eventGroup)
        {
            if (rhs.m_eventGroupEnd == &rhs.m_eventGroup)
            {
                m_eventGroupEnd = &m_eventGroup;
            }
            else
            {
                m_eventGroupEnd = &m_eventGroup + 1;
            }

            // invalidate the source
            rhs.m_eventGroupEnd = &rhs.m_eventGroup;
            rhs.m_refCount = 0;
        }

        typedef InterfaceType*  HandlerNode;

        typedef InterfaceType** iterator;
        typedef InterfaceType** reverse_iterator;


        iterator begin()                    { return &m_eventGroup; }
        iterator end()                      { return m_eventGroupEnd; }
        reverse_iterator rbegin()           { return &m_eventGroup; }
        reverse_iterator rend()             { return m_eventGroupEnd; }
        AZStd::size_t size()                { return m_eventGroupEnd == &m_eventGroup ? 0 : 1; }
        void insert(HandlerNode node)       { AZ_Assert(size() == 0, "Bus is already connected!"); m_eventGroup = node; m_eventGroupEnd = &m_eventGroup + 1; }
        void erase(HandlerNode node)        { (void)node; AZ_Assert(m_eventGroup == node, "You are passing a different event group pointer!"); m_eventGroupEnd = &m_eventGroup; }
    };

    template<class Interface, class Traits>
    struct EBECMulti
    {
        AZ_EBUS_CONTAINER_COMMON(EBECMulti, Interface, Traits)

        struct HandlerNode
            : public AZStd::intrusive_list_node<HandlerNode>
        {
            InterfaceType*  m_handler;

            inline HandlerNode& operator=(InterfaceType* handler)
            {
                m_handler = handler;
                return *this;
            }

            inline InterfaceType* operator->() const
            {
                return m_handler;
            }

            inline operator InterfaceType*() const
            {
                return m_handler;
            }
        };

        typedef AZStd::intrusive_list<HandlerNode, AZStd::list_base_hook<HandlerNode> > ListType;
        ListType  m_handlers;

        EBECMulti(BusIdType id = 0)
            : m_busId(id)
            , m_refCount(0)
        {}

        typedef typename ListType::iterator iterator;
        typedef typename ListType::reverse_iterator reverse_iterator;  // technically we can use the just iterator as the list is unsorted!

        iterator begin()                { return m_handlers.begin(); }
        iterator end()                  { return m_handlers.end(); }
        reverse_iterator rbegin()       { return m_handlers.rbegin(); }
        reverse_iterator rend()         { return m_handlers.rend(); }
        AZStd::size_t size()            { return m_handlers.size(); }
        void  insert(HandlerNode& node){ m_handlers.push_front(node);   } // always insert first so we don't call till next message
        void  erase(HandlerNode& node)  { m_handlers.erase(node);   }
    };


    /**
    * This is the default bus event compare operator. If used it your bus traits and
    * you use ordered handlers (HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered) you will need to implement
    * a function 'bool Compare(const MessageBus* rhs) const' in you message handler.
    */
    struct BusHandlerCompareDefault;

    namespace Internal
    {
        template<class Interface>
        struct BusHandlerCompareDefaultImpl
            : public AZStd::binary_function<Interface*, Interface*, bool>
        {
            AZ_FORCE_INLINE bool operator()(const Interface* left, const Interface* right) const { return left->Compare(right); }
        };
    }

    template<class Interface, class Traits>
    struct EBECMultiOrdered
    {
        AZ_EBUS_CONTAINER_COMMON(EBECMultiOrdered, Interface, Traits)

        struct HandlerNode
            : public AZStd::intrusive_multiset_node<HandlerNode>
        {
            InterfaceType*  m_handler;

            inline HandlerNode& operator=(InterfaceType* handler)
            {
                m_handler = handler;
                return *this;
            }

            inline InterfaceType* operator->() const
            {
                return m_handler;
            }

            inline operator InterfaceType*() const
            {
                return m_handler;
            }
        };

        typedef typename AZStd::Utils::if_c<AZStd::is_same<typename Traits::BusHandlerOrderCompare, BusHandlerCompareDefault>::value,
            Internal::BusHandlerCompareDefaultImpl<InterfaceType>, typename Traits::BusHandlerOrderCompare>::type HandlerOrderCompare;

        typedef AZStd::intrusive_multiset<HandlerNode, AZStd::intrusive_multiset_base_hook<HandlerNode>, HandlerOrderCompare> SetType;
        typedef typename SetType::iterator          iterator;
        typedef typename SetType::reverse_iterator  reverse_iterator;

        SetType m_set;

        EBECMultiOrdered(BusIdType id = 0 /*, const typename Traits::AllocatorType& alloc*/)
            : m_busId(id)
            , m_refCount(0)
        {}

        iterator begin()            { return m_set.begin(); }
        iterator end()              { return m_set.end(); }
        reverse_iterator rbegin()   { return m_set.rbegin(); }
        reverse_iterator rend()     { return m_set.rend(); }
        AZStd::size_t size()        { return m_set.size(); }
        void  insert(HandlerNode& node) { m_set.insert(node); }
        void  erase(HandlerNode& node)   { m_set.erase(node); }
    };

    /// @endcond

    // End of Event Containers
    //////////////////////////////////////////////////////////////////////////
}
#endif //AZ_EBUS_HANDLER_CONTAINER_H
#pragma once