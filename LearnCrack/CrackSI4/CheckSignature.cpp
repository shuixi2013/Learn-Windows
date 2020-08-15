#include <windows.h>
BYTE* ReadBinary(char* szFilePath, DWORD* cbSize);

DWORD VerifySignature(BYTE* pbData, DWORD dwDataLen, BYTE* pbSignature, DWORD dwSigLen)
{
    DWORD cbPublicKey = 0;
    LPCSTR publicKey = (LPCSTR)ReadBinary("public.key", &cbPublicKey);

    BYTE bBinary[0x400] = { 0 };
    DWORD cbBinary = sizeof(bBinary);
    if (!CryptStringToBinaryA((LPCSTR)publicKey, cbPublicKey, CRYPT_STRING_BASE64HEADER, bBinary, &cbBinary, NULL, NULL))
        return 0x1D8;

    PCERT_PUBLIC_KEY_INFO pvStructInfo = NULL;
    DWORD cbStructInfo = 0;
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO, bBinary, cbBinary,
        CRYPT_DECODE_ALLOC_FLAG, NULL, &pvStructInfo, &cbStructInfo))
        return 0x1D8;

    HCRYPTPROV hProv = NULL;
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return 0x1D9;

    HCRYPTKEY hKey = NULL;
    if (!CryptImportPublicKeyInfo(hProv, X509_ASN_ENCODING, pvStructInfo, &hKey))
        return 0x1DA;

    LocalFree(pvStructInfo);

    HCRYPTHASH hHash = NULL;
    if (!CryptCreateHash(hProv, CALG_SHA1, NULL, 0, &hHash))
        return 0x1DA;

    if (!CryptHashData(hHash, pbData, dwDataLen, 0))
        return 0x1DB;

    BOOL success = CryptVerifySignatureW(hHash, pbSignature, dwSigLen, hKey, NULL, 0);  //dwFalgs没有设置，则按照pkcs7的签名格式验证

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    if (success)
        return 0xC8;

    return 0x1CE;
}


int Base64Decode(char* szSignature, BYTE* ptr_pbSignature, DWORD* ptr_dwSigLen)
{
    int i = 0;
    int j = 0;
    int edi = 0;
    int edx = 0;
    while (true)
    {
        unsigned int ecx = 0;
        if ((unsigned int)(szSignature[i] - 'A') <= 0x19)       // char = 'A' ~ 'Z'
        {                                                       //  ecx =  0  ~  19h
            ecx = (unsigned int)szSignature[i] - 'A';
            if (ecx < 0)
            {
                break;
            }
        }
        else if ((unsigned int)(szSignature[i] - 'a') <= 0x19)  // char =  'a' ~ 'z'
        {                                                       //  ecx =  1Ah ~ 33h
            ecx = (unsigned int)szSignature[i] - 'G';
            if (ecx < 0)
            {
                break;
            }
        }
        else if ((unsigned int)(szSignature[i] - '0') <= 9)     // char = '0' ~ '9'
        {                                                       //  ecx = 34h ~ 3Dh
            ecx = (unsigned int)szSignature[i] + 4;
            if (ecx < 0)
            {
                break;
            }
        }
        else if (szSignature[i] == '+')                         // char = 2Bh
        {                                                       //  ecx = 3Eh
            ecx = '>';
        }
        else if (szSignature[i] == '/')                         // char = 2Fh
        {                                                       //  ecx = 3Fh
            ecx = '?';
        }
        else
        {
            break;
        }

        edi <<= 6;
        edi |= ecx;
        edx += 6;
        i++;
        edi &= 0xFFFF;
        if ((edx & 0xFFFF) >= 8)
        {
            edx += 0xFFF8;
            unsigned short di = edi & 0xFFFF;
            unsigned char dl = edx & 0xFF;
            BYTE bt = (di >> dl) & 0xFF;
            ptr_pbSignature[j++] = bt;
        }
    }

    while (szSignature[i] == '=')
    {
        i++;
    }
    *ptr_dwSigLen = j;
    return i;
}