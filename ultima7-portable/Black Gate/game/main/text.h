#ifndef TEXT_H
#define TEXT_H

char *GetGameText(uint8_t section, uint16_t n);
void InitTextCache(void);

#ifdef __cplusplus
extern "C" {
#endif
void ReportNoCanDo(uint8_t why);
#ifdef __cplusplus
}
#endif

#endif
