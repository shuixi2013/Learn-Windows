#pragma comment(lib, "crypt32.lib")

#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>
#include "TestEncryptDecrypt.h"

DWORD VerifySignature(BYTE* pbData, DWORD dwDataLen, BYTE* pbSignature, DWORD dwSigLen);
int Base64Decode(char* szSignature, BYTE* ptr_pbSignature, DWORD* ptr_dwSigLen);
void Base64Encode(BYTE* bSig/*size=256*/, char* szSig/*size=345 contains null-end*/);
BYTE* ReadBinary(char* szFilePath, DWORD* cbSize);

char* sig4_data = "<!--\
SourceInsight4.xLicenseFile\
DONOTEDITTHISFILE.Doingsowillrenderitunusable.\
Thislicensewascreatedfor:\
xxx\
51asm\
xxx@xxx.com\
-->\
<SourceInsightLicense>\
<Header\
Value=\"1\"\
/>\
<LicenseProperties\
LicensedUser=\"51asm\"\
ActId=\"9930152826\"\
HWID=\"ZM6QWPSW-MNFUHVF5\"\
Serial=\"S4SG-KRGM-YD7Q-RCFY\"\
Organization=\"\"\
Email=\"xxx@xxx.com\"\
Type=\"Standard\"\
Version=\"4\"\
MinorVersion=\"0\"\
Date=\"2020-08-15\"\
Expiration=\"2030-08-15\"\
/>";

void main()
{
    // ----------------------------------------------------
    // 第一部分：签名 Sign

    TestSignVerify edc;
    edc.InitializeProviderForSigner(NULL, PROV_RSA_FULL);
    // 导出公钥
    edc.ExportX509PEMPublicKey("public.key");

    // 签名数据
    BYTE* bSig = NULL;
    DWORD szSigLen = 0;
    if (!edc.SignMessage(
        CALG_SHA1,
        (BYTE*)sig4_data,   // 待签名字符串数据，来源自si4.lic 文件中 <Signature>标签之前的所有内容（去掉所有的 \r\n\t和空格）
        strlen(sig4_data),  // 待签名字符串数据长度
        (BYTE**)&bSig,      // 传出参数，二进制签名数据
        &szSigLen))         // 传出参数，二进制签名数据大小
        return;
    
    // Base64编码二进制签名，转为字符串签名，存储在 szSig中
    char szSig[345] = { 0 };
    Base64Encode(bSig, szSig);
    printf("%s\r\n", szSig);

    // ----------------------------------------------------
    // 第二部分：校验 Verify

    // Base64解码字符串签名，转为二进制签名
    BYTE bSigBuff[0x2000] = { 0 };
    DWORD dwSigLen = 0;
    int nConvertLen = Base64Decode(szSig, bSigBuff, &dwSigLen);

    // 校验签名
    DWORD dwRet = VerifySignature(
        (BYTE*)sig4_data, 
        strlen(sig4_data), 
        bSigBuff, 
        dwSigLen);

    if (dwRet == 0xC8)
        printf("success\r\n");
    else
        printf("failed\r\n");

    return;
}


void Base64Encode(BYTE* bSig/*size=256*/, char* szSig/*size=345 contains null-end*/)
{
    char cvtTable[] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };

    unsigned char cvtGroup[3] = { 0 };      //签名中的3个字节构成24位，转成4个6位数，并作为下标去cvtTable中取出转换的字符
    for (int i = 0; i < 85; i++)
    {
        memcpy(cvtGroup, bSig + i * 3, 3);

        int nCvtIdx1 = cvtGroup[0] >> 2;
        int nCvtIdx2 = ((cvtGroup[0] & 3)   << 4) | (cvtGroup[1] >> 4);
        int nCvtIdx3 = ((cvtGroup[1] & 0xF) << 2) | (cvtGroup[2] >> 6);
        int nCvtIdx4 = (cvtGroup[2] & 0x3F);

        szSig[i * 4] = cvtTable[nCvtIdx1];
        szSig[i * 4 + 1] = cvtTable[nCvtIdx2];
        szSig[i * 4 + 2] = cvtTable[nCvtIdx3];
        szSig[i * 4 + 3] = cvtTable[nCvtIdx4];
    }

    szSig[340] = cvtTable[bSig[255] >> 2];
    szSig[341] = cvtTable[(bSig[255] & 3) << 4];
    szSig[342] = '=';
    szSig[343] = '=';
}

BYTE* ReadBinary(char* szFilePath, DWORD* cbSize)
{
    *cbSize = 0;
    HANDLE hFile = CreateFile(szFilePath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == NULL)
        return NULL;

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    BYTE* buff = (BYTE*)malloc(dwFileSize);

    if (buff != NULL)
    {
        ZeroMemory(buff, dwFileSize);

        DWORD dwBytesRead = 0;
        if (!ReadFile(hFile, buff, dwFileSize, &dwBytesRead, NULL))
            free(buff);
        else
            *cbSize = dwBytesRead;
    }
    CloseHandle(hFile);
    return buff;
}

