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

#ifndef CRYINCLUDE_CRYSYSTEM_ZIPENCRYPT_H
#define CRYINCLUDE_CRYSYSTEM_ZIPENCRYPT_H
#pragma once


#include "ZipFileFormat.h"
#include "ZipDirStructures.h"


#ifdef INCLUDE_LIBTOMCRYPT
    #define USE_LTM
    #define LTM_DESC
    #include <tomcrypt.h>
    #undef byte // tomcrypt defines a byte macro which conflicts with out byte data type
    #define STREAM_CIPHER_NAME "twofish"
extern prng_state g_yarrow_prng_state;
extern rsa_key g_rsa_key_public_for_sign;
#endif //INCLUDE_LIBTOMCRYPT


namespace ZipEncrypt
{
#ifdef INCLUDE_LIBTOMCRYPT
    void Init(const uint8* pKeyData, uint32 keyLen);
    bool StartStreamCipher(unsigned char key[16], unsigned char IV[16], symmetric_CTR * pCTR, const unsigned int offset = 0);
    void FinishStreamCipher(symmetric_CTR* pCTR);
    bool DecryptBufferWithStreamCipher(unsigned char* inBuffer, unsigned char* outBuffer, size_t bufferSize, symmetric_CTR* pCTR);
    bool DecryptBufferWithStreamCipher(unsigned char* inBuffer, size_t bufferSize, unsigned char key[16], unsigned char IV[16]);
    int  GetEncryptionKeyIndex(const ZipDir::FileEntry* pFileEntry);
    void GetEncryptionInitialVector(const ZipDir::FileEntry * pFileEntry, unsigned char IV[16]);

    bool RSA_VerifyData(void* inBuffer, int sizeIn, unsigned char* signedHash, int signedHashSize, rsa_key& publicKey);
    bool RSA_VerifyData(const unsigned char** inBuffers, unsigned int* sizesIn, const int numBuffers, unsigned char* signedHash, int signedHashSize, rsa_key& publicKey);

    int custom_rsa_encrypt_key_ex(const unsigned char* in,     unsigned long inlen,
        unsigned char* out,    unsigned long* outlen,
        const unsigned char* lparam, unsigned long lparamlen,
        prng_state* prng, int prng_idx, int hash_idx, int padding, rsa_key* key);

    int custom_rsa_decrypt_key_ex(const unsigned char* in,       unsigned long  inlen,
        unsigned char* out,      unsigned long* outlen,
        const unsigned char* lparam,   unsigned long  lparamlen,
        int            hash_idx, int            padding,
        int* stat,     rsa_key* key);
#endif  //INCLUDE_LIBTOMCRYPT
}

#endif // CRYINCLUDE_CRYSYSTEM_ZIPENCRYPT_H
