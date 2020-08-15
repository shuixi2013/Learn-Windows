#pragma once
#pragma comment(lib, "crypt32.lib")

#include <Windows.h>
#include <wincrypt.h>

class TestSignVerify
{
public:
    TestSignVerify()
    {
    }
    ~TestSignVerify()
    {
        if (hKey != NULL)
            CryptDestroyKey(hKey);

        if (hProvider != NULL)
            CryptReleaseContext(hProvider, 0);
    }

    // 获取CSP容器：签名时需要私钥
    BOOL InitializeProviderForSigner(LPCTSTR pszProvider, DWORD dwProvType)
    {
        if (hProvider != NULL)
        {
            if (!CryptReleaseContext(hProvider, 0))
                return 0;
        }

        if (!CryptAcquireContext(&hProvider, NULL, pszProvider, dwProvType, 0)
            && GetLastError() == NTE_BAD_KEYSET)
        {
            return CryptAcquireContext(&hProvider, NULL, pszProvider, dwProvType, CRYPT_NEWKEYSET);
        }
    }

    // 获取CSP容器：签名者提供公钥验证，不需要从CSP中获取公钥/私钥
    BOOL InitializeProviderForVerify(LPCTSTR pszProvider, DWORD dwProvType)
    {
        if (hProvider != NULL)
        {
            if (!CryptReleaseContext(hProvider, 0))
                return 0;
        }

        return CryptAcquireContext(&hProvider, NULL, pszProvider, dwProvType, CRYPT_VERIFYCONTEXT);
    }

    // 生成一对2048位的私钥/公钥
    BOOL Generate2048BitKeys(ALG_ID Algid)
    {
        DWORD dwFlags = (0x800 << 16) | CRYPT_EXPORTABLE;
        return CryptGenKey(hProvider, Algid, dwFlags, &hKey);
    }

    // 导出 PEM格式的公钥文件
    BOOL ExportX509PEMPublicKey(char* szFilePath)
    {
        DWORD dwBlobLen;
        BYTE* pbKeyBlob;
        BYTE* pvEncoded;
        DWORD cbEncoded;
        DWORD cchString;
        DWORD dwBytesWritten;

        if (!CryptExportPublicKeyInfo(hProvider, AT_SIGNATURE, X509_ASN_ENCODING, NULL, &dwBlobLen))
            return FALSE;

        pbKeyBlob = (BYTE*)malloc(dwBlobLen);
        if (pbKeyBlob == NULL)
            return FALSE;

        if (!CryptExportPublicKeyInfo(hProvider, AT_SIGNATURE, X509_ASN_ENCODING, (PCERT_PUBLIC_KEY_INFO)pbKeyBlob, &dwBlobLen))
            return FALSE;

        if(!CryptEncodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO, pbKeyBlob, CRYPT_ENCODE_ALLOC_FLAG, NULL, &pvEncoded, &cbEncoded))
            return FALSE;

        if (!CryptBinaryToStringA((BYTE*)pvEncoded, cbEncoded, CRYPT_STRING_BASE64HEADER, NULL, &cchString))
            return FALSE;

        char* szString = (char*)malloc(cchString);
        if (!CryptBinaryToStringA((BYTE*)pvEncoded, cbEncoded, CRYPT_STRING_BASE64HEADER, szString, &cchString))
            return FALSE;

        HANDLE hFile = CreateFile(szFilePath, FILE_GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == NULL)
            return FALSE;

        if (!WriteFile(hFile, szString, strlen(szString), &dwBytesWritten, NULL))
            return FALSE;

        CloseHandle(hFile);

        return TRUE;
    }

    // 使用者需要释放 pbSignatureOut 
    BOOL SignMessage(ALG_ID Algid, BYTE* pbData, DWORD dwDataLen, BYTE** pbSignatureOut, DWORD* dwSigLenOut)
    {
        HCRYPTHASH hHash;
        BYTE* pbSignature;
        DWORD dwSigLen;

        if (!CryptCreateHash(hProvider, Algid, 0, 0, &hHash))
            return FALSE;

        if (!CryptHashData(hHash, pbData, dwDataLen, 0))
            return FALSE;

        if (!CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &dwSigLen))
            return FALSE;

        pbSignature = (BYTE*)malloc(dwSigLen);
        if (pbSignature == NULL)
            return FALSE;

        if (!CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, pbSignature, &dwSigLen))
            return FALSE;

        if (hHash)
            CryptDestroyHash(hHash);

        *pbSignatureOut = pbSignature;
        *dwSigLenOut = dwSigLen;
        return TRUE;
    }


private:
    HCRYPTPROV hProvider = NULL;
    HCRYPTKEY hKey = NULL;

};