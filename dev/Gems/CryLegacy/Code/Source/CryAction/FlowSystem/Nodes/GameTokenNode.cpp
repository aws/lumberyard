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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IGameTokens.h>

#include "CryAction.h"

namespace
{
    IGameTokenSystem* GetIGameTokenSystem()
    {
        return CCryAction::GetCryAction()->GetIGameTokenSystem();
    }

    void WarningGameTokenNotFound(const char* place, const char* tokenName)
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[flow] %s: cannot find gametoken: '%s'.", place, tokenName);
    }

    IGameToken* GetGameToken(IFlowNode::SActivationInfo* pActInfo, const string& tokenName, bool bForceCreate)
    {
        if (tokenName.empty())
        {
            return nullptr;
        }

        IGameToken* pToken = GetIGameTokenSystem()->FindToken(tokenName.c_str());
        if (!pToken)
        {
            // try graph token instead:
            string name =   pActInfo->pGraph->GetGlobalNameForGraphToken(tokenName);

            pToken = GetIGameTokenSystem()->FindToken(name.c_str());
            assert(!pToken || pToken->GetFlags() & EGAME_TOKEN_GRAPHVARIABLE);

            if (!pToken && bForceCreate)
            {
                pToken = GetIGameTokenSystem()->SetOrCreateToken(tokenName.c_str(), TFlowInputData(string("<undefined>"), false));
            }
        }

        if (!pToken && (!gEnv->IsEditor() || gEnv->IsEditorGameMode()))
        {
            WarningGameTokenNotFound("SetGameTokenFlowNode", tokenName.c_str());
        }

        return pToken;
    }

    const int bForceCreateDefault = false; // create non-existing game tokens, default false for the moment
};


//////////////////////////////////////////////////////////////////////////
class CSetGameTokenFlowNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        Token,
        TokenValue,
    };

    enum OutputPorts
    {
        OutValue,
    };

public:
    CSetGameTokenFlowNode(SActivationInfo* pActInfo)
        : m_pCachedToken(nullptr)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CSetGameTokenFlowNode(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        if (ser.IsReading()) // forces re-caching
        {
            m_pCachedToken = nullptr;
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<string> ("gametoken_Token", _HELP("Game token to set"), _HELP("Token")),
            InputPortConfig<string>("TokenValue", _HELP("Value to set")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("OutValue", _HELP("Value which was set")),
            {0}
        };
        config.sDescription = _HELP("Set the value of a game token");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= (EFLN_AISEQUENCE_SUPPORTED | EFLN_ACTIVATION_INPUT);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_pCachedToken = nullptr;
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Token))
            {
                CacheToken(pActInfo);
            }
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                if (!m_pCachedToken)
                {
                    CacheToken(pActInfo);
                }

                if (m_pCachedToken)
                {
                    m_pCachedToken->SetValue(GetPortAny(pActInfo, InputPorts::TokenValue));
                    TFlowInputData data;
                    m_pCachedToken->GetValue(data);
                    ActivateOutput(pActInfo, OutputPorts::OutValue, data);
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
    {
        string tokenName = GetPortString(pActInfo, InputPorts::Token);
        m_pCachedToken = GetGameToken(pActInfo, tokenName, bForceCreate);
    }

    IGameToken* m_pCachedToken;
};

//////////////////////////////////////////////////////////////////////////
class CGetGameTokenFlowNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        Token,
    };

    enum OutputPorts
    {
        OutValue,
    };

public:
    CGetGameTokenFlowNode(SActivationInfo* pActInfo)
        : m_pCachedToken(nullptr)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CGetGameTokenFlowNode(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        if (ser.IsReading()) // forces re-caching
        {
            m_pCachedToken = nullptr;
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<string>("gametoken_Token", _HELP("Game token to get"), _HELP("Token")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType ("OutValue", _HELP("Value of the game token")),
            {0}
        };
        config.sDescription = _HELP("Get the value of a game token");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= (EFLN_AISEQUENCE_SUPPORTED | EFLN_ACTIVATION_INPUT);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_pCachedToken = nullptr;
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Token))
            {
                CacheToken(pActInfo);
            }
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                if (!m_pCachedToken)
                {
                    CacheToken(pActInfo);
                }

                if (m_pCachedToken)
                {
                    TFlowInputData data;
                    m_pCachedToken->GetValue(data);
                    ActivateOutput(pActInfo, OutputPorts::OutValue, data);
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
    {
        string name = GetPortString(pActInfo, InputPorts::Token);
        m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
    }

    IGameToken* m_pCachedToken;
};

//////////////////////////////////////////////////////////////////////////
class CCheckGameTokenFlowNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        Token,
        CompareValue,
    };

    enum OutputPorts
    {
        TokenValue,
        Result,
        OnTrue,
        OnFalse,
    };

public:
    CCheckGameTokenFlowNode(SActivationInfo* pActInfo)
        : m_pCachedToken(nullptr)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CCheckGameTokenFlowNode(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        if (ser.IsReading()) // forces re-caching
        {
            m_pCachedToken = nullptr;
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<string>("gametoken_Token", _HELP("Game token to check"), _HELP("Token")),
            InputPortConfig<string>("CompareValue", _HELP("Value to compare the [Token] with")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("TokenValue", _HELP("Value of the [Token]")),
            OutputPortConfig<bool>("Result", _HELP("True if value of the [Token] equals [CompareValue], false otherwise")),
            OutputPortConfig_Void("OnTrue", _HELP("Activated if equal")),
            OutputPortConfig_Void("OnFalse", _HELP("Activated if not equal")),
            {0}
        };
        config.sDescription = _HELP("Check if a game token equals a value");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= (EFLN_AISEQUENCE_SUPPORTED | EFLN_ACTIVATION_INPUT);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_pCachedToken = nullptr;
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Token))
            {
                CacheToken(pActInfo);
            }
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                if (!m_pCachedToken)
                {
                    CacheToken(pActInfo);
                }

                if (m_pCachedToken)
                {
                    // now this is a messy thing.
                    // we use TFlowInputData to do all the work, because the
                    // game tokens uses them as well.
                    // does for the same values we get the same converted strings
                    TFlowInputData tokenData;
                    m_pCachedToken->GetValue(tokenData);
                    TFlowInputData checkData (tokenData);
                    checkData.SetValueWithConversion(GetPortString(pActInfo, InputPorts::CompareValue));
                    string tokenString, checkString;
                    tokenData.GetValueWithConversion(tokenString);
                    checkData.GetValueWithConversion(checkString);
                    ActivateOutput(pActInfo, OutputPorts::TokenValue, tokenData);
                    bool equal = tokenString.compareNoCase(checkString) == 0;
                    ActivateOutput(pActInfo, OutputPorts::Result, equal);
                    // activate either the [OnTrue] or [OnFalse] port depending on comparison result
                    ActivateOutput(pActInfo, equal ? OutputPorts::OnTrue : OutputPorts::OnFalse, true);
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
    {
        string name = GetPortString(pActInfo, InputPorts::Token);
        m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
    }

    IGameToken* m_pCachedToken;
};

//////////////////////////////////////////////////////////////////////////
class CGameTokenCheckMultiFlowNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        Token,
        ValueFirst,     // index of the first 'value' port
        ValueMax = 8,   // maximum number of 'value' ports
    };

    enum OutputPorts
    {
        TokenValue,
        OneTrue,
        AllFalse,
    };

public:
    CGameTokenCheckMultiFlowNode(SActivationInfo* pActInfo)
        : m_pCachedToken(nullptr)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CGameTokenCheckMultiFlowNode(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        if (ser.IsReading()) // forces re-caching
        {
            m_pCachedToken = nullptr;
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<string> ("gametoken_Token", _HELP("Game token to check"), _HELP("Token")),
            InputPortConfig<string>("Value0", _HELP("Value to compare the token with")),
            InputPortConfig<string> ("Value1", _HELP("Value to compare the token with")),
            InputPortConfig<string> ("Value2", _HELP("Value to compare the token with")),
            InputPortConfig<string> ("Value3", _HELP("Value to compare the token with")),
            InputPortConfig<string> ("Value4", _HELP("Value to compare the token with")),
            InputPortConfig<string> ("Value5", _HELP("Value to compare the token with")),
            InputPortConfig<string> ("Value6", _HELP("Value to compare the token with")),
            InputPortConfig<string> ("Value7", _HELP("Value to compare the token with")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("TokenValue", _HELP("Value of the token")),
            OutputPortConfig_Void("OneTrue", _HELP("Activated if the [Token]'s value is equal to at least one of the [Value#] ports")),
            OutputPortConfig_Void("AllFalse", _HELP("Activated if the [Token]'s value is not equal to any of the [Value#] ports")),
            {0}
        };
        config.sDescription = _HELP("Check if a game token equals to any of a list of values");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= (EFLN_AISEQUENCE_SUPPORTED | EFLN_ACTIVATION_INPUT);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_pCachedToken = nullptr;
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Token))
            {
                CacheToken(pActInfo);
            }
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                if (!m_pCachedToken)
                {
                    CacheToken(pActInfo);
                }

                if (m_pCachedToken)
                {
                    // now this is a messy thing.
                    // we use TFlowInputData to do all the work, because the
                    // game tokens uses them as well.
                    // does for the same values we get the same converted strings
                    TFlowInputData tokenData;
                    m_pCachedToken->GetValue(tokenData);
                    TFlowInputData checkData (tokenData);

                    ActivateOutput(pActInfo, OutputPorts::TokenValue, tokenData);

                    string tokenString, checkString;
                    tokenData.GetValueWithConversion(tokenString);

                    bool bAnyTrue = false;

                    for (int i = 0; i < InputPorts::ValueMax; ++i)
                    {
                        TFlowInputData chkData (tokenData);
                        checkString = GetPortString(pActInfo, InputPorts::ValueFirst + i);
                        if (!checkString.empty())
                        {
                            chkData.SetValueWithConversion(checkString);
                            chkData.GetValueWithConversion(checkString);      // this double conversion is to correctly manage cases like "true" strings vs bool tokens
                            bAnyTrue |= (tokenString.compareNoCase(checkString) == 0);
                        }
                    }

                    if (bAnyTrue)
                    {
                        ActivateOutput(pActInfo, OutputPorts::OneTrue, true);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts::AllFalse, true);
                    }
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
    {
        string name = GetPortString(pActInfo, InputPorts::Token);
        m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
    }

    IGameToken* m_pCachedToken;
};

//////////////////////////////////////////////////////////////////////////
class CGameTokenFlowNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public IGameTokenEventListener
{
    enum InputPorts
    {
        Token,
        CompareValue,
    };

    enum OutputPorts
    {
        TokenValue,
        OnTrue,
        OnFalse,
    };

public:
    CGameTokenFlowNode(SActivationInfo* pActInfo)
        : m_pCachedToken(nullptr)
        , m_actInfo(*pActInfo)
    {
    }

    ~CGameTokenFlowNode() override
    {
        Unregister();
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CGameTokenFlowNode(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        if (ser.IsReading())
        {
            // recache and register at token
            CacheToken(pActInfo);
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<string>("gametoken_Token", _HELP("Game Token to compare"), _HELP("Token")),
            InputPortConfig<string>("CompareValue", _HELP("Value to compare to")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("TokenValue", _HELP("Activates with [Token]'s value whenever it changes")),
            OutputPortConfig<bool>("OnTrue", _HELP("Activates when [Token]'s value equals [CompareValue]")),
            OutputPortConfig<bool>("OnFalse", _HELP("Activates when [Token]'s value is not equal to [CompareValue]")),
            {0}
        };
        config.sDescription = _HELP("Compare a game Token with a value");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::CompareValue))
            {
                m_sComparisonString = GetPortString(pActInfo, InputPorts::CompareValue);
            }

            if (IsPortActive(pActInfo, InputPorts::Token))
            {
                CacheToken(pActInfo);
                if (m_pCachedToken)
                {
                    TFlowInputData data;
                    m_pCachedToken->GetValue(data);
                    ActivateOutput(pActInfo, OutputPorts::TokenValue, data);
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    void OnGameTokenEvent(EGameTokenEvent event, IGameToken* pGameToken) override
    {
        assert (pGameToken == m_pCachedToken);
        if (event == EGAMETOKEN_EVENT_CHANGE)
        {
            TFlowInputData data;
            m_pCachedToken->GetValue(data);
            ActivateOutput(&m_actInfo, OutputPorts::TokenValue, data);

            // now this is a messy thing.
            // we use TFlowInputData to do all the work, because the
            // game tokens uses them as well.
            TFlowInputData checkData (data);
            string tokenString;
            data.GetValueWithConversion(tokenString);
            bool equal = (tokenString.compareNoCase(m_sComparisonString) == 0);
            ActivateOutput(&m_actInfo, equal ? OutputPorts::OnTrue : OutputPorts::OnFalse, true);
        }
        else if (event == EGAMETOKEN_EVENT_DELETE)
        {
            // no need to unregister
            m_pCachedToken = nullptr;
        }
    }

    void Register()
    {
        IGameTokenSystem* pGTSys = GetIGameTokenSystem();
        if (m_pCachedToken && pGTSys)
        {
            pGTSys->RegisterListener(m_pCachedToken->GetName(), this);
        }
    }

    void Unregister()
    {
        IGameTokenSystem* pGTSys = GetIGameTokenSystem();
        if (m_pCachedToken && pGTSys)
        {
            pGTSys->UnregisterListener(m_pCachedToken->GetName(), this);
            m_pCachedToken = nullptr;
        }
    }

    void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
    {
        Unregister();

        string tokenName = GetPortString(pActInfo, InputPorts::Token);
        m_pCachedToken = GetGameToken(pActInfo, tokenName, bForceCreate);

        if (m_pCachedToken)
        {
            Register();
        }
    }

    SActivationInfo m_actInfo;
    IGameToken* m_pCachedToken;
    string          m_sComparisonString;
};

//////////////////////////////////////////////////////////////////////////
class CModifyGameTokenFlowNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        Activate,
        Token,
        Operation,
        TokenType,
        OtherValue,
    };

    enum OutputPorts
    {
        Result,
    };

    enum ETokenOperation
    {
        OpSet,
        OpAdd,
        OpSub,
        OpMul,
        OpDiv,
    };

    enum ETokenType
    {
        TypeBool,
        TypeInt,
        TypeFloat,
        TypeVec3,
        TypeString,
    };

public:
    CModifyGameTokenFlowNode(SActivationInfo* pActInfo)
        : m_pCachedToken(nullptr)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CModifyGameTokenFlowNode(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        if (ser.IsReading()) // forces re-caching
        {
            m_pCachedToken = nullptr;
        }
    }

    template <class Type>
    struct Helper
    {
        Helper(TFlowInputData& value, const TFlowInputData& operand)
        {
            Init(value, operand);
        }

        bool Init(TFlowInputData& value, const TFlowInputData& operand)
        {
            m_ok = value.GetValueWithConversion(m_value);
            m_ok &= operand.GetValueWithConversion(m_operand);
            return m_ok;
        }

        bool m_ok;
        Type m_value;
        Type m_operand;
    };

    template<class Type>
    bool not_zero(Type& val)
    {
        return true;
    }
    bool not_zero(int& val)
    {
        return val != 0;
    }
    bool not_zero(float& val)
    {
        return val != 0.0f;
    }

    template<class Type>
    void op_set(Helper<Type>& data)
    {
        if (data.m_ok)
        {
            data.m_value = data.m_operand;
        }
    }
    template<class Type>
    void op_add(Helper<Type>& data)
    {
        if (data.m_ok)
        {
            data.m_value += data.m_operand;
        }
    }
    template<class Type>
    void op_sub(Helper<Type>& data)
    {
        if (data.m_ok)
        {
            data.m_value -= data.m_operand;
        }
    }
    template<class Type>
    void op_mul(Helper<Type>& data)
    {
        if (data.m_ok)
        {
            data.m_value *= data.m_operand;
        }
    }
    template<class Type>
    void op_div(Helper<Type>& data)
    {
        if (data.m_ok && not_zero(data.m_operand))
        {
            data.m_value /= data.m_operand;
        }
    }

    void op_div(Helper<string>& data)
    {
    }
    void op_mul(Helper<string>& data)
    {
    }
    void op_sub(Helper<string>& data)
    {
    }

    void op_mul(Helper<Vec3>& data)
    {
    }
    void op_div(Helper<Vec3>& data)
    {
    }

    void op_add(Helper<bool>& data)
    {
    }
    void op_sub(Helper<bool>& data)
    {
    }
    void op_div(Helper<bool>& data)
    {
    }
    void op_mul(Helper<bool>& data)
    {
    }

    template<class Type>
    void DoOp(ETokenOperation op, Helper<Type>& data)
    {
        switch (op)
        {
        case ETokenOperation::OpSet:
        {
            op_set(data);
            break;
        }
        case ETokenOperation::OpAdd:
        {
            op_add(data);
            break;
        }
        case ETokenOperation::OpSub:
        {
            op_sub(data);
            break;
        }
        case ETokenOperation::OpMul:
        {
            op_mul(data);
            break;
        }
        case ETokenOperation::OpDiv:
        {
            op_div(data);
            break;
        }
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<string> ("gametoken_Token", _HELP("Game token to set"), _HELP("Token")),
            InputPortConfig<int>("Operation", ETokenOperation::OpSet, _HELP("Operation to perform:  [OutValue] = [Token] OP [TokenValue]"), _HELP("Operation"), _UICONFIG("enum_int:Set=0,Add=1,Sub=2,Mul=3,Div=4")),
            InputPortConfig<int>("TokenType", ETokenType::TypeString, _HELP("Token type"), _HELP("TokenType"), _UICONFIG("enum_int:Bool=0,Int=1,Float=2,Vec3=3,String=4")),
            InputPortConfig<string>("OtherValue", _HELP("Value to operate with")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("Result", _HELP("Result value of the token after operation")),
            {0}
        };
        config.sDescription = _HELP("Operate on a Game Token to modify its value");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= (EFLN_AISEQUENCE_SUPPORTED | EFLN_ACTIVATION_INPUT);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_pCachedToken = nullptr;
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Token))
            {
                CacheToken(pActInfo);
            }
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                if (!m_pCachedToken)
                {
                    CacheToken(pActInfo);
                }

                if (m_pCachedToken)
                {
                    TFlowInputData value;
                    m_pCachedToken->GetValue(value);
                    TFlowInputData operand;
                    operand.Set(GetPortString(pActInfo, InputPorts::OtherValue));

                    ETokenOperation op = static_cast<ETokenOperation>(GetPortInt(pActInfo, InputPorts::Operation));
                    ETokenType type = static_cast<ETokenType>(GetPortInt(pActInfo, InputPorts::TokenType));
                    switch (type)
                    {
                    case ETokenType::TypeBool:
                    {
                        Helper<bool> h(value, operand);
                        DoOp(op, h);
                        if (h.m_ok)
                        {
                            m_pCachedToken->SetValue(TFlowInputData(h.m_value));
                        }
                        break;
                    }
                    case ETokenType::TypeInt:
                    {
                        Helper<int> h(value, operand);
                        DoOp(op, h);
                        if (h.m_ok)
                        {
                            m_pCachedToken->SetValue(TFlowInputData(h.m_value));
                        }
                        break;
                    }
                    case ETokenType::TypeFloat:
                    {
                        Helper<float> h(value, operand);
                        DoOp(op, h);
                        if (h.m_ok)
                        {
                            m_pCachedToken->SetValue(TFlowInputData(h.m_value));
                        }
                        break;
                    }
                    case ETokenType::TypeVec3:
                    {
                        Helper<Vec3> h(value, operand);
                        DoOp(op, h);
                        if (h.m_ok)
                        {
                            m_pCachedToken->SetValue(TFlowInputData(h.m_value));
                        }
                        break;
                    }
                    case ETokenType::TypeString:
                    {
                        Helper<string> h(value, operand);
                        DoOp(op, h);
                        if (h.m_ok)
                        {
                            m_pCachedToken->SetValue(TFlowInputData(h.m_value));
                        }
                        break;
                    }
                    }
                    m_pCachedToken->GetValue(value);
                    ActivateOutput(pActInfo, OutputPorts::Result, value);
                }
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
    {
        string name = GetPortString(pActInfo, InputPorts::Token);
        m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
    }

    IGameToken* m_pCachedToken;
};


//////////////////////////////////////////////////////////////////////////
class CFlowNodeGameTokensLevelToLevelStore
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        TokenFirst,     // index of first token input port
        TokenMax = 10,  // maximum number of token input ports
    };

public:
    CFlowNodeGameTokensLevelToLevelStore(SActivationInfo* pActInfo)
    {
    }

    virtual void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs [] =
        {
            InputPortConfig<string>("gametoken_Token0", _HELP("GameToken to be stored"), _HELP("Token0")),
            InputPortConfig<string>("gametoken_Token1", _HELP("GameToken to be stored"), _HELP("Token1")),
            InputPortConfig<string>("gametoken_Token2", _HELP("GameToken to be stored"), _HELP("Token2")),
            InputPortConfig<string>("gametoken_Token3", _HELP("GameToken to be stored"), _HELP("Token3")),
            InputPortConfig<string>("gametoken_Token4", _HELP("GameToken to be stored"), _HELP("Token4")),
            InputPortConfig<string>("gametoken_Token5", _HELP("GameToken to be stored"), _HELP("Token5")),
            InputPortConfig<string>("gametoken_Token6", _HELP("GameToken to be stored"), _HELP("Token6")),
            InputPortConfig<string>("gametoken_Token7", _HELP("GameToken to be stored"), _HELP("Token7")),
            InputPortConfig<string>("gametoken_Token8", _HELP("GameToken to be stored"), _HELP("Token8")),
            InputPortConfig<string>("gametoken_Token9", _HELP("GameToken to be stored"), _HELP("Token9")),
            {0}
        };
        config.pInputPorts = inputs;
        config.sDescription = _HELP("When activated, the value of all specified gametokens will be internally stored.\nThose values can be restored by using the 'restore' flow node in next level.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_ADVANCED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                std::vector<const char*> gameTokensList;
                for (uint32 i = InputPorts::TokenFirst; i < InputPorts::TokenMax; ++i)
                {
                    const string& gameTokenName = GetPortString(pActInfo, i);
                    if (!gameTokenName.empty())
                    {
                        gameTokensList.push_back(gameTokenName.c_str());
                    }
                }
                const char** ppGameTokensList = &(gameTokensList[0]);
                GetIGameTokenSystem()->SerializeSaveLevelToLevel(ppGameTokensList, (uint32)gameTokensList.size());
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNodeGameTokensLevelToLevelRestore
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
    };

    enum OutputPorts
    {
        Output,
    };

public:
    CFlowNodeGameTokensLevelToLevelRestore(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs [] =
        {
            {0}
        };
        static const SOutputPortConfig outputs [] =
        {
            OutputPortConfig_AnyType("Output", _HELP("Signal once tokens are restored")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("When activated, the gametokens previously saved with a 'store' flownode (supposedly at the end of the previous level) will recover their stored value.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_ADVANCED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                GetIGameTokenSystem()->SerializeReadLevelToLevel();
                ActivateOutput(pActInfo, OutputPorts::Output, true);
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Mission Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Mission:GameTokenModify", CModifyGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenSet", CSetGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenGet", CGetGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenCheck", CCheckGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenCheckMulti", CGameTokenCheckMultiFlowNode);
REGISTER_FLOW_NODE("Mission:GameToken", CGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokensLevelToLevelStore", CFlowNodeGameTokensLevelToLevelStore);
REGISTER_FLOW_NODE("Mission:GameTokensLevelToLevelRestore", CFlowNodeGameTokensLevelToLevelRestore);
