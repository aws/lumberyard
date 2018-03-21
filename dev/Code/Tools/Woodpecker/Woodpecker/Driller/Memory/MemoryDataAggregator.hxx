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

#ifndef DRILLER_MEMORY_DATAAGGREGATOR_TESTS_H
#define DRILLER_MEMORY_DATAAGGREGATOR_TESTS_H

#include "Woodpecker/Driller/DrillerAggregator.hxx"
#include "Woodpecker/Driller/DrillerAggregatorOptions.hxx"

#include <AzCore/RTTI/RTTI.h>

#include "MemoryDataParser.h"
#include "MemoryDataView.hxx"

namespace Driller
{
    namespace Memory
    {
        struct AllocationInfo;
        struct AllocatorInfo;
    }
    class MemoryDataAggregatorSavedState;

    /**
     * Memory data drilling aggregator.
     */
    class MemoryDataAggregator : public Aggregator
    {
        friend class MemoryDataView;
		MemoryDataAggregator(const MemoryDataAggregator&) = delete;
        Q_OBJECT;
    public:
        AZ_RTTI(MemoryDataAggregator, "{18589F5B-B9F0-4893-90E7-95C6E08DF798}");
        AZ_CLASS_ALLOCATOR(MemoryDataAggregator,AZ::SystemAllocator,0);

        MemoryDataAggregator(int identity = 0);
        virtual ~MemoryDataAggregator();

        static AZ::u32 DrillerId()											{ return MemoryDrillerHandlerParser::GetDrillerId(); }
        virtual AZ::u32 GetDrillerId() const override                       { return DrillerId(); }

        static const char* ChannelName() { return "Memory"; }        
        virtual AZ::Crc32 GetChannelId() const override { return AZ::Crc32(ChannelName()); }

        virtual AZ::Debug::DrillerHandlerParser* GetDrillerDataParser() 	{ return &m_parser; }

        virtual void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        virtual void ActivateWorkspaceSettings(WorkspaceSettingsProvider *);
        virtual void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);

        void KillAllViews();

        //////////////////////////////////////////////////////////////////////////
        // Aggregator
        virtual void Reset();
    public slots:
        float ValueAtFrame( FrameNumberType frame ) override;
        QColor GetColor() const override;        
        QString GetChannelName() const override;
        QString GetName() const override;
        QString GetDescription() const override;
        QString GetToolTip() const override;
        AZ::Uuid GetID() const override;
        QWidget* DrillDownRequest(FrameNumberType frame) override;
        void OptionsRequest() override;
        void OnDataViewDestroyed(QObject*);
    
    //protected:
    public:
        typedef AZStd::vector<Memory::AllocatorInfo*> AllocatorInfoArrayType;

        AllocatorInfoArrayType::iterator FindAllocatorById(AZ::u64 id);
        AllocatorInfoArrayType::iterator FindAllocatorByRecordsId(AZ::u64 recordsId);
        AllocatorInfoArrayType::iterator GetAllocatorEnd() { return m_allocators.end(); }

        AllocatorInfoArrayType				m_allocators;		///< Current state of allocators
        MemoryDrillerHandlerParser			m_parser;			///< Parser for this aggregator

        typedef AZStd::unordered_map<Driller::MemoryDataView*,AZ::u32> DataViewMap;
        DataViewMap												m_dataViews;	/// track active dialog indexes
        AZStd::intrusive_ptr<MemoryDataAggregatorSavedState>	m_persistentState;

        static void Reflect(AZ::ReflectContext* context);
    };

}

#endif
