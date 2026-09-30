#ifndef ITEMCMD_H
#define ITEMCMD_H

struct BitArray;

struct ItemId;

unsigned char IsNPCHalted(int object);
unsigned char Item_isRemoved(ItemId *object);
void PlaySoundSimple(unsigned char sound);
void Item_removeFromWorld(ItemId *object);
void Item_returnToWorld(ItemId *object);
void Item_addToView(ItemId *object);

extern char SpeechFileName[];
extern int FallbackFrames[];
extern BitArray SteppedNPCs;
extern BitArray HaltedNPCs;
void InitNPCSets();
int HasNPCStepped(int npcNum);
void ClearSteppedNPCs();
unsigned char Item_isFree(ItemId *object);
unsigned char Item_hasNoType(ItemId *object);
unsigned char Item_haltNPC(ItemId *object);
void Item_releaseNPC(ItemId *object);
unsigned char IsItemCleared(int object);
void Item_bark(ItemId *object, char *text);
void SetItemFrame(int object, int frame);
void Item_setScriptFrame(ItemId *object, int frame);
void Item_stepFrameClamped(ItemId *object, int frame);
void Item_stepFrame(ItemId *object, int frame);
void Item_setMappedFrame(ItemId *object, int frame);
void Item_moveInDirection(ItemId *object, unsigned char dir);
unsigned char Item_isScripted(ItemId *object);
void Item_setScripted(ItemId *object);
void Item_clearScripted(ItemId *object);
void Item_animateStep(ItemId *object);
void Item_turnAndAnimate(ItemId *object, unsigned char animate, unsigned char facing);
void Item_walkStep(ItemId *object, unsigned char facing, char activity);
void SetNPCFacing(int object, unsigned char facing);
void Item_face(ItemId *object, unsigned char facing);
void Item_removeFromPlay(ItemId *object);
char Item_hatch(ItemId *object);
void Item_checkOnScreen(ItemId *object);
unsigned char Item_isAvatarNear(ItemId *object, int distance);
void Item_teleport(ItemId *object, unsigned char frame);
void Item_attackTarget(ItemId *object);
unsigned char Item_isInParty(ItemId *object);
void Item_updateMissile(ItemId *, int value);
void Item_stopMissile(ItemId *, int value);
void Item_playMusic(ItemId *, unsigned char track, unsigned char resume);
void Item_playSound(ItemId *object, unsigned char sound);
void Item_playSpeech(ItemId *, unsigned char speech);
void Item_runUsecode(ItemId *object, unsigned action);
void Item_hit(ItemId *object, unsigned char amount, unsigned char type);
unsigned char Item_setWeather(ItemId, unsigned char type);
void AdvanceWeather(unsigned char advance);

#endif
