#ifndef AILDRV_H
#define AILDRV_H

#include "ail.h"

/* What sits behind a driver handle. */
#define AIL_KIND_NONE       0
#define AIL_KIND_MT32       1
#define AIL_KIND_DIGITAL    2
#define AIL_KIND_ADLIB      3

/* The kind of a registered driver, or AIL_KIND_NONE for a bad or free handle. */
int16_t AilDriverKind(HDRIVER driver);

/* The XMIDI drivers: the MT-32's and the Ad Lib's. Init and shutdown go through the shared
 * XMIDI interpreter (ailxmidi.h), whose service runs at 120 Hz. */
drvr_desc *Mt32Describe(void);
uint16_t Mt32Detect(void);
drvr_desc *OplDescribe(void);
/* old: the 1991 build of the driver */
uint16_t OplDetect(uint16_t port, uint8_t old);
void OplSetPort(uint16_t port, uint8_t old);
void XmidiStart(int16_t kind);
void XmidiStop(const char *signOff);
void XmidiServe(void);

/* The Sound Blaster digital driver. Serve keeps the output fed. */
drvr_desc *DigitalDescribe(void);
uint16_t DigitalDetect(void);
void DigitalInit(void);
void DigitalShutdown(void);
void DigitalServe(void);

#endif
