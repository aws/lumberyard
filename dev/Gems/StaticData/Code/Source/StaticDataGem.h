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

#include <AzCore/Module/Module.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#if defined(USE_MONITOR)
#include <Editor/StaticDataMonitor.h>
#endif

namespace CloudCanvas
{
    namespace StaticData
    {
        class IStaticDataManager;
        class StaticDataMonitor;
    }
}

namespace StaticData
{
    class StaticDataGem : public AZ::Module
    {
    public:
        StaticDataGem();
        AZ_RTTI(StaticDataGem, "{6B556073-DFCF-4A7F-B763-8C1C13936B94}", AZ::Module);

        ~StaticDataGem();

        void Initialize();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    private:

        AZStd::shared_ptr<CloudCanvas::StaticData::StaticDataMonitor> m_monitor;

    };
} // namespace StaticData
