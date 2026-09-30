#ifndef ITEMCMD_H
#define ITEMCMD_H

struct BitArray;

struct ItemId;

uint8_t IsNPCHalted(int16_t object);
uint8_t Item_isRemoved(ItemId *object);
void PlaySoundSimple(uint8_t sound);
void Item_removeFromWorld(ItemId *object);
void Item_returnToWorld(ItemId *object);
void Item_addToView(ItemId *object);

extern char SpeechFileName[];
extern int16_t FallbackFrames[];
extern BitArray SteppedNPCs;
extern BitArray HaltedNPCs;
void InitNPCSets();
int16_t HasNPCStepped(int16_t npcNum);
void ClearSteppedNPCs();
uint8_t Item_isFree(ItemId *object);
uint8_t Item_hasNoType(ItemId *object);
uint8_t Item_haltNPC(ItemId *object);
void Item_releaseNPC(ItemId *object);
uint8_t IsItemCleared(int16_t object);
void Item_bark(ItemId *object, char *text);
void SetItemFrame(int16_t object, int16_t frame);
void Item_setScriptFrame(ItemId *object, int16_t frame);
void Item_stepFrameClamped(ItemId *object, int16_t frame);
void Item_stepFrame(ItemId *object, int16_t frame);
void Item_setMappedFrame(ItemId *object, int16_t frame);
void Item_moveInDirection(ItemId *object, uint8_t dir);
uint8_t Item_isScripted(ItemId *object);
void Item_setScripted(ItemId *object);
void Item_clearScripted(ItemId *object);
void Item_animateStep(ItemId *object);
void Item_turnAndAnimate(ItemId *object, uint8_t animate, uint8_t facing);
void Item_walkStep(ItemId *object, uint8_t facing, int8_t activity);
void SetNPCFacing(int16_t object, uint8_t facing);
void Item_face(ItemId *object, uint8_t facing);
void Item_removeFromPlay(ItemId *object);
int8_t Item_hatch(ItemId *object);
void Item_checkOnScreen(ItemId *object);
uint8_t Item_isAvatarNear(ItemId *object, int16_t distance);
void Item_teleport(ItemId *object, uint8_t frame);
void Item_attackTarget(ItemId *object);
uint8_t Item_isInParty(ItemId *object);
void Item_updateMissile(ItemId *, int16_t value);
void Item_stopMissile(ItemId *, int16_t value);
void Item_playMusic(ItemId *, uint8_t track, uint8_t resume);
void Item_playSound(ItemId *object, uint8_t sound);
void Item_playSpeech(ItemId *, uint8_t speech);
void Item_runUsecode(ItemId *object, uint16_t action);
void Item_hit(ItemId *object, uint8_t amount, uint8_t type);
uint8_t Item_setWeather(ItemId, uint8_t type);
void AdvanceWeather(uint8_t advance);

#endif
