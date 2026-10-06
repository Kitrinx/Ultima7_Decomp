#ifndef TEXT_H
#define TEXT_H

char *GetGameText(unsigned char section, unsigned n);
void InitTextCache(void);

#ifdef __cplusplus
extern "C" {
#endif
void ReportNoCanDo(unsigned char why);
#ifdef __cplusplus
}
#endif

#endif
