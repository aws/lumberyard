// Copyright 2017 FragLab Ltd. All rights reserved.

#include "StdAfx.h"
#include "MarshalersSystemComponent.h"
#include "MarshalerTest.h"

namespace FragLab
{
    namespace Marshalers
    {
        void MarshalersSystemComponent::CmdMarshalersTest(IConsoleCmdArgs* pArg)
        {
            Test::MarshalersTest(pArg->GetArgCount() > 1 ? pArg->GetArg(1) : nullptr);
        }
    }
}
