// Copyright 2017 FragLab Ltd. All rights reserved.
#pragma once

#include "StringTable.h"

#include <AzCore/Component/Component.h>

#include <Utils/CVars/AutoCVars.h>
#include <Utils/CVars/AutoCommand.h>

namespace FragLab
{
    namespace Marshalers
    {
        class MarshalersSystemComponent
            : public AZ::Component
            , protected AutoCVars<MarshalersSystemComponent>
        {
            using MarshalersCommand = AutoCommand<MarshalersSystemComponent>;

        public:
            AZ_COMPONENT(MarshalersSystemComponent, "{DE0CA094-FF8F-479F-8F21-14454C023D5A}");

            static void Reflect(AZ::ReflectContext* context) {}

        protected:
            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override {}
            void Activate() override {}
            void Deactivate() override {}
            ////////////////////////////////////////////////////////////////////////

        private:
            static void CmdMarshalersTest(IConsoleCmdArgs* pArg);

        private:
            CStringTable m_netStringTable;

            MarshalersCommand marshalers_test { "marshalers_test", CmdMarshalersTest, VF_NULL,
                                                "Test predefined marshalers.\n"
                                                "Execute marshal/unmarshal with different values for each marshaler many times.\n"
                                                "Collect statistics (minimum, maximum and average) about memory usage, deviation from target value and process time.\n"
                                                "Params:\n"
                                                "'c' - container marshalers\n"
                                                "'d' - direction marshalers\n"
                                                "'e' - enum marshaler\n"
                                                "'f' - float marshaler\n"
                                                "'i' - int marshaler\n"
                                                "'p' - position marshalers\n"
                                                "'r' - rotation marshalers\n"
                                                "'s' - string marshalers\n"
                                                "'cdefiprs' or empty - all" };
        };
    } // namespace Marshalers
}     // namespace FragLab
