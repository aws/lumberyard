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

#include "Declaration.h"

namespace CodeGenerator
{
    class IWriter;

    //! Fields are class member variables.
    template <typename T>
    class Field
        : public Declaration<T>
    {
    public:
        using Declaration<T>::WriteNameType;
        bool Write(T* decl, IWriter* writer) override
        {
            writer->Begin();
            {
                WriteNameType(decl, writer);

                Annotation<T> annotation;
                annotation.Write(decl, writer);
            }
            writer->End();

            return true;
        }

    protected:

        void WriteType(T* decl, IWriter* writer) override
        {
            writer->WriteString("type");
            writer->WriteString(decl->getType().getAsString().c_str());
            writer->WriteString("canonical_type");
            writer->WriteString(decl->getType().getCanonicalType().getAsString());
        }
    };
}