#ifndef  _ALGORITHM_USER_API_H_
#define  _ALGORITHM_USER_API_H_

unsigned char AccessCredential(unsigned char *param);

void Alg_RamInit(void);

unsigned char Get_AlgVersion(unsigned char * string);

unsigned char Alg_UserTest(unsigned char* buf ,int len);

#endif
