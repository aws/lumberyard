#include "StdAfx.h"
#include "DX12SamplerState.hpp"

namespace DX12
{
    //---------------------------------------------------------------------------------------------------------------------
    SamplerState::SamplerState()
        : AzRHI::ReferenceCounted()
        , m_DescriptorHandle(INVALID_CPU_DESCRIPTOR_HANDLE)
    {
        // clear before use
        memset(&m_unSamplerDesc, 0, sizeof(m_unSamplerDesc));
    }

    //---------------------------------------------------------------------------------------------------------------------
    SamplerState::~SamplerState()
    {
    }
}
