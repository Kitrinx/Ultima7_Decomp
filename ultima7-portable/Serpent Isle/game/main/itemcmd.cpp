/* Serpent Isle SI.EXE, resident segment 16 (file offsets 0x01138a to 0x012654, 4810 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "objref.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "dosio.h"
#include "type.h"
#include "u7npc.h"
#include "collide.h"
#include "item.h"
#include "coord.h"
#include "bitarray.h"
#include "sprite.h"
#include "u7manage.h"
#include "bltshape.h"
#include "slime.h"
#include "barge.h"
#include "trigger.h"
#include "u7ibuf.h"
#include "usehook.h"
#include "camera.h"
#include "cbattack.h"
#include "loadreg.h"
#include "missile.h"
#include "misstrac.h"
#include "party.h"
#include "voice.h"
#include "death.h"
#include "explode.h"
#include "preload.h"
#include "sortitem.h"
#include "mapview.h"
#include "weather.h"
#include "bogus.h"
#include "maps.h"
#include "search.h"
#include "damage.h"
#include "npcref.h"
#include "u7sound.h"
#include "itemcmd.h"


inline objref ItemRef(ItemId &id) { objref r; r.off = id.off; return r; }
inline uint8_t ItemFlags(objref &ref, uint8_t mask) { return Item_getQualityFlags(&ref) & mask; }

inline void SetItemFlags(objref &ref, uint8_t mask)
{
	Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) | mask);
}

inline void ClearItemFlags(objref &ref, uint8_t mask)
{
	Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) & ~mask);
}

inline void ToggleItemFlags(objref &ref, uint8_t mask)
{
	Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) ^ mask);
}

inline void SetFrameBits(objref &ref, int16_t frame)
{
	uint16_t &value = ref.ptr()->typeFrame;

	value = (value & 0x3ff) | ((frame << 10) & 0xfc00);
}

inline uint8_t ItemFlags(objref &&ref, uint8_t mask) { return ItemFlags(ref, mask); }
inline void SetItemFlags(objref &&ref, uint8_t mask) { SetItemFlags(ref, mask); }
inline void ClearItemFlags(objref &&ref, uint8_t mask) { ClearItemFlags(ref, mask); }
inline void ToggleItemFlags(objref &&ref, uint8_t mask) { ToggleItemFlags(ref, mask); }
inline void SetFrameBits(objref &&ref, int16_t frame) { SetFrameBits(ref, frame); }

#define TYPE(record) ((record)->typeFrame & 0x3ff)
#define FRAME(record) (((record)->typeFrame & 0xfc00) >> 10)
#define TYPE_CLASS(record) (gItemTypeInfo[TYPE(record)].typeClass)
#define IS_CLASS(record, n) ((int8_t)(TYPE_CLASS(record) == (n)))
#define IS_NPC(record) ((uint8_t)((ItemTypeClassFlags[TYPE_CLASS(record)] & CLASS_NPC) != 0))
#define HAS_REMAP_FRAME(record) ((int8_t)gItemTypeInfo[TYPE(record)].strangeMovement)
#define EXTRA(off) ((EggRecord *)ItemAt((off)))

extern void Item_move(objref *, uint8_t);
extern "C" void PlaySoundAtItem(uint8_t, int16_t);
extern "C" void PlaySfx(uint8_t number, uint16_t volume, int16_t pan);
extern void SpawnRegionalGuards(objref *origin, uint16_t action);

inline uint8_t HasLinkedDestination(const objref &object)
{
	return EXTRA(object.ptr()->data.extra)->data1.bytes.first != 255;
}

char SpeechFileName[] = "sispeech.spc";
const int16_t FallbackFrames[] = {0, 0, 0, 0, 7, 8, 9, 4, 5, 6, 0, 12, 11, 0, 9, 3};
BitArray SteppedNPCs, HaltedNPCs;

void InitNPCSets()
{
	SteppedNPCs.allocate(NPC_COUNT);
	HaltedNPCs.allocate(NPC_COUNT);
}

int16_t HasNPCStepped(int16_t npcNum)
{
	return SteppedNPCs.data[npcNum >> 3] & (1 << (npcNum & 7));
}

void ClearSteppedNPCs()
{
	SteppedNPCs.clear();
}

uint8_t IsNPCHalted(int16_t object)
{
	int16_t npcNum;

	if (!IS_NPC(objref(object).ptr()))
		return 0;
	npcNum = Item_getNpcNumber(&objref(object));
	return HaltedNPCs.data[npcNum >> 3] & (1 << (npcNum & 7));
}

uint8_t Item_isRemoved(ItemId *object)
{
	for (objref current(DetachedItems); current.valid(); current.off = current.ptr()->next) {
		if (object->off == current.off)
			return 1;
	}
	return 0;
}

uint8_t Item_isFree(ItemId *object)
{
	objref ref(object->off);
	ItemInfo info = GetItemZAndStuff(&ref);

	return info.kind() == LOCATION_DELETED;
}

uint8_t Item_hasNoType(ItemId *object)
{
	return TYPE(objref(object->off).ptr()) == 0;
}

uint8_t Item_haltNPC(ItemId *object)
{
	if (IS_NPC(objref(object->off).ptr())) {
		HaltedNPCs.setBit(Item_getNpcNumber(&objref(object->off)));
		return 1;
	}
	return 0;
}

void Item_releaseNPC(ItemId *object)
{
	if (IS_NPC(objref(object->off).ptr()))
		HaltedNPCs.clearBit(Item_getNpcNumber(&objref(object->off)));
	Item_clearScripted(object);
}

uint8_t IsItemCleared(int16_t object)
{
	return objref(object).ptr()->typeFrame == 0;
}

void Item_bark(ItemId *object, char *text)
{
	SpriteManager_barkOnItem(&gSpriteManager, object->off, text, 5, 15, 0);
}

void SetItemFrame(int16_t object, int16_t frame)
{
	uint16_t shape = TYPE(objref(object).ptr());

	if (shape == 529)   /* slime */
		return;
	if (IS_NPC(objref(object).ptr())) {
		uint16_t direction =
			FacingFrameOffsets[(uint8_t) ((GetNpcBufferForIbo(&objref(object))->status & 7) >> 1)];

		if (gShapeManager.isFrameEmpty(shape, (direction + frame) & 0x1f)) {
			/* Frame -1 read the word before the table on DOS: the "c" ending SpeechFileName. */
			frame = frame == -1 ? 0x63 : FallbackFrames[frame];
			if (gShapeManager.isFrameEmpty(shape, (direction + frame) & 0x1f))
				frame = 0;
		}
		SetFrameBits(objref(object), direction + frame);
	} else {
		uint16_t count = ShapeManager_getFrameCount(&gShapeManager, shape);

		Item_setFrame(&objref(object), (frame + count) % count);
	}
}

void Item_setScriptFrame(ItemId *object, int16_t frame)
{
	SetItemFrame(object->off, frame);
}

static uint16_t Item_getFrame(ItemId *object)
{
	if (IS_NPC(objref(object->off).ptr()))
		return FRAME(objref(object->off).ptr()) & 0xf;
	return FRAME(objref(object->off).ptr());
}

void Item_stepFrameClamped(ItemId *object, int16_t frame)
{
	Item_setScriptFrame(object, ClampInt(0, Item_getFrame(object) + frame,
		ShapeManager_getFrameCount(&gShapeManager, TYPE(objref(object->off).ptr())) - 1));
}

void Item_stepFrame(ItemId *object, int16_t frame)
{
	Item_setScriptFrame(object, Item_getFrame(object) + frame);
}

void Item_setMappedFrame(ItemId *object, int16_t frame)
{
	if (HAS_REMAP_FRAME(objref(object->off).ptr()) && HasFixedFrames(TYPE(objref(object->off).ptr()))) {
		int16_t mappedFrame = GetStrangeMoverFrame(objref(object->off), frame);

		if (mappedFrame >= 0)
			Item_setScriptFrame(object, mappedFrame);
	} else
		Item_setScriptFrame(object, frame);
}

/* barges also take 8 for down and 9 for up */
void Item_moveInDirection(ItemId *object, uint8_t dir)
{
	if (!((uint8_t)(TYPE_CLASS(objref(object->off).ptr()) == TYPE_CLASS_BARGE))) {
		Item_move(&objref(object->off), dir);
	} else {
		objref ref = object->off;

		switch (dir) {
		case 8:
			Barge_descend(ref);
			break;
		case 9:
			Barge_rise(ref);
			break;
		default:
			Barge_move(ref, dir, 4);
			break;
		}
	}
}

uint8_t Item_isScripted(ItemId *object)
{
	objref ref(object->off);

	return Item_getQualityFlags(&ref) & QUALITY_BUSY;
}

void Item_setScripted(ItemId *object)
{
	SetItemFlags(objref(object->off), 0x20);
}

void Item_clearScripted(ItemId *object)
{
	ClearItemFlags(objref(object->off), 0x20);
}

void Item_animateStep(ItemId *object)
{
	int16_t frame;

	if (!SteppedNPCs.test(Item_getNpcNumber(&objref(object->off)))) {
		if (objref(object->off) != AvatarRef && Item_getFrame(object))
			frame = 0;
		else {
			if (ItemFlags(objref(object->off), 4))
				frame = 1;
			else
				frame = 2;
			ToggleItemFlags(objref(object->off), 4);
		}
		Item_setScriptFrame(object, frame);
		uint16_t shape = TYPE(objref(object->off).ptr());
		SteppedNPCs.setBit(Item_getNpcNumber(&objref(object->off)));
	}
}

void Item_turnAndAnimate(ItemId *object, uint8_t animate, uint8_t facing)
{
	if (facing != 255)
		Item_face(object, facing);
	if (!animate)
		Item_setScriptFrame(object, 0);
	else
		Item_animateStep(object);
}

void Item_walkStep(ItemId *object, uint8_t facing, int8_t activity)
{
	if (facing != 255)
		Item_face(object, facing);
	Item_animateStep(object);
	Item_move(&objref(object->off), facing, activity);
}

void SetNPCFacing(int16_t object, uint8_t facing)
{
	int8_t turn = 1;

	if (facing % 2) {
		objref ref(object);
		int16_t difference = facing - (uint8_t)(GetNpcBufferForIbo(&ref)->status & 7);
		if (difference < 0)
			difference = -difference;
		if (difference > 4)
			difference = 8 - difference;
		if (difference <= 1)
			turn = 0;
	}
	if (turn) {
		objref ref(object);
		NpcBuffer *record = GetNpcBufferForIbo(&ref);
		record->status &= 0xfff8;
		record->status |= facing & 7;
		SetItemFrame(object, (objref(object).ptr()->typeFrame & 0x7c00) >> 10 & 0xf);
	}
}

void Item_face(ItemId *object, uint8_t facing)
{
	SetNPCFacing(object->off, facing);
}

void Item_removeFromPlay(ItemId *object)
{
	if (IS_CLASS(objref(object->off).ptr(), TYPE_CLASS_HUMAN)
		|| (IS_CLASS(objref(object->off).ptr(), TYPE_CLASS_MONSTER) && !IsTemporary(&objref(object->off)))) {
		if (objref(object->off) == AvatarRef)
			RunUsable(4, objref(object->off), 0x60e);
		else
			SendNPCToLunch(*(NPCRef *) object);
	} else {
		Item_delete(&objref(object->off));
	}
}

int8_t Item_hatch(ItemId *object)
{
	int8_t accepted = 0;

	if (!ArmageddonDone) {
		int8_t usable = IS_CLASS(objref(object->off).ptr(), TYPE_CLASS_EGG)
			&& (int8_t)EXTRA(objref(object->off).ptr()->data.extra)->kind
			&& !EXTRA(objref(object->off).ptr()->data.extra)->hatched;
		if (usable != 0) {
			if (Egg_hatch(objref(object->off)) != 0)
				accepted = 1;
		}
	}
	return accepted;
}

void Item_activateRepeatingEgg(ItemId *object)
{
	EXTRA(objref(object->off).ptr()->data.extra)->repeat = 1;
	Egg_activate(objref(object->off), 0);
}

void Item_setEggCriteria(ItemId *object, uint8_t criteria, uint8_t distance)
{
	objref egg(object->off);

	Egg_setCriteriaAndDistance(&egg.off, criteria, distance);
}

void Item_checkOnScreen(ItemId *object)
{
	gCamera.isOnScreen(Item_getX(objref(object->off)), Item_getY(objref(object->off)));
}

uint8_t Item_isAvatarNear(ItemId *object, int16_t distance)
{
	int16_t dx = GetMagnitude(Item_getX(AvatarRef) - Item_getX(objref(object->off)));
	int16_t dy = GetMagnitude(Item_getY(AvatarRef) - Item_getY(objref(object->off)));
	uint8_t within = LargerOf(dx, dy) <= distance;

	if (within && IS_CLASS(objref(object->off).ptr(), TYPE_CLASS_EGG)
		&& (uint8_t)EXTRA(objref(object->off).ptr()->data.extra)->kind == EGG_TELEPORT) {
		if (Item_getZ(&AvatarRef) != Item_getZ(&objref(object->off)))
			within = 0;
	}
	return within;
}

void Item_teleport(ItemId *object, uint8_t frame)
{
	Loc x, regionX, y, regionY;
	int16_t z = 0;

	if (HasLinkedDestination(objref(object->off))) {
		AreaSearch destination;
		regionX = RegionX[CurrentRegion];
		regionY = RegionY[CurrentRegion];
		FindItemInArea(&destination, regionX, regionY, regionX + 512, regionY + 512, 16, 275, frame, 6); /* Egg */
		if (destination.current.valid() == 0)
			return;
		x = Item_getX(destination.current);
		y = Item_getY(destination.current);
		z = Item_getZ(&destination.current);
	}
	if (!HasLinkedDestination(objref(object->off))) {
		x = RegionX[EXTRA(objref(object->off).ptr()->data.extra)->data1.bytes.second]
			+ EXTRA(objref(object->off).ptr()->data.extra)->data2.bytes.first;
		y = RegionY[EXTRA(objref(object->off).ptr()->data.extra)->data1.bytes.second]
			+ EXTRA(objref(object->off).ptr()->data.extra)->data2.bytes.second;
	}
	TeleportParty(x, y, z);
}

void Item_attackTarget(ItemId *object)
{
	objref ref(object->off);

	if (Npc_hasItemTarget(&ref)) {
		StrikeItemWithWeapon(ref, Npc_getItemTarget(&ref), Npc_getTargetWeapon(&ref));
	} else {
		Coord x, y;
		int16_t z;
		Npc_getTargetCoords(&ref, &x.value, &y.value, &z);
		StrikeCoordsWithWeapon(ref, x, y, z, Npc_getTargetWeapon(&ref));
	}
}

uint8_t Item_isInParty(ItemId *object)
{
	return (GetNpcBufferForIbo(&objref(object->off))->status & NPC_IN_PARTY) != 0;
}

/* Declared void in the original, but callers test the result left in AX. */
uint8_t Item_updateMissile(ItemId *, int16_t value)
{
	return UpdateMissile(value);
}

uint8_t Item_stopMissile(ItemId *, int16_t value)
{
	return StopMissile(value);
}

void Item_playMusic(ItemId *, uint8_t track, uint8_t resume)
{
	if (track != 0)
		PlayMusic(track);
	MusicResume = resume;
}

void Item_playSound(ItemId *object, uint8_t sound)
{
	PlaySoundAtItem(sound, objref(object->off));
}

void PlaySoundSimple(uint8_t sound)
{
	PlaySfx(sound, 255, 64);
}

void Item_playSpeech(ItemId *, uint8_t speech)
{
	if (speech >= 0 && speech <= 26)
		SpeechPlayer.playFile(SpeechFileName, speech);
}

void Item_runUsecode(ItemId *object, uint16_t action)
{
	if (action >= 30000) {
		switch (action) {
		case 30000:
			ResurrectBody(objref(object->off), Item_getX(objref(object->off)),
				Item_getY(objref(object->off)), Item_getZ(&objref(object->off)));
			break;
		case 31001:
			RunUsable(7, object->off, -1);
			break;
		case 31002:
			RunUsable(5, object->off, -1);
			break;
		case 31003:
			RunUsable(6, object->off, -1);
			break;
		default:
			SpawnRegionalGuards(&objref(object->off), action);
			break;
		}
	} else if (IS_CLASS(objref(object->off).ptr(), TYPE_CLASS_EGG)) {
		if ((!InDungeon && !IsUnderMountain(objref(object->off)))
			|| (InDungeon && IsUnderMountain(objref(object->off))))
			RunUsable(3, object->off, action);
	} else if (action == 704)   /* powder keg */
		ExplodePowderKeg(objref(object->off));
	else
		RunUsable(2, object->off, action);
}

void Item_hit(ItemId *object, uint8_t amount, uint8_t type)
{
	ReduceHealth(ItemRef(*object), amount, type, 0);
}

void Item_removeFromWorld(ItemId *object)
{
	Item_detach(&objref(object->off));
}

void Item_returnToWorld(ItemId *object)
{
	PlaceItem(&objref(object->off), Item_getX(objref(object->off)), Item_getY(objref(object->off)));
}

void Item_addToView(ItemId *object)
{
	MainWorldView.repaintItem(object->off);
}

uint8_t Item_setWeather(ItemId, uint8_t type)
{
	if (type == 255) {
		Weather_stop(&CurrentWeather);
		return 1;
	} else if (CurrentWeather.type == 0) {
		SetWeather(&CurrentWeather, type);
		return 1;
	}
	return 0;
}

void AdvanceWeather(uint8_t advance)
{
	if (CurrentWeather.type)
		UpdateWeather(&CurrentWeather, advance);
}

extern "C" void ResetItemcmdGlobals(void)
{
	memcpy(SpeechFileName, "sispeech.spc", sizeof(SpeechFileName));
	memset((void *)&SteppedNPCs, 0, sizeof(SteppedNPCs));
	memset((void *)&HaltedNPCs, 0, sizeof(HaltedNPCs));
}

extern "C" void ConstructItemcmdGlobals(void)
{
	new (&SteppedNPCs) BitArray();
	new (&HaltedNPCs) BitArray();
}
