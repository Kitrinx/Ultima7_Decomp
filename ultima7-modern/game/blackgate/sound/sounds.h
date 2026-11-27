#ifndef BG_SOUNDS_H
#define BG_SOUNDS_H

struct ItemId;
struct CellCoord;

/* The engine's settings and the renderer's animation frame, which both games' sound code reads. */
extern int16_t AnimationPhase;
extern "C" uint8_t QuietWeapons;

namespace BgSound {

void PlayAmbientSounds();
void ResetContinuousSounds();
void PlayItemAmbientSound(uint16_t type, int16_t frame, int16_t dx, int16_t dy, bool playOneShots);
void PlaySoundAtItem(uint8_t sound, ItemId object);
void PlaySoundAt(uint8_t sound, CellCoord x, CellCoord y);
void RequestContinuousSound(uint8_t which, int16_t volume, int16_t pan);
void UpdateContinuousSounds();

extern uint8_t WaterWheelPlayed;
extern uint8_t MillStonePlayed;

}

#endif
