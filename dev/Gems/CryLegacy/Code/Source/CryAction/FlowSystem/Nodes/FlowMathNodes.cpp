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
#include <sstream>
#include <AzCore/Math/MathUtils.h>

//////////////////////////////////////////////////////////////////////////
// Math nodes.
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
/// Description : This node asks the user for an Operand and two Operators and performs selected Operation on the Operands
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Calculate
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        Operator,
        A,
        B
    };
    enum Operation
    {
        Add = 0,
        Subtract,
        Multiply,
        Divide
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Calculate(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<int>("Operation", Operation::Add, _HELP("Operation : Add=0,Sub=1,Mul=2,Div=3"), nullptr, _UICONFIG("enum_int:Add=0,Sub=1,Mul=2,Div=3")),
            InputPortConfig<float>("A", _HELP("Left hand side Operand ")),
            InputPortConfig<float>("B", _HELP("Right hand side Operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Result of [A] <operation> [B]")),
            {0}
        };

        config.sDescription = _HELP("[Out] = Result of [A] <operation> [B]");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                int op = GetPortInt(pActInfo, InputPorts::Operator);
                float out = 0;
                switch (op)
                {
                case Operation::Add:
                {
                    out = a + b;
                    break;
                }
                case Operation::Subtract:
                {
                    out = a - b;
                    break;
                }
                case Operation::Multiply:
                {
                    out = a * b;
                    break;
                }

                case Operation::Divide:
                {
                    if (!iszero(b))
                    {
                        out = a / b;
                    }
                    break;
                }
                }
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Description : Rounds the input to the nearest integer value
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Round
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        Value,
    };

    enum OutputPorts
    {
        Output = 0,
    };
public:
    CFlowNode_Round(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Number to be rounded")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<int>("OutRounded", _HELP("Rounded output")),
            {0}
        };

        config.sDescription = _HELP("Rounds an input float value to an integer value");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float inpVal = GetPortFloat(pActInfo, InputPorts::Value);
                int out = int_round(inpVal);
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Description : Outputs a float as a string with a cap on the number of decimal places in the output, Used for debug visualization
//////////////////////////////////////////////////////////////////////////
class CFlowNode_FloatToString
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        Number,
        AmountOfDecimals,
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_FloatToString(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("Number", _HELP("Float to be converted to String")),
            InputPortConfig<int>("AmountOfDecimals", _HELP("Count of decimal places in the output")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<string>("Out", _HELP("String representation of given Number")),
            {0}
        };

        config.sDescription = _HELP("Outputs a float as a string, Used for debug visualization");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_DEBUG);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float inpVal = GetPortFloat(pActInfo, InputPorts::Number);
                int decimals = GetPortInt(pActInfo, InputPorts::AmountOfDecimals);
                stack_string formatStr;
                formatStr.Format("%%.%df", decimals);
                string tempstr;
                tempstr.Format(formatStr.c_str(), inpVal);
                ActivateOutput(pActInfo, OutputPorts::Output, tempstr);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs summation of the two operands
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Add
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Add(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Left hand side operand")),
            InputPortConfig<float>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("[A] + [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs summation of the two operands");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                float out = a + b;
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    const char* GetClassTag() const override
    {
        return CLASS_TAG;
    }

    static const char* CLASS_TAG;
};

//////////////////////////////////////////////////////////////////////////
/// Outputs subtraction of the two operands
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Sub
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Sub(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Left hand side operand")),
            InputPortConfig<float>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("[A] - [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs subtraction of the two operands");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                float out = a - b;
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    const char* GetClassTag() const override
    {
        return CLASS_TAG;
    }

    static const char* CLASS_TAG;
};

//////////////////////////////////////////////////////////////////////////
/// Outputs Multiplication of the two operands
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Mul
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Mul(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Left hand side operand")),
            InputPortConfig<float>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("[A] * [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs multiplication of the two operands");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                float out = a * b;
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Result of [A] <operation> [B]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_CalculateVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        Operator,
        A,
        B
    };
    enum Operation
    {
        Add = 0,
        Sub,
        Mul
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_CalculateVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<int>("Operator", Operation::Add, _HELP("Operation : Add=0,Sub=1,Mul=2"), nullptr, _UICONFIG("enum_int:Add=0,Sub=1,Mul=2")),
            InputPortConfig<Vec3>("A", _HELP(" Left hand side Operand ")),
            InputPortConfig<Vec3>("B", _HELP(" Right hand side Operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out", _HELP("Result of [A] <operation> [B]")),
            {0}
        };

        config.sDescription = _HELP("[Out] = [A] <operation> [B] ");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                Vec3 a = GetPortVec3(pActInfo, InputPorts::A);
                Vec3 b = GetPortVec3(pActInfo, InputPorts::B);
                int op = GetPortInt(pActInfo, InputPorts::Operator);
                Vec3 out(0, 0, 0);
                switch (op)
                {
                case Operation::Add:
                {
                    out = a + b;
                    break;
                }

                case Operation::Sub:
                {
                    out = a - b;
                    break;
                }

                case Operation::Mul:
                {
                    out.x = a.x * b.x;
                    out.y = a.y * b.y;
                    out.z = a.z * b.z;
                    break;
                }
                }

                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs Summation of the two vectors
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AddVec3
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_AddVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("A", _HELP("Left hand side operand")),
            InputPortConfig<Vec3>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out", _HELP("[A] + [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs Summation of the two vectors");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                Vec3 a = GetPortVec3(pActInfo, InputPorts::A);
                Vec3 b = GetPortVec3(pActInfo, InputPorts::B);
                Vec3 out = a + b;
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Outputs Subtraction of the two vectors
//////////////////////////////////////////////////////////////////////////
class CFlowNode_SubVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_SubVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("A", _HELP("Left hand side operand")),
            InputPortConfig<Vec3>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out", _HELP("[A] - [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs Subtraction of the two vectors");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                Vec3 a = GetPortVec3(pActInfo, InputPorts::A);
                Vec3 b = GetPortVec3(pActInfo, InputPorts::B);
                Vec3 out = a - b;
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs Hadamard product of the two vectors
//////////////////////////////////////////////////////////////////////////
class CFlowNode_MulVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_MulVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("A", _HELP("Left hand side operand")),
            InputPortConfig<Vec3>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out", _HELP("Hadamard product of the two vectors [A] * [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs Multiplication [element wise] of the two vectors");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                Vec3 a = GetPortVec3(pActInfo, InputPorts::A);
                Vec3 b = GetPortVec3(pActInfo, InputPorts::B);
                Vec3 out(a.x * b.x, a.y * b.y, a.z * b.z);
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs Cross product of the two vectors
//////////////////////////////////////////////////////////////////////////
class CFlowNode_CrossVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_CrossVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("A", _HELP("Left hand side operand")),
            InputPortConfig<Vec3>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out", _HELP("[A] x [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs Cross product of the two vectors");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                Vec3 a = GetPortVec3(pActInfo, InputPorts::A);
                Vec3 b = GetPortVec3(pActInfo, InputPorts::B);
                Vec3 out = a.Cross(b);
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs Dot product of the two vectors
//////////////////////////////////////////////////////////////////////////
class CFlowNode_DotVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_DotVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("A", _HELP("Left hand side operand")),
            InputPortConfig<Vec3>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("[A] . [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs Dot product of the two vectors");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                Vec3 a = GetPortVec3(pActInfo, InputPorts::A);
                Vec3 b = GetPortVec3(pActInfo, InputPorts::B);
                float out = a.Dot(b);
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_MagnitudeVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_MagnitudeVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("vector"),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Length"),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            ActivateOutput(pActInfo, 0, GetPortVec3(pActInfo, 0).GetLength());
            break;
        }
        }
        ;
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ReciprocalVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_ReciprocalVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("vector"),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("length", _HELP("1.0 / length[vector]")),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            Vec3 in(GetPortVec3(pActInfo, 0));
            Vec3 out(abs(in.x) > 0.0001f ? 1.0f / in.x : 0.0f,
                abs(in.y) > 0.0001f ? 1.0f / in.y : 0.0f,
                abs(in.z) > 0.0001f ? 1.0f / in.z : 0.0f);
            ActivateOutput(pActInfo, 0, out);
        }
        }
        ;
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ScaleVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_ScaleVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("vector"),
            InputPortConfig<float>("scale"),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("out"),
            {0}
        };
        config.sDescription = _HELP("[Out] = [vector]*  [scale]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            Vec3 a = GetPortVec3(pActInfo, 0);
            float b = GetPortFloat(pActInfo, 1);
            Vec3 out = a * b;
            ActivateOutput(pActInfo, 0, out);
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
class CFlowNode_NormalizeVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_NormalizeVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("vector", _HELP("||out||=1")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("out", _HELP("||out||=1")),
            OutputPortConfig<float>("length", _HELP("||out||=1"), _HELP("length")),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
            Vec3 a = GetPortVec3(pActInfo, 0);
            float l = a.len();

            if (l > 0)
            {
                ActivateOutput(pActInfo, 0, a * (1.0f / l));
            }
            ActivateOutput(pActInfo, 1, l);
            break;
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_EqualVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_EqualVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("A", _HELP("out triggered when A==B")),
            InputPortConfig<Vec3>("B", _HELP("out triggered when A==B")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool> ("out"),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
            Vec3 a = GetPortVec3(pActInfo, 0);
            Vec3 b = GetPortVec3(pActInfo, 1);
            if (a.IsEquivalent(b))
            {
                ActivateOutput(pActInfo, 0, true);
            }
            else
            {
                ActivateOutput(pActInfo, 0, false);
            }
            break;
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs Reciprocal of the input value
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Reciprocal
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Reciprocal(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Number whose reciprocal is to be calculated")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Reciprocal of the input value")),
            {0}
        };

        config.sDescription = _HELP("Outputs Reciprocal of the input value");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float out = abs(a) > 0.0001f ? 1.0f / a : 0.0f;
                ActivateOutput(pActInfo, OutputPorts::Output, out);
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
/// Outputs base ^ power
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Power
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Power(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("Base", _HELP("The value of the base")),
            InputPortConfig<float>("Power", _HELP("The value of the exponent")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("base ^ power")),
            {0}
        };

        config.sDescription = _HELP("Outputs base ^ power");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                float out = pow(a, b);
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs sqrt(A)
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Sqrt
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Sqrt(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Number whose square root is to be calculated")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP(" sqrt(A) ")),
            {0}
        };

        config.sDescription = _HELP("Calculates the square root of given number");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float out = sqrtf(a);
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs the absolute value of the input number
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Abs
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Abs(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Number whose absolute value is to be calculated")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Outputs the absolute value of the input number")),
            {0}
        };

        config.sDescription = _HELP("Calculates absolute value of a given number");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float out = abs(a);
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs division of two given numbers
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Div
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Div(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Dividend")),
            InputPortConfig<float>("B", _HELP("Divisor")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("[A] / [B]")),
            {0}
        };

        config.sDescription = _HELP("[Out] = [A] / [B]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                float out = 0.0f;
                if (!iszero(b))
                {
                    out = a / b;
                }
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs remainder of the division of the two inputs
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Remainder
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Remainder(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Dividend")),
            InputPortConfig<float>("B", _HELP("Divisor")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Remainder of [A] / [B]")),
            {0}
        };

        config.sDescription = _HELP("Outputs remainder of [A]/[B]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                int ia = FtoI(a);
                int ib = FtoI(b);
                int out;
                if (ib != 0)
                {
                    out = ia % ib;
                }
                else
                {
                    out = 0;
                }
                ActivateOutput(pActInfo, OutputPorts::Output, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Checks if Input number [In] is within the indicated range [Min] and [Max]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_InRange
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_InRange(SActivationInfo* pActInfo)
    {
    }

    enum InputPorts
    {
        Activate = 0,
        NumToBeChecked,
        RangeMin,
        RangeMax,
    };
    enum OutputPorts
    {
        Out = 0,
        True,
        False,
    };


    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Number to be checked")),
            InputPortConfig<float>("Min", _HELP("Minimum value included in the range")),
            InputPortConfig<float>("Max", _HELP("Maximum value included in the range")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Out", _HELP("True when [In]>=[Min] and [In]<=[Max], false otherwise")),
            OutputPortConfig_Void("OnTrue", _HELP("Activated if [In] is in range ([In]>=[Min] and [In]<=[Max])")),
            OutputPortConfig_Void("OnFalse", _HELP("Activated if [In] is out of range ([In]<Min or [In]>Max)")),
            {0}
        };

        config.sDescription = _HELP("Checks if Input number [In] is within the indicated range [Min] and [Max]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        bool bOut = false;
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float in = GetPortFloat(pActInfo, InputPorts::NumToBeChecked);
                float v_min = GetPortFloat(pActInfo, InputPorts::RangeMin);
                float v_max = GetPortFloat(pActInfo, InputPorts::RangeMax);
                bOut = (in >= v_min && in <= v_max);
                ActivateOutput(pActInfo, bOut ? OutputPorts::True : OutputPorts::False, true);
                ActivateOutput(pActInfo, OutputPorts::Out, bOut);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Checks if the two inputs are equal or not
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Equal
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
        True,
        False
    };
public:
    CFlowNode_Equal(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Left hand side operand")),
            InputPortConfig<float>("B", _HELP("Right hand side operand")),
            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Out", _HELP("true if [A] equals [B], false otherwise")),
            OutputPortConfig_Void("OnTrue", _HELP("Activated if [A] equals [B]")),
            OutputPortConfig_Void("OnFalse", _HELP("Activated if [A] is not equal to [B]")),
            {0}
        };

        config.sDescription = _HELP("Checks if [A] equals [B]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        bool bOut = false;
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                const TFlowInputData& a = GetPortAny(pActInfo, InputPorts::A);
                const TFlowInputData& b = GetPortAny(pActInfo, InputPorts::B);

                if (a.GetType() == eFDT_FlowEntityId || b.GetType() == eFDT_FlowEntityId)
                {
                    // Entity Id comparison. If either input is an Id, promote both.
                    const FlowEntityId& entityA = GetPortFlowEntityId(pActInfo, InputPorts::A);
                    const FlowEntityId& entityB = GetPortFlowEntityId(pActInfo, InputPorts::B);

                    bOut = (entityA == entityB);

                    ActivateOutput(pActInfo, entityA.GetId() == entityB.GetId() ? OutputPorts::True : OutputPorts::False, true);
                    ActivateOutput(pActInfo, OutputPorts::Output, bOut);
                }
                else
                if (a.GetType() == eFDT_Float || b.GetType() == eFDT_Float || a.GetType() == eFDT_Double || b.GetType() == eFDT_Double || a.GetType() == eFDT_Int || b.GetType() == eFDT_Int)
                {
                    // Floating point comparison (ints get converted to floating point in order to support int vs. float comparisons).
                    float numericA = GetPortFloat(pActInfo, InputPorts::A);
                    float numericB = GetPortFloat(pActInfo, InputPorts::B);

                    const float precisionTolerance = 0.00001f;     // Acceptable tolerance within the flowgraph system, anything smaller may lead to inconsistent results.
                    bOut = AZ::IsClose(numericA, numericB, precisionTolerance);

                    ActivateOutput(pActInfo, bOut ? OutputPorts::True : OutputPorts::False, true);
                    ActivateOutput(pActInfo, OutputPorts::Output, bOut);
                }
                else
                if (a.GetType() == eFDT_Vec3 && b.GetType() == eFDT_Vec3)
                {
                    Vec3 vec3A = GetPortVec3(pActInfo, InputPorts::A);
                    Vec3 vec3B = GetPortVec3(pActInfo, InputPorts::B);

                    const float precisionTolerance = 0.00001f;     // Acceptable tolerance within the flowgraph system, anything smaller may lead to inconsistent results.
                    bOut = vec3A.IsEquivalent(vec3B, precisionTolerance);

                    ActivateOutput(pActInfo, bOut ? OutputPorts::True : OutputPorts::False, true);
                    ActivateOutput(pActInfo, OutputPorts::Output, bOut);
                }
                else
                if (a.GetType() == eFDT_Bool && b.GetType() == eFDT_Bool)
                {
                    bool boolA = GetPortBool(pActInfo, InputPorts::A);
                    bool boolB = GetPortBool(pActInfo, InputPorts::B);

                    bOut = (boolA == boolB);

                    ActivateOutput(pActInfo, bOut ? OutputPorts::True : OutputPorts::False, true);
                    ActivateOutput(pActInfo, OutputPorts::Output, bOut);
                }
                else
                {
                    // Invalid comparison
                    ActivateOutput(pActInfo, OutputPorts::False, true);
                    ActivateOutput(pActInfo, OutputPorts::Output, false);
                    CryLogAlways("Warning: Math:Equal does not support comparison of types: %d and %d, see EFlowDataTypes", a.GetType(), b.GetType());
                }
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Checks if [A] is less than [B]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Less
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Output = 0,
        True,
        False
    };

public:
    CFlowNode_Less(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Left hand side operand")),
            InputPortConfig<float>("B", _HELP("Right hand side operand")),
            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Out", _HELP("Set to true if [A] < [B] false otherwise")),
            OutputPortConfig_Void("OnTrue", _HELP("Activated if [A] is less than [B]")),
            OutputPortConfig_Void("OnFalse", _HELP("Activated if [A] is greater than or equal to [B]")),
            {0}
        };

        config.sDescription = _HELP("Checks if [A] is less than [B]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                bool bOut = a < b;
                ActivateOutput(pActInfo, bOut ? OutputPorts::True : OutputPorts::False, true);
                ActivateOutput(pActInfo, OutputPorts::Output, bOut);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Counts the number of times the [in] port gets an activation signal
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Counter
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        In = 0,
        Reset,
        Max
    };

    enum OutputPorts
    {
        Output = 0,
    };

public:
    CFlowNode_Counter(SActivationInfo* pActInfo) { m_nCounter = 0; };
    IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_Counter(pActInfo); }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("m_nCounter", m_nCounter);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("In", _HELP("Port on which activation signals are counted"), "In"),
            InputPortConfig_Void("Reset", _HELP("Resets the counter")),
            InputPortConfig<int>("Max", _HELP("Maximum value the counter can reach before being reset, set to 0 for unlimited")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<int>("Count", _HELP("Count of the times an activation signal was receieved on [In] ")),
            {0}
        };

        config.sDescription = _HELP("Counts the number of times the [In] port gets an activation signal");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_nCounter = 0;
            ActivateOutput(pActInfo, OutputPorts::Output, m_nCounter);
            break;
        }

        case eFE_Activate:
        {
            int nMax = GetPortInt(pActInfo, InputPorts::Max);
            int nCounter = m_nCounter;
            if (IsPortActive(pActInfo, InputPorts::In))
            {
                nCounter++;
            }
            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                nCounter = 0;
            }

            if (nMax > 0 && nCounter >= nMax)
            {
                nCounter = 0;
            }
            m_nCounter = nCounter;
            ActivateOutput(pActInfo, OutputPorts::Output, m_nCounter);
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
private:
    int m_nCounter;
};

//////////////////////////////////////////////////////////////////////////
/// Counts the number of activated 'in' ports. Allows designers to, for example, trigger an action when 7 out of 12 entities have died
//////////////////////////////////////////////////////////////////////////
class CFlowNode_PortCounter
    : public CFlowBaseNode<eNCT_Instanced>
{
    static const int PORT_COUNT = 16;


    enum InputPorts
    {
        Reset = 0,
        PortThreshold,
        TotalThreshold
    };

    enum OutputPorts
    {
        portCount = 0,
        totalCount,
        portTrigger,
        totalTrigger
    };

public:
    CFlowNode_PortCounter(SActivationInfo* pActInfo) { this->ReInitialize(); };
    IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_PortCounter(pActInfo); }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("Reset", _HELP("Resets Total and Port counters")),
            InputPortConfig<int>("PortThreshold", 0, _HELP("If portCount equals [portThreshold], [portTrigger] is activated")),
            InputPortConfig<int>("TotalThreshold", 0, _HELP("If totalCount equals [totalThreshold], [totalTrigger] is activated")),
            InputPortConfig_AnyType("In00"),
            InputPortConfig_AnyType("In01"),
            InputPortConfig_AnyType("In02"),
            InputPortConfig_AnyType("In03"),
            InputPortConfig_AnyType("In04"),
            InputPortConfig_AnyType("In05"),
            InputPortConfig_AnyType("In06"),
            InputPortConfig_AnyType("In07"),
            InputPortConfig_AnyType("In08"),
            InputPortConfig_AnyType("In09"),
            InputPortConfig_AnyType("In10"),
            InputPortConfig_AnyType("In11"),
            InputPortConfig_AnyType("In12"),
            InputPortConfig_AnyType("In13"),
            InputPortConfig_AnyType("In14"),
            InputPortConfig_AnyType("In15"),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<int>("PortCount", _HELP("Total number of ports that have been set since last reset. Only counts each port once. (0 to 16)")),
            OutputPortConfig<int>("TotalCount", _HELP("Sum of all the times any of the ports have been set, since last reset.")),
            OutputPortConfig<bool>("PortTrigger", _HELP("Triggered when port count reaches port threshold.")),
            OutputPortConfig<bool>("TotalTrigger", _HELP("Triggered when total count reaches total threshold.")),
            {0}
        };
        config.sDescription = _HELP("Counts the number of activated 'in' ports. Allows designers to, for example, trigger an action when 7 out of 12 entities have died");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            //--- Reset?
            if (IsPortActive(pActInfo, InputPorts::Reset) || (event == eFE_Initialize))
            {
                this->ReInitialize();
            }

            //--- Any in ports activated?
            int iPort;
            int nPorts = m_nPorts;
            int nTotal = m_nTotal;

            for (iPort = 3; iPort < PORT_COUNT; ++iPort)
            {
                if (IsPortActive(pActInfo, iPort))
                {
                    if (!m_abPorts[iPort])
                    {
                        m_abPorts[iPort] = true;
                        ++nPorts;
                    }
                    ++nTotal;
                }
            }

            //--- Check triggers
            if (!m_bPortTriggered && (nPorts >= GetPortInt(pActInfo, InputPorts::PortThreshold)))
            {
                m_bPortTriggered = true;
                ActivateOutput(pActInfo, OutputPorts::portTrigger, m_bPortTriggered);
            }

            if (!m_bTotalTriggered && (nTotal >= GetPortInt(pActInfo, InputPorts::TotalThreshold)))
            {
                m_bTotalTriggered = true;
                ActivateOutput(pActInfo, OutputPorts::totalTrigger, m_bTotalTriggered);
            }

            //--- Output any changed totals
            if (nPorts != m_nPorts)
            {
                m_nPorts = nPorts;
                ActivateOutput(pActInfo, OutputPorts::portCount, m_nPorts);
            }

            if (nTotal != m_nTotal)
            {
                m_nTotal = nTotal;
                ActivateOutput(pActInfo, OutputPorts::totalCount, m_nTotal);
            }

            break;
        }
        }
    };

    void Serialize(SActivationInfo*, TSerialize ser) override
    {
        ser.BeginGroup("Local");
        ser.Value("m_nPorts", m_nPorts);
        ser.Value("m_nTotal", m_nTotal);
        ser.Value("m_bPortTriggered", m_bPortTriggered);
        ser.Value("m_bTotalTriggered", m_bTotalTriggered);
        if (ser.IsWriting())
        {
            std::vector<bool> ab (m_abPorts, m_abPorts + PORT_COUNT);
            ser.Value("m_abPorts", ab);
        }
        else
        {
            memset((void*)m_abPorts, 0, sizeof(m_abPorts)); // clear them in case we can't read all values
            std::vector<bool> ab;
            ser.Value("m_abPorts", ab);
            std::copy(ab.begin(), ab.end(), m_abPorts);
        }
        ser.EndGroup();
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    int m_nPorts;
    int m_nTotal;
    bool m_bPortTriggered;
    bool m_bTotalTriggered;
    bool m_abPorts[PORT_COUNT];

    void ReInitialize()
    {
        m_nPorts = 0;
        m_nTotal = 0;
        m_bPortTriggered = false;
        m_bTotalTriggered = false;
        memset((void*)m_abPorts, 0, sizeof(m_abPorts));
    }
};

//////////////////////////////////////////////////////////////////////////
/// Generates a random number between [Min] and [Max]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Random
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Generate = 0,
        Min,
        Max
    };

    enum OutputPorts
    {
        Out,
        RoundedOut
    };

public:
    CFlowNode_Random(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Generate", _HELP("Generate a random number")),
            InputPortConfig<float>("Min", _HELP("Minimum value of the random number")),
            InputPortConfig<float>("Max", _HELP("Maximum value of the random number")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Random number between min and max")),
            OutputPortConfig<int>("OutRounded", _HELP("[Out] Rounded to next integer value")),
            {0}
        };

        config.sDescription = _HELP("Generates a random number between [Min] and [Max]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Generate))
            {
                float min = GetPortFloat(pActInfo, InputPorts::Min);
                float max = GetPortFloat(pActInfo, InputPorts::Max);
                float f = cry_random(min, max);
                ActivateOutput(pActInfo, OutputPorts::Out, f);
                ActivateOutput(pActInfo, OutputPorts::RoundedOut, int_round(f));
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Clamps a given value to a specified range
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Clamp
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        In = 0,
        Min,
        Max
    };

    enum OutputPorts
    {
        Out = 0,
    };
public:
    CFlowNode_Clamp(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("The value that is to be clamped")),
            InputPortConfig<float>("Min", _HELP("Lowest value that [Out] can be")),
            InputPortConfig<float>("Max", _HELP("Highest value that [Out] can be")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("[In] clamped in the range of [Min] and [Max]")),
            {0}
        };

        config.sDescription = _HELP("Clamps a given value to a specified range");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            float in = GetPortFloat(pActInfo, InputPorts::In);
            float min = GetPortFloat(pActInfo, InputPorts::Min);
            float max = GetPortFloat(pActInfo, InputPorts::Max);
            ActivateOutput(pActInfo, OutputPorts::Out, clamp_tpl(in, min, max));
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ClampVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_ClampVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("In"),
            InputPortConfig<Vec3>("Min"),
            InputPortConfig<Vec3>("Max"),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out"),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            Vec3 in = GetPortVec3(pActInfo, 0);
            Vec3 min = GetPortVec3(pActInfo, 1);
            Vec3 max = GetPortVec3(pActInfo, 2);
            ActivateOutput(pActInfo, 0, Vec3(clamp_tpl(in.x, min.x, max.x), clamp_tpl(in.y, min.y, max.y), clamp_tpl(in.z, min.z, max.z)));
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_SetVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_SetVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Set", _HELP("Trigger to output")),
            InputPortConfig<Vec3>("In", _HELP("Value to be set on output")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out"),
            {0}
        };
        config.sDescription = _HELP("Send input value to the output when event on set port is received.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED); // POLICY-CHANGE: don't send to output on initialize
    }

    void ProcessEvent(EFlowEvent evt, SActivationInfo* pActInfo) override
    {
        switch (evt)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, 0))
            {
                ActivateOutput(pActInfo, 0, GetPortVec3(pActInfo, 1));
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_SetColor
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_SetColor(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("Set", _HELP("Trigger to output")),
            InputPortConfig<Vec3>("color_In", _HELP("Color to be set on output")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Out"),
            {0}
        };
        config.sDescription = _HELP("Send input value to the output when event on set port is received.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent evt, SActivationInfo* pActInfo) override
    {
        switch (evt)
        {
        case eFE_Activate:
        case eFE_Initialize:
            if (IsPortActive(pActInfo, 0))
            {
                ActivateOutput(pActInfo, 0, GetPortVec3(pActInfo, 1));
            }
            break;
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Checks if the input number is even or odd
//////////////////////////////////////////////////////////////////////////
class CFlowNode_EvenOrOdd
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        In = 0,
    };

    enum OutputPorts
    {
        Odd = 0,
        Even
    };
public:
    CFlowNode_EvenOrOdd(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<int>("In", _HELP("Value that is to be checked")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_Void("Odd"),
            OutputPortConfig_Void("Even"),
            {0}
        };

        config.sDescription = _HELP("Checks if the input number is even or odd");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            int nIndex = GetPortInt(pActInfo, InputPorts::In) % 2;
            ActivateOutput(pActInfo, (nIndex == 0) ? OutputPorts::Even : OutputPorts::Odd, true);
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Converts three float values to a vector
//////////////////////////////////////////////////////////////////////////
class CFlowNode_ToVec3
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        x = 0,
        y,
        z
    };

    enum OutputPorts
    {
        Result = 0,
    };

public:
    CFlowNode_ToVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("x", _HELP("X co-ordinate of the output vector")),
            InputPortConfig<float>("y", _HELP("Y co-ordinate of the output vector")),
            InputPortConfig<float>("z", _HELP("Z co-ordinate of the output vector")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Result", _HELP("The output vector")),
            {0}
        };

        config.sDescription = _HELP("Converts three givem float values to a vector");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            float x = GetPortFloat(pActInfo, InputPorts::x);
            float y = GetPortFloat(pActInfo, InputPorts::y);
            float z = GetPortFloat(pActInfo, InputPorts::z);
            ActivateOutput(pActInfo, OutputPorts::Result, Vec3(x, y, z));
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_FromVec3
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_FromVec3(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("vec3"),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("x"),
            OutputPortConfig<float>("y"),
            OutputPortConfig<float>("z"),
            {0}
        };
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            Vec3 v = GetPortVec3(pActInfo, 0);
            ActivateOutput(pActInfo, 0, v.x);
            ActivateOutput(pActInfo, 1, v.y);
            ActivateOutput(pActInfo, 2, v.z);
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ToBoolean
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        True = 0,
        False
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_ToBoolean(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_Void("true", _HELP("Will output true when event on this port received")),
            InputPortConfig_Void("false", _HELP("Will output false when event on this port received")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("out", _HELP("Output true or false depending on the input ports")),
            { 0 }
        };
        config.sDescription = _HELP("Output true or false depending on the input ports");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED); // No Longer output false on Initialize
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::True))
            {
                ActivateOutput(pActInfo, OutputPorts::Out, true);
            }
            else if (IsPortActive(pActInfo, InputPorts::False))
            {
                ActivateOutput(pActInfo, OutputPorts::Out, false);
            }
            break;
        }
        }
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_FromBoolean
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        In = 0
    };

    enum OutputPorts
    {
        False = 0,
        True = 1
    };

public:
    CFlowNode_FromBoolean(SActivationInfo* pActInfo) {};

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<bool>("Value", _HELP("Boolean value")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_Void("False", _HELP("Triggered if Boolean was False")),
            OutputPortConfig_Void("True", _HELP("Triggered if Boolean was True")),
            { 0 }
        };
        config.sDescription = _HELP("Boolean switch");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED); // No Longer output false on Initialize
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            const bool bValue = GetPortBool(pActInfo, InputPorts::In);
            ActivateOutput(pActInfo, bValue ? OutputPorts::True : OutputPorts::False, true);
            break;
        }
        }
    };

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Calculated the Sine and Cosine of a given value
//////////////////////////////////////////////////////////////////////////
class CFlowNode_SinCos
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Sine = 0,
        Cosine
    };

public:
    CFlowNode_SinCos(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("The value whose sine or cosine is to be calculated")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Sin", _HELP("The Sine of [In]")),
            OutputPortConfig<float>("Cos", _HELP("The Cosine of [In]")),
            {0}
        };

        config.sDescription = _HELP("Calculated the Sine and Cosine of a given value");
        config.nFlags = EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float x = GetPortFloat(pActInfo, InputPorts::In);
                ActivateOutput(pActInfo, OutputPorts::Sine, sinf(DEG2RAD(x)));
                ActivateOutput(pActInfo, OutputPorts::Cosine, cosf(DEG2RAD(x)));
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Converts euler angle input to a  direction unit vector output
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AnglesToDir
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Angles = 0,
    };

    enum OutputPorts
    {
        Direction = 0,
        Roll = 1,
    };

public:
    CFlowNode_AnglesToDir(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("Angles", _HELP("Euler angles input")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Dir", _HELP("Direction Unit vector")),
            OutputPortConfig<float>("Roll", _HELP("Output roll")),
            {0}
        };

        config.sDescription = _HELP("Converts euler angle input to a  direction unit vector output");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            Ang3 angles(DEG2RAD(GetPortVec3(pActInfo, InputPorts::Angles)));
            Vec3 dir(Matrix33::CreateRotationXYZ(angles).GetColumn(1));

            ActivateOutput(pActInfo, OutputPorts::Direction, dir);
            ActivateOutput(pActInfo, OutputPorts::Roll, angles.y);
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Converts a given direction vector to the corresponding Euler angles
//////////////////////////////////////////////////////////////////////////
class CFlowNode_DirToAngles
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Direction = 0,
        Roll
    };

    enum OutputPorts
    {
        Angles = 0,
    };

public:
    CFlowNode_DirToAngles(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("Dir", _HELP("Direction")),
            InputPortConfig<float>("Roll", _HELP("Roll")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Angles", _HELP("Euler angles")),
            {0}
        };

        config.sDescription = _HELP("Converts a given direction vector to the corresponding Euler angles");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            Vec3 dir(GetPortVec3(pActInfo, InputPorts::Direction));
            float roll(DEG2RAD(GetPortFloat(pActInfo, InputPorts::Roll)));
            Ang3  angles(Ang3::GetAnglesXYZ(Matrix33::CreateOrientation(dir, Vec3(0, 0, 1), roll)));
            ActivateOutput(pActInfo, OutputPorts::Angles, RAD2DEG(Vec3(angles)));
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Send input value to the output when event on set port is received
//////////////////////////////////////////////////////////////////////////
class CFlowNode_SetNumber
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_SetNumber(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Value to be set on output")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Value that is being set")),
            {0}
        };

        config.sDescription = _HELP("Send input value to the output when event on set port is received");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED); // POLICY-CHANGE: don't send to output on initialize
    }

    void ProcessEvent(EFlowEvent evt, SActivationInfo* pActInfo) override
    {
        switch (evt)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                ActivateOutput(pActInfo, OutputPorts::Out, GetPortFloat(pActInfo, InputPorts::In));
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Outputs the sine of [In]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Sin
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_Sin(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Angle in degrees")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("sin([In])")),
            {0}
        };

        config.sDescription = _HELP("Outputs the sine of [In]");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::In);
                float out = sinf(DEG2RAD(a));
                ActivateOutput(pActInfo, OutputPorts::Out, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs cosine of [In]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Cos
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_Cos(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Angle in degrees")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("cosine([In])")),
            {0}
        };

        config.sDescription = _HELP("Outputs cosine of [In]");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::In);
                float out = cos(DEG2RAD(a));
                ActivateOutput(pActInfo, OutputPorts::Out, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs tangent of [In]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Tan
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_Tan(SActivationInfo* pActInfo)
    {
    }
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Angle in degrees")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("out=tangent([In])")),
            {0}
        };

        config.sDescription = _HELP("Outputs tangent of [In]");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::In);
                float out = tanf(DEG2RAD(a));
                ActivateOutput(pActInfo, OutputPorts::Out, out);
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
/// Outputs sine inverse (arcsin) of the input in degrees
//////////////////////////////////////////////////////////////////////////
class CFlowNode_SinInverse
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_SinInverse(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Value whose arcsin is to be calculated")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Angle in degrees")),
            {0}
        };

        config.sDescription = _HELP("Outputs sine inverse (arcsin) of the input in degrees");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::In);
                float out = RAD2DEG(asinf(a));
                ActivateOutput(pActInfo, OutputPorts::Out, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs cosine inverse (arccos) of [In]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_CosInverse
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_CosInverse(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Value whose arccos is to be calculated")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("arccos([In]) in degrees")),
            {0}
        };

        config.sDescription = _HELP("Outputs cosine inverse (arccos) of [In]");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::In);
                float out = RAD2DEG(acosf(a));
                ActivateOutput(pActInfo, OutputPorts::Out, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs tangent inverse (arctan) of [In]
//////////////////////////////////////////////////////////////////////////
class CFlowNode_TanInverse
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Activate = 0,
        In
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_TanInverse(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("In", _HELP("Value whose arctan is to be calculated")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("arctan([In]) in degrees")),
            {0}
        };

        config.sDescription = _HELP("Outputs tangent inverse (arctan) of [In]");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::In);
                float out = RAD2DEG(atanf(a));
                ActivateOutput(pActInfo, OutputPorts::Out, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Outputs tangent inverse (arctan2) of ([y], [x])
//////////////////////////////////////////////////////////////////////////
class CFlowNode_TanInverse2
    : public CFlowBaseNode < eNCT_Singleton >
{
    enum InputPorts
    {
        Activate = 0,
        x,
        y
    };

    enum OutputPorts
    {
        Out = 0,
    };

public:
    CFlowNode_TanInverse2(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("x", _HELP("X value to be used in arctan2 calculation")),
            InputPortConfig<float>("y", _HELP("Y value to be used in arctan2 calculation")),
            { 0 }
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("arctan2([y], [x]) in degrees")),
            { 0 }
        };

        config.sDescription = _HELP("Outputs tangent inverse (arctan2) of [y], [x]");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float x = GetPortFloat(pActInfo, InputPorts::x);
                float y = GetPortFloat(pActInfo, InputPorts::y);
                float out = RAD2DEG(atan2_tpl(y, x));
                ActivateOutput(pActInfo, OutputPorts::Out, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
/// An up/down counter
//////////////////////////////////////////////////////////////////////////
class CFlowNode_UpDownCounter
    : public CFlowBaseNode < eNCT_Instanced >
{
    enum InputPorts
    {
        Preset = 0,
        Max,
        Min,
        Decrement,
        Increment,
        Wrap
    };

    enum OutputPorts
    {
        Out = 0,
    };

    int m_counter;
    int m_highLimit;
    int m_lowLimit;

public:
    CFlowNode_UpDownCounter(SActivationInfo* pActInfo)
        : m_counter(0)
        , m_highLimit(10)
        , m_lowLimit(0)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_UpDownCounter(pActInfo);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        ser.Value("m_counter", m_counter);
        ser.Value("m_lowLimit", m_lowLimit);
        ser.Value("m_highLimit", m_highLimit);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<int>("Preset", _HELP("Preset value")),
            InputPortConfig<int>("Max", _HELP("Upper limit")),
            InputPortConfig<int>("Min", _HELP("Lower limit")),
            InputPortConfig<bool>("Decrement", _HELP("Count goes down")),
            InputPortConfig<bool>("Increment", _HELP("Count goes up")),
            InputPortConfig<bool>("Wrap", _HELP("The counter will wrap if TRUE")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Out", _HELP("Current count")),
            {0}
        };
        config.sDescription = _HELP("An up/down counter");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Preset) || event == eFE_Initialize)
            {
                m_counter = GetPortInt(pActInfo, InputPorts::Preset);
            }
            else
            {
                if (IsPortActive(pActInfo, InputPorts::Decrement))
                {
                    m_counter--;
                }
                if (IsPortActive(pActInfo, InputPorts::Increment))
                {
                    m_counter++;
                }
            }

            if (IsPortActive(pActInfo, InputPorts::Max))
            {
                m_highLimit = GetPortInt(pActInfo, InputPorts::Max);
            }

            if (IsPortActive(pActInfo, InputPorts::Min))
            {
                m_lowLimit = GetPortInt(pActInfo, InputPorts::Min);
            }

            if (m_counter > m_highLimit)
            {
                m_counter = GetPortBool(pActInfo, InputPorts::Wrap) ? m_lowLimit : m_highLimit;
            }
            else if (m_counter < m_lowLimit)
            {
                m_counter = GetPortBool(pActInfo, InputPorts::Wrap) ? m_highLimit : m_lowLimit;
            }

            ActivateOutput(pActInfo, OutputPorts::Out, m_counter);
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


////////////////////////////////////////////////////
/// Outputs ceiling of A
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Ceil
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        In,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_Ceil(SActivationInfo* pActInfo)
    {
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
    }

    ////////////////////////////////////////////////////
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // Define input ports here, in same order as InputPorts
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<float>("In", 0.0f, _HELP("Value to be ceilinged")),
            {0}
        };

        // Define output ports here, in same order as OutputPorts
        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<float>("Out", _HELP("ceil([In])")),
            {0}
        };

        // Fill in configuration
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs ceiling of A");
        config.SetCategory(EFLN_APPROVED);
    }

    ////////////////////////////////////////////////////
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            // If there is a number to be ceil'ed
            if (IsPortActive(pActInfo, InputPorts::In))
            {
                float result = ceil_tpl(GetPortFloat(pActInfo, InputPorts::In));
                ActivateOutput(pActInfo, OutputPorts::Out, result);
            }
            break;
        }
        }
    }

    ////////////////////////////////////////////////////
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    ////////////////////////////////////////////////////
    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_Ceil(pActInfo);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Floors a given value
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Floor
    : public CFlowBaseNode<eNCT_Instanced>
{
    enum InputPorts
    {
        In,
    };

    enum OutputPorts
    {
        Out,
    };

public:
    ////////////////////////////////////////////////////
    CFlowNode_Floor(SActivationInfo* pActInfo)
    {
    }

    ////////////////////////////////////////////////////
    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
    }

    ////////////////////////////////////////////////////
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        // Define input ports here, in same order as EInputPorts
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<float>("In", 0.0f, _HELP("Value to be floored")),
            {0}
        };

        // Define output ports here, in same order as EOutputPorts
        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<float>("Out", _HELP("floor([In])")),
            {0}
        };

        // Fill in configuration
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Floors a given value");
        config.SetCategory(EFLN_APPROVED);
    }

    ////////////////////////////////////////////////////
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            // If there is a value to be floored
            if (IsPortActive(pActInfo, InputPorts::In))
            {
                float answer = floor_tpl(GetPortFloat(pActInfo, InputPorts::In));
                ActivateOutput(pActInfo, OutputPorts::Out, answer);
            }
            break;
        }
        }
    }

    ////////////////////////////////////////////////////
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    ////////////////////////////////////////////////////
    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_Floor(pActInfo);
    }
};

//////////////////////////////////////////////////////////////////////////
/// Calcules the Modulo of first input to the second
//////////////////////////////////////////////////////////////////////////
class CFlowNode_Modulo
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate = 0,
        A,
        B
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CFlowNode_Modulo(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<float>("A", _HELP("Left hand side operand")),
            InputPortConfig<float>("B", _HELP("Right hand side operand")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("Result", _HELP("[A] % [B]")),
            {0}
        };

        config.sDescription = _HELP("Calcules the Modulo of [A] to [B]");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                float a = GetPortFloat(pActInfo, InputPorts::A);
                float b = GetPortFloat(pActInfo, InputPorts::B);
                float out = fmod_tpl(a, b);
                ActivateOutput(pActInfo, OutputPorts::Out, out);
            }
            break;
        }
        }
    };

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

class CBoundingBoxVsBoundingBox
    : public CFlowBaseNode < eNCT_Instanced >
{
public:

    enum class InputPorts
    {
        Activate,

        Min1,
        Max1,
        Min2,
        Max2
    };

    enum class OutputPorts
    {
        Result,
        True,
        False
    };

    CBoundingBoxVsBoundingBox(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CBoundingBoxVsBoundingBox(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("Min1", _HELP("Minimum point for first bounding box")),
            InputPortConfig<Vec3>("Max1", _HELP("Maximum point for first bounding box")),
            InputPortConfig<Vec3>("Min2", _HELP("Minimum point for second bounding box")),
            InputPortConfig<Vec3>("Max2", _HELP("Maximum point for second bounding box")),

            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Result", _HELP("Outputs true if intersection occurred, false if not")),
            OutputPortConfig_Void("True", _HELP("Fires if intersection occurred")),
            OutputPortConfig_Void("False", _HELP("Fires if intersection did not occur")),

            { 0 }
        };

        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.sDescription = _HELP("Tests two bounding boxes for an intersection");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, (int)InputPorts::Activate))
            {
                AABB bounds1(GetPortVec3(pActInfo, (int)InputPorts::Min1), GetPortVec3(pActInfo, (int)InputPorts::Max1));
                AABB bounds2(GetPortVec3(pActInfo, (int)InputPorts::Min2), GetPortVec3(pActInfo, (int)InputPorts::Max2));

                bool intersects = bounds1.IsIntersectBox(bounds2);

                ActivateOutput(pActInfo, (int)OutputPorts::Result, intersects);
                ActivateOutput(pActInfo, intersects ? (int)OutputPorts::True : (int)OutputPorts::False, true);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

class CBoundingBoxVsSphere
    : public CFlowBaseNode < eNCT_Instanced >
{
public:

    enum class InputPorts
    {
        Activate,

        BoundsMin,
        BoundsMax,
        SphereCenter,
        SphereRadius
    };

    enum class OutputPorts
    {
        Result,
        True,
        False
    };

    CBoundingBoxVsSphere(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CBoundingBoxVsSphere(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("BoundsMin", _HELP("Minimum point of bounding box")),
            InputPortConfig<Vec3>("BoundsMax", _HELP("Maximum point of bounding box")),
            InputPortConfig<Vec3>("SphereCenter", _HELP("Center of sphere")),
            InputPortConfig<float>("SphereRadius", _HELP("Radius of sphere")),

            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<bool>("Result", _HELP("Outputs true if intersection occurred, false if not")),
            OutputPortConfig_Void("True", _HELP("Fires if intersection occurred")),
            OutputPortConfig_Void("False", _HELP("Fires if intersection did not occur")),

            { 0 }
        };

        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.sDescription = _HELP("Tests a bounding box and a sphere for an intersection");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, (int)InputPorts::Activate))
            {
                AABB bounds(GetPortVec3(pActInfo, (int)InputPorts::BoundsMin),
                    GetPortVec3(pActInfo, (int)InputPorts::BoundsMax));

                bool intersects = bounds.IsOverlapSphereBounds(GetPortVec3(pActInfo, (int)InputPorts::SphereCenter),
                        GetPortFloat(pActInfo, (int)InputPorts::SphereRadius));

                ActivateOutput(pActInfo, (int)OutputPorts::Result, intersects);
                ActivateOutput(pActInfo, intersects ? (int)OutputPorts::True : (int)OutputPorts::False, true);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


class CRotateVec3OnAxis
    : public CFlowBaseNode < eNCT_Instanced >
{
public:

    enum class InputPorts
    {
        Activate,

        Vector,
        Axis,
        Angle
    };

    enum class OutputPorts
    {
        RotatedVector
    };

    CRotateVec3OnAxis(SActivationInfo* pActInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CRotateVec3OnAxis(pActInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<Vec3>("Vector", _HELP("The vector to rotate")),
            InputPortConfig<Vec3>("Axis", _HELP("Axis to rotate the vector around")),
            InputPortConfig<float>("Angle", _HELP("Angle in degrees to rotate")),

            { 0 }
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<Vec3>("Rotated Vector", _HELP("The result of the rotation")),

            { 0 }
        };

        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.sDescription = _HELP("Rotates a vector on the specified axis by the specified number of degrees");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, (int)InputPorts::Activate))
            {
                Vec3 vec = GetPortVec3(pActInfo, (int)InputPorts::Vector);
                Vec3 axis = GetPortVec3(pActInfo, (int)InputPorts::Axis);
                float angle = DEG2RAD(GetPortFloat(pActInfo, (int)InputPorts::Angle));

                Quat orientation;
                orientation.SetRotationAA(angle, axis);
                Vec3 result = vec * orientation;

                ActivateOutput(pActInfo, (int)OutputPorts::RotatedVector, result);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

const char* CFlowNode_Add::CLASS_TAG = "Math:Add";
const char* CFlowNode_Sub::CLASS_TAG = "Math:Sub";

// Unit testing node creation factory
IFlowNode* CreateMathFlowNode(const char* name)
{
    std::string sname(name);
    if (sname == "Math:Add")
    {
        return new CFlowNode_Add(nullptr);
    }
    else if (sname == "Math:Round")
    {
        return new CFlowNode_Round(nullptr);
    }
    return nullptr;
}

// In-game registration
REGISTER_FLOW_NODE("Math:Round", CFlowNode_Round);
REGISTER_FLOW_NODE(CFlowNode_Add::CLASS_TAG, CFlowNode_Add);
REGISTER_FLOW_NODE(CFlowNode_Sub::CLASS_TAG, CFlowNode_Sub);
REGISTER_FLOW_NODE("Math:Mul", CFlowNode_Mul);
REGISTER_FLOW_NODE("Math:Div", CFlowNode_Div)
REGISTER_FLOW_NODE("Math:Equal", CFlowNode_Equal)
REGISTER_FLOW_NODE("Math:InRange", CFlowNode_InRange)
REGISTER_FLOW_NODE("Math:Less", CFlowNode_Less)
REGISTER_FLOW_NODE("Math:Counter", CFlowNode_Counter)
REGISTER_FLOW_NODE("Math:PortCounter", CFlowNode_PortCounter);
REGISTER_FLOW_NODE("Math:Remainder", CFlowNode_Remainder)
REGISTER_FLOW_NODE("Math:EvenOrOdd", CFlowNode_EvenOrOdd)
REGISTER_FLOW_NODE("Math:Reciprocal", CFlowNode_Reciprocal);
REGISTER_FLOW_NODE("Math:Random", CFlowNode_Random);
REGISTER_FLOW_NODE("Math:Power", CFlowNode_Power);
REGISTER_FLOW_NODE("Math:Sqrt", CFlowNode_Sqrt);
REGISTER_FLOW_NODE("Math:Abs", CFlowNode_Abs);
REGISTER_FLOW_NODE("Math:Clamp", CFlowNode_Clamp);
REGISTER_FLOW_NODE("Math:SinCos", CFlowNode_SinCos);
REGISTER_FLOW_NODE("Math:AnglesToDir", CFlowNode_AnglesToDir);
REGISTER_FLOW_NODE("Math:DirToAngles", CFlowNode_DirToAngles);
REGISTER_FLOW_NODE("Math:SetNumber", CFlowNode_SetNumber);
REGISTER_FLOW_NODE("Math:BooleanTo", CFlowNode_ToBoolean);
REGISTER_FLOW_NODE("Math:BooleanFrom", CFlowNode_FromBoolean);
REGISTER_FLOW_NODE("Math:UpDownCounter", CFlowNode_UpDownCounter);
REGISTER_FLOW_NODE("Math:SetColor", CFlowNode_SetColor);
REGISTER_FLOW_NODE("Math:Sine", CFlowNode_Sin);
REGISTER_FLOW_NODE("Math:Cosine", CFlowNode_Cos);
REGISTER_FLOW_NODE("Math:Tangent", CFlowNode_Tan);
REGISTER_FLOW_NODE("Math:ArcSin", CFlowNode_SinInverse);
REGISTER_FLOW_NODE("Math:ArcCos", CFlowNode_CosInverse);
REGISTER_FLOW_NODE("Math:ArcTan", CFlowNode_TanInverse);
REGISTER_FLOW_NODE("Math:ArcTan2", CFlowNode_TanInverse2);
REGISTER_FLOW_NODE("Math:Calculate", CFlowNode_Calculate);
REGISTER_FLOW_NODE("Debug:FloatToString", CFlowNode_FloatToString);
REGISTER_FLOW_NODE("Math:Ceil", CFlowNode_Ceil);
REGISTER_FLOW_NODE("Math:Floor", CFlowNode_Floor);
REGISTER_FLOW_NODE("Math:Mod", CFlowNode_Modulo);
REGISTER_FLOW_NODE("Vec3:AddVec3", CFlowNode_AddVec3);
REGISTER_FLOW_NODE("Vec3:SubVec3", CFlowNode_SubVec3);
REGISTER_FLOW_NODE("Vec3:MulVec3", CFlowNode_MulVec3);
REGISTER_FLOW_NODE("Vec3:CrossVec3", CFlowNode_CrossVec3);
REGISTER_FLOW_NODE("Vec3:DotVec3", CFlowNode_DotVec3);
REGISTER_FLOW_NODE("Vec3:NormalizeVec3", CFlowNode_NormalizeVec3);
REGISTER_FLOW_NODE("Vec3:ScaleVec3", CFlowNode_ScaleVec3);
REGISTER_FLOW_NODE("Vec3:EqualVec3", CFlowNode_EqualVec3);
REGISTER_FLOW_NODE("Vec3:MagnitudeVec3", CFlowNode_MagnitudeVec3);
REGISTER_FLOW_NODE("Vec3:ToVec3", CFlowNode_ToVec3)
REGISTER_FLOW_NODE("Vec3:FromVec3", CFlowNode_FromVec3)
REGISTER_FLOW_NODE("Vec3:ReciprocalVec3", CFlowNode_ReciprocalVec3);
REGISTER_FLOW_NODE("Vec3:ClampVec3", CFlowNode_ClampVec3);
REGISTER_FLOW_NODE("Vec3:SetVec3", CFlowNode_SetVec3);
REGISTER_FLOW_NODE("Vec3:Calculate", CFlowNode_CalculateVec3);
REGISTER_FLOW_NODE("Vec3:RotateVec3OnAxis", CRotateVec3OnAxis);
REGISTER_FLOW_NODE("IntersectionTests:BoundingBoxVsBoundingBox", CBoundingBoxVsBoundingBox);
REGISTER_FLOW_NODE("IntersectionTests:BoundingBoxVsSphere", CBoundingBoxVsSphere);
