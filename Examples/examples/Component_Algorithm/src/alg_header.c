#include "alg_config.h"

#define _M(A)   ".byte "#A" \n\n"
#define M(A)   _M(A)

#define _MMM(A) ".long "#A" \n\n"
#define MMM(A) _MMM(A)

__attribute__ ((section(".alg_header"),used)) __attribute__((naked))
static void AlgorithmHeader(void)
{
__asm__ __volatile__(
		 ".byte 0x48 \n\n"  //0
		 M(ALG_HIGH_ADDR)   //1
		 M(ALG_MID_ADDR)    //2
		 M(ALG_LOW_ADDR)    //3
		 ".long 0xFFFFFFFF \n\n"//4
		 ".long 0x2c000428 \n\n"//8
		 ".long 0x2c000428 \n\n"//0xc
		 ".long 0x0000F56F \n\n"//0x10
		 ".short 0xDCDC \n\n"//0x14
		 M(ALG_ENCYPTION_FLAG)//0x16
		 M(CHIP_TYPE)//0x17
		 M(ALG_AREA_SET)//0x18
		 M(ALG_MAJOR_VERSION)//0x19
		 M(ALG_MINOR_VERSION)//0x1A
		 M(ALG_PATCH_VERSION)//0x1B
		 ".long 0xFFFFFFFF \n\n"
		 MMM(Alg_Funlist)
         ".rept 0x38 \n\n"
         ".long 0xFFFFFFFF \n\n"
         ".endr \n\n"
		 ".short 0x55AA \n\n"//0x14
    );
}

__attribute__ ((section(".stub_section"),used)) __attribute__((naked))
static void stub(void)
{
__asm__ __volatile__(
                    ".rept 0x4000/4 \n\n"
                    ".long 0xFFFFFFFF \n\n"
                    ".endr \n\n"
    );
}
