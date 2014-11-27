//
//  test.cpp
//  my-code
//
//  Created by luoning on 14-8-14.
//  Copyright (c) 2014年 luoning. All rights reserved.
//

#include "test.h"
//#include "crypt.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
//#include "hex_string.hpp"
using namespace std;

//static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static int digits_num = strlen(digits);

char* idtourl(uint32_t id)
{
    //static char digits[] =   "0123456789abcdefghigklmnopqrstuvwx";

    static char buf[64];
	char *ptr;
	uint32_t value = id * 2 + 80;
    
	// convert to 36 number system
	ptr = buf + sizeof(buf) - 1;
	*ptr = '\0';
    
	do {
		*--ptr = digits[value % digits_num];
		value /= digits_num;
	} while (ptr > buf && value);
    
	*--ptr = '1';	// add version number
    
	return ptr;
}

uint32_t urltoid(const char* url)
{
	uint32_t url_len = strlen(url);
	char c;
	uint32_t number = 0;
	for (uint32_t i = 1; i < url_len; i++) {
		c = url[i];
        
		if (c >= '0' && c <='9') {
			c -= '0';
		} else if (c >= 'a' && c <= 'z') {
			c -= 'a' - 10;
		} else if (c >= 'A' && c <= 'Z') {
			c -= 'A' - 10;
		} else {
			continue;
		}
        
		number = number * digits_num + c;
	}
    
	return (number - 80) >> 1;
}
/*
void tea_test()
{
    char szKey[17] = "aaaabbbbccccdddd";
    
    char szContent[32] = "abcdefgh";
    char szContent2[32] = "abcdefgh";
    char szEncryptOut[50] = {0};
    int nEncryptOutLength = 50;
    int nDecryptOutLength = 50;
    char szDecryptOut[50] = {0};
    int nEncryptTotalLength = ac::CXTEA::Encrypt(szContent, strlen(szContent), szEncryptOut,
                                                 nEncryptOutLength, szKey);
    int nDecryptTotalLength = ac::CXTEA::Decrypt(szEncryptOut, nEncryptTotalLength,
                                                 szDecryptOut, nDecryptOutLength, szKey);
    
    
    char szEncryptOut2[50] = {0};
    int nEncryptOutLength2 = 50;
    char szDecryptOut2[50] = {0};
    int nDecryptOutLength2 = 50;
    bool bRet1 = ac::TEAEncrypt((BYTE*)szContent2, strlen(szContent2), (BYTE*)szEncryptOut2,
                                &nEncryptOutLength2, (BYTE*)szKey, 16);
    bool bRet2 = ac::TEADecrypt((BYTE*)szEncryptOut2, nEncryptOutLength2,
                                (BYTE*)szDecryptOut2, &nDecryptOutLength2, (BYTE*)szKey, 16);
    
    //if (bRet ) {
    //    printf("aaaa");
    //}
    
    //printf("%u\n", urltoid(szKey));
    printf("#############CXTEA################\n");
    printf("%u, %u\n", nEncryptTotalLength, nDecryptTotalLength);
    string strHexOut = ToHexString((unsigned char*)szEncryptOut, nEncryptTotalLength);
    cout << strHexOut << "," << strHexOut.length() << endl;
    string strDec = string(szDecryptOut, nDecryptTotalLength);
    cout << strDec << "," << nDecryptTotalLength << endl;
    
    printf("############TEA###############\n");
    printf("%u, %u\n", nEncryptOutLength2, nDecryptOutLength2);
    string strHexOut2 = ToHexString((unsigned char*)szEncryptOut2, nEncryptOutLength2);
    cout << strHexOut2 << "," << strHexOut2.length() << endl;
    string strDec2 = string(szDecryptOut2, nDecryptOutLength2);
    cout << strDec2 << "," << nDecryptOutLength2 << endl;
}
*/
int main()
{
    int b = 10001;
    printf("%s\n", idtourl(b));
    string str_a = "1fi1e";
    printf("%d, %s\n", urltoid(str_a.c_str()), str_a.c_str());

    /*
    int a = 99994;
    string str_a(idtourl(a));
    int b = urltoid(str_a.c_str());
    printf("%d, %s, %d\n", a, str_a.c_str(), b);
    //return 0;
    for (int i = 0; i < 10000000; i++) {
        string str_i(idtourl(i));
        int j = urltoid(str_i.c_str());
        if (i != j) {
            printf("parse failed, %d-> %d\n", i, j);
        }
    }
    */
    return 0;
}