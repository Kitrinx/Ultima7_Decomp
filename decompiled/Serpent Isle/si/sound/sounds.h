#ifndef SOUNDS_H
#define SOUNDS_H

struct ItemId;

void PlayAmbientSounds();
void ResetContinuousSounds();
void PlayItemAmbientSound(unsigned type, int frame, int dx, int dy);
extern "C" void PlaySoundAtItem(unsigned char sound, ItemId object);

struct CellCoord;

void PlaySoundAt(unsigned char sound, CellCoord x, CellCoord y);

extern int AnimationPhase;
void RequestContinuousSound(unsigned char which, int volume, int pan);
void UpdateContinuousSounds();

extern unsigned char WaterWheelPlayed;
extern unsigned char MillStonePlayed;

#endif
