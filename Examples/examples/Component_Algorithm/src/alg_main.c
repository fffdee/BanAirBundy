#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "type.h"
#include "spi_flash.h"
#include "chip_info.h"
#include "alg_config.h"
#include "alg_api.h"

//step 0 由用户代码调用授权代码
__attribute__((section(".alg_function"), optimize("O0")))
uint8_t Authoriz_CertificateionAtFlash(uint32_t  ApplyMode, uint8_t* Param)//授权证书存放在Flash中
{
	int i;
	uint8_t TempUUID[8];
	uint8_t buf[8];

	//step 1 获取芯片UUID
	Chip_IDGet((uint64_t*)TempUUID);
	//step 3  生成授权证书,并存放在Flash
	if(MultiApplyCode1 == ApplyMode)
	{
		for(i=0;i<8;i++)
		{
			Param[i] = TempUUID[i]+1;//加密算法1 用户自定义
		}
	}
	else if(MultiApplyCode2 == ApplyMode)
	{
		for(i=0;i<8;i++)
		{
			Param[i] = TempUUID[i]^0x5c;//加密算法2 用户自定义
		}
	}
	else
	{
		return 0;
	}

	SpiFlashInit(80000000, MODE_1BIT, 0, FSHC_RC_CLK_MODE);//注意用户代码需要配置好flash_clk
	SpiFlashWrite(0xE4,Param,8,0);
	//注意: 加锁解锁由用户代码来配置
	//注意：用户代码加锁要放在认证之后
	Reset_McuSystem();//复位 这样再进入SDK后，会重新配置flash，不需要担心还原
	SpiFlashRead(0xE4,buf,8,20);//重新读取，防止前面的写操作失败
	if(0 == memcmp(Param,buf,8))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

__attribute__((section(".alg_function"), optimize("O0")))
uint8_t Authoriz_CertificateionAtSram(uint32_t  ApplyMode, uint8_t* Param)//授权证书存放在SRAM中
{
	int i;
	uint8_t TempUUID[8];
///step 1 获取芯片UUID
	Chip_IDGet((uint64_t*)TempUUID);
	//step2 访问授权证书
	if(MultiApplyCode1 == ApplyMode)
	{
		for(i=0;i<8;i++)
		{
			Param[i+4] = TempUUID[i]+1;//加密算法1 用户自定义
		}
	}
	else if(MultiApplyCode2 == ApplyMode)
	{
		for(i=0;i<8;i++)
		{
			Param[i+4] = TempUUID[i]^0x5c;//加密算法2 用户自定义
		}
	}
	//3  生成授权证书,并存放在
	Param[1] = 0xBC;
	Param[0] = 0xB0;
}

//全局变量和ZI区域变量初始化代码
//SDK工程在使用之前调用，否则变量未初始化
__attribute__((section(".alg_main"), optimize("O0")))
void __c_init_key1()
{
/* Use compiler builtin memcpy and memset */
#define MEMCPY(des, src, n) __builtin_memcpy ((des), (src), (n))
#define MEMSET(s, c, n) __builtin_memset ((s), (c), (n))

	extern char _end;
	extern char __bss_start;
	int size;

	/* data section will be copied before we remap.
	 * We don't need to copy data section here. */
	extern char __data_lmastart_rom;
	extern char __data_start_rom;
	extern char _edata;

	/* Copy data section to RAM */
	size = &_edata - &__data_start_rom;
	MEMCPY(&__data_start_rom, &__data_lmastart_rom, size);

	/* Clear bss section */
	size = &_end - &__bss_start;
	MEMSET(&__bss_start, 0, size);
	return;
}

unsigned char Alg_GetVersion(unsigned char * string)
{
	if(string)
	{
		string[0] = ALG_MAJOR_VERSION + '0';
		string[1] = '.';
		string[2] = ALG_MINOR_VERSION + '0';
		string[3] = '.';
		string[4] = ALG_PATCH_VERSION + '0';
		string[5] = '\0';
		return 1;
	}
	return 0;
}

unsigned char Alg_TestAdd(unsigned char* buf ,int len)
{
	if(buf && len > 0)
	{
		int i;
		for(i = 0;i<len; i++)
			buf[i] += i;
		return 1;
	}
	return 0;
}

const MY_FUNLIST Alg_Funlist =
{
	.CertificateionAtFlash = Authoriz_CertificateionAtFlash,
	.CertificateionAtSram = Authoriz_CertificateionAtSram,
	.Alg_Ram_Init = __c_init_key1,
	.Alg_GetVer = Alg_GetVersion,
	.Alg_TestFunc = Alg_TestAdd
};

