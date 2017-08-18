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
#include "Output.h"
#include "JSONWriter.h"

namespace CodeGenerator
{
    class IWriter;

    //! Will parse the annotations supporting nested attributes
    class Parser
    {
    public:

        struct Attribute
        {
            std::string m_name;
            std::vector < Attribute > m_params;

            enum class Type
            {
                Token,
                Integer,
                Double
            };
            Type m_type;

            union
            {
                double d;
                int i;
            } m_value;

            Attribute()
                : m_type(Type::Token)
            {}
        };

        using AttributeList = std::vector < Attribute >;
        using AttributeEntry = std::pair< std::string, std::vector < Attribute > >;
        using AttributeMap = std::map < std::string, AttributeList >;

        //! Parses a parenthesis delimited expression, supports nested expressions
        //! Attributes may be separated by comma, space or tabs.
        //! Example:
        //! annotation_name(attrib0 attrib1(attrib2("Some text") attrib4(123)) attrib5))
        size_t Parse(const std::string& text, AttributeList& list);

        //! Traverses the AttributeList provided and writes the contents as a complex JSON object.
        void Write(IWriter* writer, const AttributeList& list);

    protected:

        //! Write individual attributes depending on their type into a rapidjson Value.
        void WriteAttribute(const Attribute& attribute, rapidjson::Value& object, rapidjson::Document::AllocatorType& allocator);

        //! Traverse and write the attribute map.
        void WriteMap(IWriter* writer, const AttributeMap& map);

        //! Recursively write the AST data into a rapidjson Value
        void Write(IWriter* writer, const AttributeEntry& entry, rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator);

        //! Recursively build the a rapidjson document that represents the AST.
        void Write(IWriter* writer, const AttributeEntry& entry, rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator);

        //! Given an AttributeList it will produce a map of attributes, used to perform attribute grouping.
        void BuildAttributeMaps(const AttributeList& list, AttributeMap& map);

        //! Parse an attribute token.
        size_t ParseAttribute(const std::string& str, AttributeList& list);

        //! Parse an numeric attribute.
        size_t ParseNumber(const std::string& str, AttributeList& list);
    };

    //! Parses and writes out annotations.
    template <typename T>
    class Annotation
        : public Declaration<T>
    {
    public:
        //! Parses the text within an annotation tag and writes it out as nested key/value arrays.
        void ParseInnerText(IWriter* writer, llvm::StringRef& innerText)
        {
            Parser p;
            Parser::AttributeList attributeList;
            p.Parse(innerText.str(), attributeList);
            p.Write(writer, attributeList);
        }

        //! Write out all the annotations as key/value pairs including support for arrays and nested attributes.
        bool Write(T* decl, IWriter* writer) override
        {
            writer->WriteString(std::string("annotations"));

            writer->Begin();
            {
                for (auto attrIt = decl->template specific_attr_begin<clang::AnnotateAttr>(); attrIt != decl->template specific_attr_end<clang::AnnotateAttr>(); ++attrIt)
                {
                    const clang::AnnotateAttr* attr = clang::dyn_cast<clang::AnnotateAttr>(*attrIt);
                    if (!attr)
                    {
                        continue;
                    }

                    // Split into name and what's inside parens (if anything)
                    auto annotationSplitPair = attr->getAnnotation().split("(");

                    auto attributes = annotationSplitPair.second.slice(0, annotationSplitPair.second.find_last_of(")"));

                    // Write the annotation name
                    writer->WriteString(annotationSplitPair.first.trim().str());

                    if (attributes.size() > 0)
                    {
                        ParseInnerText(writer, attributes);
                    }
                }
            }
            writer->End();

            return true;
        }
    };
}