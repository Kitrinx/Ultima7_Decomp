#ifndef ITEMBASE_H
#define ITEMBASE_H

/* Items live in one buffer and are named by their 16-bit offset in it. */
#ifdef __cplusplus
extern "C" {
#endif
extern uint8_t *ItemBufferBase;
#ifdef __cplusplus
}
#endif

#define ItemAt(off) ((void *)(ItemBufferBase + (uint16_t)(off)))

#endif
