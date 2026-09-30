#ifndef XFORMTBL_H
#define XFORMTBL_H

void LoadXformTables(long *table, char *name, int count, int blanks);
void MoveBufferToVoodoo(long *block, void *buffer, int size);
void SetXformEntries(long *table, unsigned char *indices, char value);
void SetXformEntry(long *table, int index, char value);
void SetXformTableByte(long *table, int record, int index, char value);

#endif
