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

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/set.h>

#include <AzCore/EBus/Policies.h>

namespace AZ
{
    namespace Internal
    {
        template <class EBNode, EBusAddressPolicy>
        struct EBusContainer;

        /**
        * Single BUS type.
        * This EventGroup that uses EBNode will use only one BUS.
        * \note the EBNode bus id is set to default value 0.
        */
        template <class EBNode>
        struct EBusContainer<EBNode, EBusAddressPolicy::Single>
        {
            typedef typename EBNode::BusIdType BusIdType;

            EBNode* m_node = nullptr;
            typename AZStd::aligned_storage<sizeof(EBNode), AZStd::alignment_of<EBNode>::value>::type m_buffer;
            typedef EBNode* iterator;
            typedef EBNode* reverse_iterator;

            iterator insert(BusIdType id);
            iterator find(BusIdType id);
            void erase(EBNode& node);

            void KeepIteratorsStable();
            bool IsKeepIteratorsStable() const;
            void AllowUnstableIterators();

            iterator begin();
            iterator end();

            reverse_iterator rbegin();
            reverse_iterator rend();

            AZStd::size_t size() const;

            static EBNode* toNodePtr(iterator& iter);
        };

        /**
        * Multi BUS type.
        * This EventGroup will use multiple communication buses. The order of
        * them is undefined.
        */
        template <class EBNode>
        struct EBusContainer<EBNode, EBusAddressPolicy::ById>
        {
            typedef typename EBNode::BusIdType BusIdType;

            typedef AZStd::unordered_set<EBNode, typename EBNode::hash, typename EBNode::equal_to, typename EBNode::AllocatorType> BusSetType;
            BusSetType  m_buses;

            typedef typename BusSetType::iterator iterator;
            typedef typename BusSetType::reverse_iterator reverse_iterator; // technically we can use the just iterator as the list is unsorted!

            iterator insert(BusIdType id);
            iterator find(BusIdType id);
            void erase(EBNode& bus);

            void KeepIteratorsStable();
            bool IsKeepIteratorsStable() const;
            void AllowUnstableIterators();

            iterator begin();
            iterator end();
            reverse_iterator rbegin();
            reverse_iterator rend();
            AZStd::size_t size() const;

            static EBNode* toNodePtr(iterator& iter);
        };

        /**
        * Multi BUS ordered type.
        * This event group will use multiple buses and they will sorted as the user is requesting.
        */
        template <class EBNode>
        struct EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>
        {
            typedef typename EBNode::BusIdType BusIdType;
            typedef AZStd::set<EBNode, typename EBNode::BusIdOrderCompare, typename EBNode::AllocatorType> BusOrderedListType;
            typedef typename BusOrderedListType::iterator iterator;
            typedef typename BusOrderedListType::reverse_iterator reverse_iterator;

            BusOrderedListType m_orderList;

            iterator insert(BusIdType id);
            iterator find(BusIdType id);
            void erase(EBNode& bus);

            void KeepIteratorsStable();
            bool IsKeepIteratorsStable() const;
            void AllowUnstableIterators();

            iterator begin();
            iterator end();
            reverse_iterator rbegin();
            reverse_iterator rend();
            AZStd::size_t size() const;

            static EBNode* toNodePtr(iterator& iter);
        };

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::Single>::iterator EBusContainer<EBNode, EBusAddressPolicy::Single>::insert(BusIdType id)
        {
            (void)id;
            if (m_node == nullptr)  // we have destroyed the object
            {
                m_node = new(&m_buffer)EBNode(0);
            }
            return m_node;
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::Single>::iterator EBusContainer<EBNode, EBusAddressPolicy::Single>::find(BusIdType id)
        {
            (void)id;
            return m_node;
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::Single>::erase(EBNode& node)
        {
            (void)node;
            if (m_node)
            {
                AZ_Assert(m_node == &node, "We have only one bus for this event. And this is NOT it!");
                m_node->~EBNode();
                m_node = nullptr;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::Single>::KeepIteratorsStable() 
        {
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        bool EBusContainer<EBNode, EBusAddressPolicy::Single>::IsKeepIteratorsStable() const 
        { 
            return false; 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::Single>::AllowUnstableIterators() 
        {
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::Single>::iterator EBusContainer<EBNode, EBusAddressPolicy::Single>::begin() 
        { 
            return m_node; 
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::Single>::iterator EBusContainer<EBNode, EBusAddressPolicy::Single>::end() 
        {
            return m_node ? m_node + 1 : m_node; 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::Single>::reverse_iterator EBusContainer<EBNode, EBusAddressPolicy::Single>::rbegin() 
        { 
            return m_node; 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::Single>::reverse_iterator EBusContainer<EBNode, EBusAddressPolicy::Single>::rend()
        { 
            return m_node ? m_node + 1 : m_node; 
        }

        template <class EBNode>
        AZStd::size_t EBusContainer<EBNode, EBusAddressPolicy::Single>::size() const 
        { 
            return m_node ? 1 : 0; 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        EBNode* EBusContainer<EBNode, EBusAddressPolicy::Single>::toNodePtr(iterator& iter) 
        { 
            return iter; 
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ById>::iterator EBusContainer<EBNode, EBusAddressPolicy::ById>::insert(BusIdType id)
        {
            return m_buses.insert_from(id, typename EBNode::ConvertFromBusId(), typename EBNode::hash(), typename EBNode::equal_to()).first;
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ById>::iterator EBusContainer<EBNode, EBusAddressPolicy::ById>::find(BusIdType id)
        {
            return m_buses.find_as(id, typename EBNode::hash(), typename EBNode::equal_to());
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::ById>::erase(EBNode& bus)
        {
            m_buses.erase(bus);
            if (m_buses.empty())   // free all the memory if we are empty
            {
                m_buses.rehash(0);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::ById>::KeepIteratorsStable() 
        { 
            m_buses.max_load_factor(100000.f);         /// Don't allow the hash table to rehash on insert (just allow high load factor).
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        bool EBusContainer<EBNode, EBusAddressPolicy::ById>::IsKeepIteratorsStable() const 
        { 
            return m_buses.max_load_factor() == 100000.f; 
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::ById>::AllowUnstableIterators()
        { 
            m_buses.max_load_factor(BusSetType::traits_type::max_load_factor); /// Reset load factor to default value and rehash if needed
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ById>::iterator EBusContainer<EBNode, EBusAddressPolicy::ById>::begin() 
        { 
            return m_buses.begin(); 
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ById>::iterator EBusContainer<EBNode, EBusAddressPolicy::ById>::end()
        { 
            return m_buses.end(); 
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ById>::reverse_iterator EBusContainer<EBNode, EBusAddressPolicy::ById>::rbegin() 
        { 
            return m_buses.rbegin(); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ById>::reverse_iterator EBusContainer<EBNode, EBusAddressPolicy::ById>::rend()
        { 
            return m_buses.rend(); 
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        AZStd::size_t EBusContainer<EBNode, EBusAddressPolicy::ById>::size() const 
        { 
            return m_buses.size(); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        EBNode* EBusContainer<EBNode, EBusAddressPolicy::ById>::toNodePtr(iterator& iter) 
        { 
            return &*iter; 
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::iterator EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::insert(BusIdType id) 
        { 
            return m_orderList.insert(id).first; 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::iterator EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::find(BusIdType id)
        { 
            return m_orderList.find(id); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::erase(EBNode& bus) 
        { 
            m_orderList.erase(bus.m_busId); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::KeepIteratorsStable() 
        {
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        bool EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::IsKeepIteratorsStable() const 
        { 
            return false; 
        }
        
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        void EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::AllowUnstableIterators() 
        {
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::iterator EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::begin()
        { 
            return m_orderList.begin(); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::iterator EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::end()
        { 
            return m_orderList.end(); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::reverse_iterator EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::rbegin()
        { 
            return m_orderList.rbegin(); 
        }
        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        typename EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::reverse_iterator EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::rend() 
        { 
            return m_orderList.rend(); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        AZStd::size_t EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::size() const
        { 
            return m_orderList.size(); 
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EBNode>
        EBNode* EBusContainer<EBNode, EBusAddressPolicy::ByIdAndOrdered>::toNodePtr(iterator& iter)
        { 
            return &*iter; 
        }

    } // namespace Internal
} // namespace AZ
