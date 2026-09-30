#ifndef ITEMOVR1_H
#define ITEMOVR1_H

struct objref;

void WriteItemTree(objref *object, int16_t file);
void ReadItemTree(objref *object, int16_t file, int16_t mode);
void RestoreAvatarMana();

int8_t MoveContents(objref *source, objref *dest);
uint8_t CloneItem(objref *source);
uint8_t CanStackWith(objref *first, objref second);
uint8_t PlaceDraggedInContainer(objref *object, objref container);
void WriteItemRecord(objref *object, int16_t file);
void ReadItemRecord(objref *object, int16_t file, int16_t length, objref link, int16_t mode);

extern char *MlmFileName;

#endif
