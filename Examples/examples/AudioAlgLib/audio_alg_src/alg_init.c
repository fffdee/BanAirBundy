#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "alg_api.h"

#define MultiApplyCode   MultiApplyCode1
//#define AUTHORIZATION_ENC_FLASH  //打开宏则表示授权证书存放在flash中，关闭宏则表示授权证书存放在sram中
//
unsigned char AccessCredential(uint8_t *param)
{
	int i,j;
	uint8_t TempUUID[8];
	uint8_t temp[8];

	Chip_IDGet((uint64_t*)TempUUID);

	MY_FUNLIST * fun = MY_FUNLIST_ADDR;


#ifdef AUTHORIZATION_ENC_FLASH
	unsigned char buf[8];
	printf("Pointer to main1: %p\n", fun->CertificateionAtFlash);
//	Remap_AddrRemapDisable(ADDR_REMAP0);
//	Remap_InitTcm((uint32_t)0x4000, 8);//要操作flash,需要将flash操作函数放到TCM
	SpiFlashRead(0xE4,buf,8,20);//只用0xE4 8个字节
	if((0xFFFFFFFF == *((uint32_t*)&buf[0]))&&(0xFFFFFFFF == *((uint32_t*)&buf[4])))//判断没有有证书
	{
		fun->CertificateionAtFlash(MultiApplyCode,buf);//调用生成证书函数（存储在flash中）
	}
//step 1 解密授权申请码
	//存在N组解密算法
	if(MultiApplyCode1 == MultiApplyCode)
	{
		for(j=0;j<8;j++)
		{
			temp[j] = buf[j]-1;//解密算法1 用户自定义
		}
	}
	if(MultiApplyCode2 == MultiApplyCode)
	{
		for(j=0;j<8;j++)
		{
			temp[j] = buf[j]^0x5c;//解密算法2 用户自定义
		}
	}

//step 2 认证解密后的申请码
	if(0 == memcmp(TempUUID,temp,8))
	{
		return 3;//认证成功
	}
	else{
    //方式一
	//延迟
	for(i=0;i<100000;i++)
	{
		__asm__("nop");
	}
	//跳转回到ROMBOOT
	__asm("sethi $r2,#0x01000\n");
	__asm("jral $r2\n");
	return 0xFF;//error site

	// 方式二
//	Reset_McuSystem();

	//方式三
//	return 0;//认证失败
	}
#else
	unsigned char AUTHORIZ_RAM[12];//存放授权证书
	printf("Pointer to main2: %p\n", fun->CertificateionAtSram);
	if((0xB0 != AUTHORIZ_RAM[0])&&(0xBC!= AUTHORIZ_RAM[1]))//判断没有有证书
	{
		fun->CertificateionAtSram(MultiApplyCode,AUTHORIZ_RAM);//调用生成证书函数（存储在sram中）
	}
//	uint8_t temp[8];
//step 1 解密授权申请码
	//存在N组解密算法
	if(MultiApplyCode1 == MultiApplyCode)
	{
		for(j=0;j<8;j++)
		{
			temp[j] = AUTHORIZ_RAM[j+4]-1;//解密算法1 用户自定义
		}
	}
	if(MultiApplyCode2 == MultiApplyCode)
	{
		for(j=0;j<8;j++)
		{
			temp[j] = AUTHORIZ_RAM[j+4]^0x5c;//解密算法2 用户自定义
		}
	}

//step 2 认证解密后的申请码
	if(0 == memcmp(TempUUID,temp,8))
	{
		return 3;//认证成功
	}
	else{

//step3 认证失败
	//方式一
	//延迟
//	for(i=0;i<100000;i++)
//	{
//		__asm__("nop");
//	}
//	//跳转回到ROMBOOT
//	__asm("sethi $r2,#0x01000\n");
//	__asm("jral $r2\n");
//	return 0xFF;//error site

	// 方式二
//	Reset_McuSystem();

	//方式三
	return 0;
	}
#endif
}

