#ifndef AILDRV_H
#define AILDRV_H

#include "ail.h"

/* What sits behind a driver handle. */
#define AIL_KIND_NONE       0
#define AIL_KIND_MT32       1
#define AIL_KIND_DIGITAL    2

/* The kind of a registered driver, or AIL_KIND_NONE for a bad or free handle. */
int16_t AilDriverKind(HDRIVER driver);

/* The MT-32 XMIDI driver. Serve is its 120 Hz service. */
drvr_desc *Mt32Describe(void);
uint16_t Mt32Detect(void);
void Mt32Init(void);
void Mt32Shutdown(const char *signOff);
void Mt32Serve(void);

/* The Sound Blaster digital driver. Serve keeps the output fed. */
drvr_desc *DigitalDescribe(void);
uint16_t DigitalDetect(void);
void DigitalInit(void);
void DigitalShutdown(void);
void DigitalServe(void);

#endif
