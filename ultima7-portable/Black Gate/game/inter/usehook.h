#ifndef USEHOOK_H
#define USEHOOK_H

int8_t RunUsable(uint8_t event, int16_t item, uint16_t func);

uint8_t HasUsable(uint16_t func);

extern int16_t TelekinesisUsable;
extern int8_t UsableRunning;

#endif
