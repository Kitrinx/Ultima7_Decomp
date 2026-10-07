#ifndef SOUNDS_H
#define SOUNDS_H

struct ItemId;

void PlayAmbientSounds();
void ResetContinuousSounds();
void PlayItemAmbientSound(uint16_t type, int16_t frame, int16_t dx, int16_t dy);
extern "C" void PlaySoundAtItem(uint8_t sound, ItemId object);

struct CellCoord;

void PlaySoundAt(uint8_t sound, CellCoord x, CellCoord y);

extern int16_t AnimationPhase;
void RequestContinuousSound(uint8_t which, int16_t volume, int16_t pan);
void UpdateContinuousSounds();

extern uint8_t WaterWheelPlayed;
extern uint8_t MillStonePlayed;
/* Launcher switch: silences the fire sword and firedoom staff crackle. */
extern "C" uint8_t QuietWeapons;

#endif
