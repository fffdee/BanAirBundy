#ifndef  _ALGORITHM_API_H_
#define  _ALGORITHM_API_H_

typedef enum
{
	MultiApplyCode1 = 0x12345678,//user defined
	MultiApplyCode2 = 0x78654321,//user defined
}MODE_MAGIC;

typedef unsigned char (*Ida_Apply)(unsigned int ApplyMode, unsigned char* Param);
typedef void (*Alg_Init)(void);
typedef unsigned char (*Get_String)(unsigned char * string);
typedef unsigned char (*Alg_Test)(unsigned char* buf ,int len);

typedef struct
{
	Ida_Apply 	CertificateionAtFlash;
	Ida_Apply 	CertificateionAtSram;
	Alg_Init  	Alg_Ram_Init;
	Get_String	Alg_GetVer;
	Alg_Test	Alg_TestFunc;
}MY_FUNLIST;

//#define MY_FUNLIST_ADDR		*(unsigned int *)0x20		//未加密的情况下 可以直接访问
#define MY_FUNLIST_ADDR			Read_adrress()				//加密的情况下需要用 SpiFlashRead

#endif
