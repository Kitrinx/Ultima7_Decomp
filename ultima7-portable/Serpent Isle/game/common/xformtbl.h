#ifndef XFORMTBL_H
#define XFORMTBL_H

void LoadXformTables(int32_t *table, char *name, int16_t count, int16_t blanks);
void MoveBufferToVoodoo(int32_t *block, void *buffer, int16_t size);
void SetXformEntries(int32_t *table, uint8_t *indices, int8_t value);
void SetXformEntry(int32_t *table, int16_t index, int8_t value);
void SetXformTableByte(int32_t *table, int16_t record, int16_t index, int8_t value);

#endif
