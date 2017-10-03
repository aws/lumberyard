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

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/utils.h>

#include <AzCore/Android/JNI/JNI.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            namespace Internal
            {
                //! \brief Templated interface for getting specific JNI type signatures.  This is intentionally left empty
                //!        to enforce usage to only known template specializations.
                //! \tparam Type The JNI type desired for the signature request
                //! \tparam StringType The type of string that should be returned.  Defaults to 'const char*'
                //! \return String containing the type specific JNI signature
                template<typename Type, typename StringType = const char*>
                StringType GetTypeSignature(Type);

                ///!@{
                //! \brief Known type specializations for \ref AZ::Android::JNI::Internal::GetTypeSignature.
                template<> inline const char* GetTypeSignature(jboolean) { return "Z"; }
                template<> inline const char* GetTypeSignature(bool) { return "Z"; }
                template<> inline const char* GetTypeSignature(jbooleanArray) { return "[Z"; }

                template<> inline const char* GetTypeSignature(jbyte) { return "B"; }
                template<> inline const char* GetTypeSignature(jbyteArray) { return "[B"; }

                template<> inline const char* GetTypeSignature(jchar) { return "C"; }
                template<> inline const char* GetTypeSignature(jcharArray) { return "[C"; }

                template<> inline const char* GetTypeSignature(jshort) { return "S"; }
                template<> inline const char* GetTypeSignature(jshortArray) { return "[S"; }

                template<> inline const char* GetTypeSignature(jint) { return "I"; }
                template<> inline const char* GetTypeSignature(jintArray) { return "[I"; }

                template<> inline const char* GetTypeSignature(jlong) { return "J"; }
                template<> inline const char* GetTypeSignature(jlongArray) { return "[J"; }

                template<> inline const char* GetTypeSignature(jfloat) { return "F"; }
                template<> inline const char* GetTypeSignature(jfloatArray) { return "[F"; }

                template<> inline const char* GetTypeSignature(jdouble) { return "D"; }
                template<> inline const char* GetTypeSignature(jdoubleArray) { return "[D"; }

                template<> inline const char* GetTypeSignature(jstring) { return "Ljava/lang/String;"; }

                template<> inline const char* GetTypeSignature(jclass) { return "Ljava/lang/Class;"; }

                template<typename StringType>
                StringType GetTypeSignature(jobject value);

                template<typename StringType>
                StringType GetTypeSignature(jobjectArray value);
                //!@}
            }


            //! \brief Utility for generating JNI signatures
            //! \tparam StringType The type of string that should be return during generation.  Defaults to AZStd::string
            template<typename StringType = AZStd::string>
            class Signature
            {
            public:
                typedef StringType string_type;  //!< Internal representation of the string type used

                //! \brief Required for handling cases when an empty set of variadic arguments are forwarded to Signature::Generate calls
                //! \return An empty string
                static string_type Generate()
                {
                    Signature sig;
                    return sig.m_signature;
                }

                //! \brief Gets the signature from n-number of parameters
                //! \param parameters Throwaway variables only used to forward their types on to \ref AZ::Android::JNI::Signature::GenerateImpl
                //! \return String containing a fully qualified Java signature
                template<typename... Args>
                static string_type Generate(Args&&... parameters)
                {
                    Signature sig;
                    sig.GenerateImpl(AZStd::forward<Args>(parameters)...);
                    return sig.m_signature;
                }


            private:
                //! \brief Internal constructor of the signature generator.  Pre-allocates some memory for the internal cache to
                //!        try and prevent the hammering of reallocations in most cases.
                Signature()
                    : m_signature()
                {
                    m_signature.reserve(8);
                }


                //! \brief Appends the desired type to the internal signature cache
                //! \param value Throwaway variable only used to forward the type on to \ref AZ::Android::JNI::Internal::GetTypeSignature
                template<typename Type>
                void GenerateImpl(Type value)
                {
                    m_signature.append(Internal::GetTypeSignature(value));
                }

                //! \brief Explicit definitions for jobject and jobjectArray need to be defined in order to route their
                //!        calls to the correct version of \ref AZ::Android::JNI::Internal::GetTypeSignature which returns
                //!        a string instead of a c-string.
                void GenerateImpl(jobject value)
                {
                    m_signature.append(Internal::GetTypeSignature<string_type>(value));
                }

                void GenerateImpl(jobjectArray value)
                {
                    m_signature.append(Internal::GetTypeSignature<string_type>(value));
                }
                //!@}

                //! \brief Appends the desired type to the internal signature cache
                //! \param first Variable only used to forward the type on to \ref AZ::Android::JNI::Signature::GenerateImpl
                //! \param parameters Additional variables only used to forward their types into recursive calls
                template<typename Type, typename... Args>
                void GenerateImpl(Type first, Args&&... parameters)
                {
                    GenerateImpl(first);
                    GenerateImpl(AZStd::forward<Args>(parameters)...);
                }


                // ----

                string_type m_signature; //!< Internal cache for signature generation
            };


            //! \brief Default Signature template (AZStd::string), primarily used in \ref AZ::Android::JNI::GetSignature
            typedef Signature<AZStd::string> SignatureGenerator;


            //! \brief Generates a fully qualified Java signature from n-number of parameters. This is the preferred implementation
            //!        for generating JNI signatures
            //! \param parameters Variables only used to forward their type info on to \ref AZ::Android::Signature::Generate
            //! \return String containing a fully qualified Java signature
            template<typename... Args>
            AZ_INLINE AZStd::string GetSignature(Args&&... parameters)
            {
                return SignatureGenerator::Generate(AZStd::forward<Args>(parameters)...);
            }
        } // namespace JNI
    } // namespace Android
} // namespace AZ

#include <AzCore/Android/JNI/Internal/Signature_impl.h>


