#pragma once
#ifndef __CCRYDX12INPUTLAYOUT__
#define __CCRYDX12INPUTLAYOUT__

#include "DX12/Device/CCryDX12DeviceChild.hpp"

class CCryDX12InputLayout
    : public CCryDX12DeviceChild<ID3D11InputLayout>
{
public:
    DX12_OBJECT(CCryDX12InputLayout, CCryDX12DeviceChild<ID3D11InputLayout>);

    static CCryDX12InputLayout* Create(CCryDX12Device* pDevice, const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs11, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength);

    virtual ~CCryDX12InputLayout();

    typedef std::vector<D3D12_INPUT_ELEMENT_DESC> TDescriptors;

    const TDescriptors& GetDescriptors() const
    {
        return m_Descriptors;
    }

protected:
    CCryDX12InputLayout();

private:
    TDescriptors m_Descriptors;

    std::vector<std::string> m_SemanticNames;
};

#endif // __CCRYDX12INPUTLAYOUT__
