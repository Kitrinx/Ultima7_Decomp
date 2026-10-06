#ifndef XMMHAND_H
#define XMMHAND_H

#ifdef __cplusplus
extern "C" {
#endif
unsigned char HasXmmhand(void);
void SaveXmmhand(int handle);
int LoadXmmhand(void);
void DeleteXmmhand(void);
#ifdef __cplusplus
}
#endif

#endif
