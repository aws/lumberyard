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
#include "SelectionCondition.h"
#include "SelectionVariables.h"


class SimpleLexer
{
public:
    SimpleLexer(const char* buffer)
        : m_buffer(buffer)
        , m_obuffer(buffer)
    {
    }

    int peek(string* ident = 0)
    {
        const char* obuffer = m_buffer;
        string oident(m_ident);
        int tok = operator()();
        m_buffer = obuffer;
        if (ident)
        {
            ident->swap(m_ident);
        }
        m_ident.swap(oident);

        return tok;
    }

    const char* ident() const
    {
        return m_ident.c_str();
    }

    const char* buf() const
    {
        return m_obuffer;
    }


    int operator()()
    {
        while (unsigned char ch = *m_buffer++)
        {
            unsigned char next = *m_buffer;

            switch (ch)
            {
            case '(':
            case ')':
                return (int)ch;
            case '=':
                if (next == '=')
                {
                    ++m_buffer;
                    return '==';
                }
                return ch;
            case '!':
                if (next == '=')
                {
                    ++m_buffer;
                    return '!=';
                }
                return ch;
            default:
            {
                if (isalpha(ch) || (ch == '_'))
                {
                    m_ident.clear();
                    m_ident.push_back(ch);
                    while ((ch = *m_buffer) && (isalnum(ch) || (ch == '.') || (ch == '_')))
                    {
                        ++m_buffer;
                        m_ident.push_back(ch);
                    }

                    // keywords
                    if (!_stricmp(m_ident.c_str(), "or"))
                    {
                        return 'or';
                    }
                    else if (!_stricmp(m_ident.c_str(), "and"))
                    {
                        return 'and';
                    }
                    else if (!_stricmp(m_ident.c_str(), "xor"))
                    {
                        return 'xor';
                    }
                    else if (!_stricmp(m_ident.c_str(), "true"))
                    {
                        return 'true';
                    }
                    else if (!_stricmp(m_ident.c_str(), "false"))
                    {
                        return 'fals';
                    }
                    return 'var';
                }
                else if (isspace(ch))
                {
                    continue;
                }

                return ch;
            }
            }
        }

        return 0;
    }

private:
    const char* m_buffer;
    const char* m_obuffer;
    string m_ident;
};


SelectionCondition::SelectionCondition()
    : m_rootID(-1)
{
}

SelectionCondition::SelectionCondition(const char* condition, const SelectionVariableDeclarations& variables)
    : m_rootID(Parse(condition, variables))
{
    if (m_rootID >= 0)
    {
        Optimise();
    }
}

int SelectionCondition::AddOp(const ConditionOp& op)
{
    m_conditionOps.push_back(op);
    return (int)m_conditionOps.size() - 1;
}

int SelectionCondition::ParseLogical(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables)
{
    int leftID = ParseCompOp(lex, tok, variables);
    if (leftID == -1)
    {
        return -1;
    }

    while ((tok = lex.peek()) && (tok == 'and') || (tok == 'or') || (tok == 'xor'))
    {
        lex();

        int rightID = ParseCompOp(lex, lex(), variables);
        if (rightID == -1)
        {
            return -1;
        }

        switch (tok)
        {
        case 'or':
            leftID = AddOp(ConditionOp(ConditionOp::Or, leftID, rightID));
            break;
        case 'and':
            leftID = AddOp(ConditionOp(ConditionOp::And, leftID, rightID));
            break;
        case 'xor':
            leftID = AddOp(ConditionOp(ConditionOp::Xor, leftID, rightID));
            break;
        }
    }

    return leftID;
}

int SelectionCondition::ParseCompOp(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables)
{
    int leftID = ParseUnary(lex, tok, variables);
    if (leftID == -1)
    {
        return -1;
    }

    tok = lex.peek();
    if ((tok == '==') || (tok == '!='))
    {
        lex();

        int rightID = ParseUnary(lex, lex(), variables);
        if (rightID == -1)
        {
            return -1;
        }

        switch (tok)
        {
        case '==':
            return AddOp(ConditionOp(ConditionOp::Equal, leftID, rightID));
        case '!=':
            return AddOp(ConditionOp(ConditionOp::NotEqual, leftID, rightID));
        }
    }

    return leftID;
}

int SelectionCondition::ParseUnary(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables)
{
    if (tok == '!')
    {
        int opID = ParseValue(lex, lex(), variables);
        if (opID != -1)
        {
            return AddOp(ConditionOp(ConditionOp::Not, opID, -1));
        }
        return -1;
    }

    return ParseValue(lex, tok, variables);
}

int SelectionCondition::ParseValue(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables)
{
    if (tok == '(')
    {
        int opID = ParseLogical(lex, lex(), variables);
        if ((opID == -1) || (lex() != ')'))
        {
            return -1;
        }

        return opID;
    }
    else if (tok == 'true')
    {
        return AddOp(ConditionOp(ConditionOp::Constant, true));
    }
    else if (tok == 'fals')
    {
        return AddOp(ConditionOp(ConditionOp::Constant, false));
    }
    else if (tok == 'var')
    {
        if (SelectionVariableID variableID = variables.GetVariableID(lex.ident()))
        {
            return AddOp(ConditionOp(ConditionOp::Variable, variableID));
        }
        else
        {
            AIWarning("Unknown variable '%s' used in condition '%s'...", lex.ident(), lex.buf());
        }
    }

    return -1;
}

int SelectionCondition::Parse(const char* condition, const SelectionVariableDeclarations& variables)
{
    SimpleLexer lex = SimpleLexer(condition);
    return ParseLogical(lex, lex(), variables);
}

void SelectionCondition::Optimise()
{
    // TODO(Márcio)
}

bool SelectionCondition::EvaluateOp(const SelectionVariables& variables, const ConditionOp& op) const
{
    switch (op.opType)
    {
    case ConditionOp::Variable:
    {
        bool value = false;
        variables.GetVariable(op.variableID, &value);

        return value;
    }
    case ConditionOp::Constant:
        return op.value;
    case ConditionOp::Or:
        return EvaluateOp(variables, m_conditionOps[op.operandLeft])
               || EvaluateOp(variables, m_conditionOps[op.operandRight]);
    case ConditionOp::And:
        return EvaluateOp(variables, m_conditionOps[op.operandLeft])
               && EvaluateOp(variables, m_conditionOps[op.operandRight]);
    case ConditionOp::Xor:
        return EvaluateOp(variables, m_conditionOps[op.operandLeft])
               ^ EvaluateOp(variables, m_conditionOps[op.operandRight]);
    case ConditionOp::Not:
        return !EvaluateOp(variables, m_conditionOps[op.operandLeft]);
    case ConditionOp::Equal:
        return EvaluateOp(variables, m_conditionOps[op.operandLeft])
               == EvaluateOp(variables, m_conditionOps[op.operandRight]);
    case ConditionOp::NotEqual:
        return EvaluateOp(variables, m_conditionOps[op.operandLeft])
               != EvaluateOp(variables, m_conditionOps[op.operandRight]);
    }

    return false;
}

bool SelectionCondition::Evaluate(const SelectionVariables& variables) const
{
    if (Valid())
    {
        return EvaluateOp(variables, m_conditionOps[m_rootID]);
    }

    return false;
}

bool SelectionCondition::Valid() const
{
    return m_rootID >= 0;
}