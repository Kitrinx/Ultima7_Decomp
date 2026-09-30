#ifndef ITEMOVR1_H
#define ITEMOVR1_H

struct objref;

void far WriteItemTree(objref *object, int file);
void far ReadItemTree(objref *object, int file, int mode);
void far RestoreAvatarMana();

char far MoveContents(objref *source, objref *dest);
unsigned char far CloneItem(objref *source);
unsigned char far CanStackWith(objref *first, objref second);
unsigned char far PlaceDraggedInContainer(objref *object, objref container);
void far WriteItemRecord(objref *object, int file);
void far ReadItemRecord(objref *object, int file, int length, objref link, int mode);

extern char *MlmFileName;

#endif
