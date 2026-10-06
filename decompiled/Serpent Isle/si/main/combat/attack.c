/* Serpent Isle SI.EXE, overlay segment 343 (file offsets 0x09fa10 to 0x0a1d93, 9091 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: attack.c */
#include "activity.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "partymov.h"
#include "sprite.h"
#include "gtimer.h"
#include "combat.h"
#include "cheat.h"
#include "random.h"
#include "combmode.h"
#include "combpick.h"
#include "party.h"
#include "text.h"
#include "script.h"
#include "combatai.h"
#include "sche.h"
#include "actitem.h"
#include "actqueue.h"
#include "legalmov.h"
#include "mapview.h"
#include "monsters.h"
#include "voolook.h"
#include "u7ibuf.h"

struct MonsterRef {
	int index;
	MonsterRef() {}
	MonsterRef(int n) { index = n; }
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
	unsigned char none() { return index == 0; }
};

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define IS_VALID(r) ((char) ((r) != 0))
#define KIND(b) ((char) ((int) (b) & 7))
#define IS_KIND(b, k) ((char) (KIND(b) == (k)))
#define FLAG(v, m) ((unsigned char) ((v) & (m)))
#define NPC_ALIGNMENT(r) ((unsigned char) ((NPC((objref *)(r))->status & 0x18) >> 3))
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)

unsigned char SchedulePeriod;
unsigned char AvatarStepRequested, AutorouteActive;
/* Declared char where it is defined; this module reads it unsigned. */
extern unsigned char KillNpcMode;

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define NPC_FLAG(n, bit) ((unsigned char) (((n)->status & (bit)) != 0))
#define ALIGNMENT(n) ((char) (((n)->status & 0x18) >> 3))
#define IS_ATTACK_MODE(r, m) ((unsigned char) ((unsigned char)Item_getQuality(r) == (m)))
#define HAS_PRIMARY(n) ((unsigned char) ((n)->primaryTarget != -1))
#define HAS_SECONDARY(n) ((unsigned char) ((n)->secondaryTarget != -1))
#define HAS_OPPRESSOR(n) ((unsigned char) ((n)->oppressor != -1))
#define HAS_IVR(n) ((unsigned char) ((n)->iVr[0] != -1))
#define MOVE_FLAGS(r, m) ((unsigned char) (Item_getQualityFlags(r) & (m)))
#define IS_CONJURED(n) ((unsigned char) ((n)->typeFlagsHigh & 0x40))
#define WANTS_PRIMARY(n) ((unsigned char) ((n)->typeFlagsHigh & 1))

inline unsigned char HasMovePoints(objref *p) { return CombatGroups.movePoints[(unsigned)Item_getNpcNumber(p)] >= 16; }
inline unsigned char IsActive(objref *p)
{
	ItemInfo info;

	info = GetItemZAndStuff(p);
	if ((unsigned char) (info.kind() == LOCATION_OFF_MAP))
		return 0;
	return IsItemInCellWindow(*p);
}
inline unsigned char IsInAction(NPCRef r) { return NPC(&r)->typeFlagsHigh & 0x20; }

void far StopFlanking(objref *npc)
{
	NPCRef other;
	int i;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&other, i);
		if (other.valid() && ALIGNMENT(NPC(&other)) == ALIGNMENT(NPC(npc))
			&& IS_ATTACK_MODE(&other, ATTACK_FLANK) && !HAS_IVR(NPC(&other)))
			SetAttackMode(other, ATTACK_DEFEND);
	}
}

void far KeepNearAvatar(objref *npc)
{
	int range;
	char wasReady, moved;

	if (NPC_FLAG(NPC(npc), NPC_IN_PARTY))
		range = (GetPartyIndex(*npc) - 1) / 2 * 3 + 3;
	else
		range = 8;
	wasReady = HasMovePoints(npc) ? 1 : 0;
	moved = 0;
	while (Item_greatestDeltaToItem(*npc, AvatarRef) > range && HasMovePoints(npc)) {
		if (PathNextToItem(npc, AvatarRef, 75) == 0
			&& StepToward(npc, Item_getX(AvatarRef), Item_getY(AvatarRef),
			Item_getZ(&AvatarRef), GetMoveStride(npc)) == 0)
			break;
		moved = 1;
		CombatGroups.movePoints[(unsigned)Item_getNpcNumber(npc)] -= 16;
	}
	if (wasReady) {
		if (moved)
			CombatGroups.someoneActed = 1;
		else if (FRAME(ITEM(npc->off)) != 0 && FRAME(ITEM(npc->off)) != 16
			&& !(Item_getQualityFlags(npc) & QUALITY_BUSY))
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
	}
}

void far IdleInCombat(objref *npc)
{
	if (GetMoveStride(npc) == 1 && RollChance(3)) {
		Npc_pushSchedule(npc, WORK_LOITER, -1, -1, 0);
		CUR_SCHED(npc).state = 1;
	} else if (!(Item_getQualityFlags(npc) & QUALITY_BUSY))
		ActionQueue.add(*npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(6) + 5, SCRIPT_END));
}

void far UpdateCombatNPCs()
{
	int i, n, j, mode, dist;
	Coord ownX, ownY, foeX, foeY, lineX, lineY;
	int dx, reach;
	Coord nearAX, nearAY, nearBX, nearBY, farAX, farAY, farBX, farBY;
	int intercept;
	NPCRef actor;
	objref nearest, other;
	NPCRef leader;
	NPCRef target, pursuer;
	objref oppressor;
	unsigned char chosen, stayNear, otherSide, steep, fleeing;
	int targetNum;
	int k, dy;

	otherSide = 0;
	CombatGroups.addMovePoints();
	CombatGroups.clearMarks();
	do {
		CombatGroups.someoneActed = 0;
		for (i = 0; i < NPC_COUNT; i++) {
			GetNpcIbo(&actor, i);
			if (!actor.valid())
				continue;
			if (!IsActive(&actor))
				continue;
			if (MOVE_FLAGS(&actor, 0x20))
				continue;
			if (NPC_FLAG(NPC(&actor), NPC_PARALYZED))
				continue;
			if ((unsigned char) ((char)Item_getHitPoints(&actor) <= 0))
				continue;
			if (NPC_FLAG(NPC(&actor), NPC_DEAD))
				continue;
			if (NPC_FLAG(NPC(&actor), NPC_ASLEEP) && NPC(&actor)->workType != WORK_SLEEP)
				continue;
			if (DoScheduleNpc != -1 && DoScheduleNpc != i && i != 0)
				continue;
			if ((unsigned char)Item_isAvatar(&actor)) {
				if (!AutorouteActive && !AvatarStepRequested) {
					if (IsInCombat(&actor)) {
						if ((unsigned char) ((unsigned char)Item_getQuality(&actor) != ATTACK_MANUAL)) {
							NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
							GetNpcIbo(&target, NPC(&actor)->primaryTarget);
							if (!HAS_PRIMARY(NPC(&actor))) {
								mode = (unsigned char)Item_getQuality(&actor);
								if (mode == ATTACK_DEFEND) {
									if ((char)Item_getHitPoints(&actor) <=
										((unsigned char)(NPC(&actor)->strength & 0x1f)) / 2)
										mode = ATTACK_WEAKEST;
									else
										mode = ATTACK_NEAREST;
								}
								switch (mode) {
								case ATTACK_NEAREST:
									targetNum = ChooseNearestTarget(&actor, 1, 0, 10);
									break;
								case ATTACK_WEAKEST:
									targetNum = ChooseWeakestTarget(&actor, 1, 0, 10);
									break;
								case ATTACK_STRONGEST:
									targetNum = ChooseStrongestTarget(&actor, 1, 0, 10);
									break;
								}
								if (targetNum != -1) {
									Npc_setTarget(&actor, targetNum, 1);
									KillNpcMode = 0;
								}
							} else if ((IsNpcUnconscious(&target) || IS_ATTACK_MODE(&target, ATTACK_FLEE))
								&& !KillNpcMode)
								RemoveFromCombat(actor);
							if (HAS_PRIMARY(NPC(&actor))) {
								NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
								if (PrepareAttack(&actor))
									TakeCombatTurn(&actor);
								else
									ApproachTarget(&actor);
							}
						}
					} else if (NPC(&actor)->workType != WORK_FOLLOW_AVT)
						Npc_runSchedule(&actor);
				} else if (NPC(&actor)->workType != WORK_COMBAT && NPC(&actor)->workType != WORK_FOLLOW_AVT) {
					for (j = 0; j < PartySize; j++) {
						if (!IsInAction(PartyMembers[j])) {
							objref *member = &PartyMembers[j];

							Npc_setSchedule(member, WORK_FOLLOW_AVT);
							CUR_SCHED(member).state = 1;
						}
					}
					for (j = 0; j < DownedPartyCount; j++) {
						if (!IsInAction(DownedPartyMembers[j])) {
							objref *member = &DownedPartyMembers[j];

							Npc_setSchedule(member, WORK_FOLLOW_AVT);
							CUR_SCHED(member).state = 1;
						}
					}
				}
				continue;
			}
			if (!GameTime.running() && !NPC_FLAG(NPC(&actor), NPC_IN_PARTY))
				continue;
			if (IsInCombat(&actor)) {
				if (!HAS_PRIMARY(NPC(&actor)) && !HAS_SECONDARY(NPC(&actor))
					&& !IS_ATTACK_MODE(&actor, ATTACK_FLANK) && !IS_ATTACK_MODE(&actor, ATTACK_PROTECT)) {
					if (!HAS_OPPRESSOR(NPC(&actor))) {
						/* face doubles as a combat idle counter */
						if (NPC(&actor)->face > 0 || !ChooseTarget(actor, 100)) {
							if (NPC_FLAG(NPC(&actor), NPC_IN_PARTY) || IS_CONJURED(NPC(&actor)))
								stayNear = 1;
							else
								stayNear = 0;
							if (NPC(&actor)->face == 20) {
								NPC(&actor)->face = 0;
								if (stayNear)
									KeepNearAvatar(&actor);
							} else {
								NPC(&actor)->face++;
								if (stayNear)
									KeepNearAvatar(&actor);
							}
							if (IS_ATTACK_MODE(&actor, ATTACK_FLEE) && RollChance(stayNear ? 30 : 3)) {
								NPC(&actor)->oppressor = ChooseNearestTarget(&actor, 1, 0, 100);
								if (!HAS_OPPRESSOR(NPC(&actor)))
									NPC(&actor)->oppressor = ChooseNearestTarget(&actor, 1, 1, 100);
								continue;
							}
							if (!stayNear) {
								Npc_pushSchedule(&actor, WORK_LOITER, -1, -1, 0);
								CUR_SCHED(&actor).state = 1;
							}
							continue;
						}
					} else {
						NPC(&actor)->face++;
						fleeing = (unsigned char)Item_getQuality(&actor) == ATTACK_FLEE;
						if (NPC(&actor)->face == (fleeing ? 100 : 30)) {
							NPC(&actor)->face = 0;
							GetNpcIbo(&oppressor, NPC(&actor)->oppressor);
							if (ALIGNMENT(NPC(&oppressor)) == ALIGNMENT(NPC(&actor))) {
								NPC(&actor)->oppressor = -1;
								if (!(Item_getQualityFlags(&actor) & QUALITY_BUSY))
									ActionQueue.add(actor, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
							}
						} else if (!fleeing && RollChance(30))
							ChooseTarget(actor, 100);
						if (!fleeing)
							continue;
					}
				} else
					NPC(&actor)->face = 0;

				if (IS_ATTACK_MODE(&actor, ATTACK_FLEE)) {
					MonsterRef monster;

					monster = GetMonsterNumber(actor);
					if (HAS_OPPRESSOR(NPC(&actor))) {
						GetNpcIbo(&pursuer, NPC(&actor)->oppressor);
						dist = Item_greatestDeltaToItem(actor, pursuer);
						if (!FleeStep(&actor, 1, 1, 1)) {
							if (PrepareAttack(&actor)
								&& (!(Item_getQualityFlags(&pursuer) & QUALITY_INVISIBLE) || dist <= 3))
								TakeCombatTurn(&actor);
							else if (FRAME(ITEM(actor.off)) != 0 && FRAME(ITEM(actor.off)) != 16
								&& !(Item_getQualityFlags(&actor) & QUALITY_BUSY))
								ActionQueue.add(actor, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
						}
						if (RollChance(25)) {
							if ((int)Npc_hasTournamentFlag(&actor))
								continue;
							if (GetAttackRange(&pursuer) >= dist) {
								switch (TYPE(ITEM(actor.off))) {
								case 354:
								case 478:
								case 691:
								case 725:
								case 744:
									SpriteManager_barkOnItem(&gSpriteManager, actor,
										GetGameText(1, GenerateRandomIntegerInRange(3) + 213), 1, 15, 0);
									continue;
								default:
									if ((int)FLAG(monster->extraFlags, 0x20))
										continue;
									SpriteManager_barkOnItem(&gSpriteManager, actor,
										GetGameText(1, GenerateRandomIntegerInRange(7) + 72), 1, 15, 0);
									continue;
								}
							} else {
								switch (TYPE(ITEM(actor.off))) {
								case 354:
								case 478:
								case 691:
								case 725:
								case 744:
									SpriteManager_barkOnItem(&gSpriteManager, actor,
										GetGameText(1, GenerateRandomIntegerInRange(2) + 216), 1, 15, 0);
									continue;
								default:
									if ((int)FLAG(monster->extraFlags, 0x20))
										continue;
									SpriteManager_barkOnItem(&gSpriteManager, actor,
										GetGameText(1, GenerateRandomIntegerInRange(4) + 79), 1, 15, 0);
									continue;
								}
							}
						}
					} else if (RollChance(30)) {
						NPC(&actor)->oppressor = ChooseNearestTarget(&actor, 1, 0, 100);
						if (!HAS_OPPRESSOR(NPC(&actor)))
							NPC(&actor)->oppressor = ChooseNearestTarget(&actor, 1, 1, 100);
					}
				} else if (IS_ATTACK_MODE(&actor, ATTACK_FLANK)) {
					if (HAS_IVR(NPC(&actor))) {
						ApproachTarget(&actor);
						continue;
					}
					if (!FindGroupCentre(&actor, &foeX, &foeY, 1)) {
						KeepNearAvatar(&actor);
						continue;
					}
					FindGroupCentre(&actor, &ownX, &ownY, 0);
					/* Flankers go first beside our own group, then beside the enemy: the farthest open cells
					 * on each side of lines across the gap between the two groups. */
					reach = GetDistance(ownX, ownY, foeX, foeY);
					lineX = (foeX * 4 + ownX) / 5;
					lineY = (foeY * 4 + ownY) / 5;
					dx = -GetDelta(ownX, foeX);
					dy = GetDelta(ownY, foeY);
					if (dy == 0) {
						StopFlanking(&actor);
						i--;
						continue;
					}
					intercept = (int) lineY - dx * lineX / dy;
					if (dx / dy > 1 || dx / dy < -1) {
						if (dx == 0) {
							StopFlanking(&actor);
							i--;
							continue;
						}
						steep = 1;
					} else
						steep = 0;
					for (k = reach; k >= 1; k--) {
						if (steep) {
							farAY = lineY + k;
							farAX = (farAY * dy - intercept * dy) / dx;
						} else {
							farAX = lineX + k;
							farAY = dx * farAX / dy + intercept;
						}
						if (CanTypeMoveTo(farAX, farAY, Item_getZ(&actor), ITEM(actor.off)->typeFrame))
							break;
					}
					for (k = reach; k >= 1; k--) {
						if (steep) {
							farBY = lineY - k;
							farBX = (farBY * dy - intercept * dy) / dx;
						} else {
							farBX = lineX - k;
							farBY = dx * farBX / dy + intercept;
						}
						if (CanTypeMoveTo(farBX, farBY, Item_getZ(&actor), ITEM(actor.off)->typeFrame))
							break;
					}
					lineX = ownX;
					lineY = ownY;
					intercept = (int) lineY - dx * lineX / dy;
					reach = reach * 7 / 10;
					for (k = reach; k >= 1; k--) {
						if (steep) {
							nearAY = lineY + k;
							nearAX = (nearAY * dy - intercept * dy) / dx;
						} else {
							nearAX = lineX + k;
							nearAY = dx * nearAX / dy + intercept;
						}
						if (CanTypeMoveTo(nearAX, nearAY, Item_getZ(&actor), ITEM(actor.off)->typeFrame))
							break;
					}
					for (k = reach; k >= 1; k--) {
						if (steep) {
							nearBY = lineY - k;
							nearBX = (nearBY * dy - intercept * dy) / dx;
						} else {
							nearBX = lineX - k;
							nearBY = dx * nearBX / dy + intercept;
						}
						if (CanTypeMoveTo(nearBX, nearBY, Item_getZ(&actor), ITEM(actor.off)->typeFrame))
							break;
					}
					for (n = 0; n < NPC_COUNT; n++) {
						GetNpcIbo(&other, n);
						if (other.valid() && ALIGNMENT(NPC(&other)) == ALIGNMENT(NPC(&actor))
							&& IS_ATTACK_MODE(&other, ATTACK_FLANK) && !HAS_IVR(NPC(&other))) {
							if (otherSide) {
								if ((unsigned char) (Item_getX(other) != nearAX)
									|| (unsigned char) (Item_getY(other) != nearAY)) {
									NPC(&other)->iVr[0] = Loc(nearAX).value;
									NPC(&other)->iVr[1] = Loc(nearAY).value;
									NPC(&other)->typeFlags = NPC(&other)->typeFlags | 2;
									NPC(&other)->sVr[0] = -1;
									NPC(&other)->sVr[0] = Loc(farAX).value;
									NPC(&other)->sVr[1] = Loc(farAY).value;
								}
							} else {
								if ((unsigned char) (Item_getX(other) != nearBX)
									|| (unsigned char) (Item_getY(other) != nearBY)) {
									NPC(&other)->iVr[0] = Loc(nearBX).value;
									NPC(&other)->iVr[1] = Loc(nearBY).value;
									NPC(&other)->typeFlags = NPC(&other)->typeFlags | 2;
									NPC(&other)->sVr[0] = -1;
									NPC(&other)->sVr[0] = Loc(farBX).value;
									NPC(&other)->sVr[1] = Loc(farBY).value;
								}
							}
							otherSide = !otherSide;
						}
					}
					i--;
				} else if (IS_ATTACK_MODE(&actor, ATTACK_PROTECT)) {
					GetNpcIbo(&leader, CombatGroups.leaders[GetAlignment(&actor)]);
					if (leader.valid()) {
						if (HAS_PRIMARY(NPC(&actor))) {
							NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
							if (PrepareAttack(&actor))
								TakeCombatTurn(&actor);
							else {
								GetNpcIbo(&target, NPC(&actor)->primaryTarget);
								if (Item_greatestDeltaToItem(target, leader) <= GetAttackRange(&target) + 6
									&& !ApproachTarget(&actor) && RollChance(10))
									SetAttackMode(actor, ATTACK_NEAREST);
							}
						} else if (HAS_SECONDARY(NPC(&actor))) {
							if (!RollChance(25)) {
								NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh & ~1;
								if (PrepareAttack(&actor))
									TakeCombatTurn(&actor);
								else
									ApproachTarget(&actor);
							} else {
								Npc_setTarget(&actor, -1, 0);
								RemoveFromCombat(actor);
							}
						} else if (Item_greatestDeltaToItem(actor, leader) > 6) {
							if (HasMovePoints(&actor)) {
								if (PathNextToItem(&actor, leader, 60)) {
									CombatGroups.movePoints[(unsigned)Item_getNpcNumber(&actor)] -= 16;
									CombatGroups.someoneActed = 1;
								} else if (RollChance(10))
									SetAttackMode(actor, ATTACK_NEAREST);
							}
						} else if (FRAME(ITEM(actor.off)) != 0 && FRAME(ITEM(actor.off)) != 16)
							PostScheduleScript(&actor, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
					} else {
						SetAttackMode(actor, ATTACK_NEAREST);
						i--;
					}
				} else if (WANTS_PRIMARY(NPC(&actor))) {
					if (HAS_PRIMARY(NPC(&actor))) {
						if (PrepareAttack(&actor))
							TakeCombatTurn(&actor);
						else if (!ApproachTarget(&actor)) {
							NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh & ~1;
							chosen = 0;
							if (HAS_SECONDARY(NPC(&actor)) || (chosen = ChooseTarget(actor, 100)) != 0) {
								if (PrepareAttack(&actor))
									TakeCombatTurn(&actor);
								else if (!ApproachTarget(&actor) && !chosen) {
									GetNpcIbo(&nearest, ChooseNearestTarget(&actor, 0, 1, 100));
									if (nearest.valid()) {
										Npc_setTarget(&actor, (unsigned)Item_getNpcNumber(&nearest), 0);
										if (PrepareAttack(&actor))
											TakeCombatTurn(&actor);
										else if (!ApproachTarget(&actor)) {
											NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
											IdleInCombat(&actor);
										}
									}
								}
							} else {
								NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
								IdleInCombat(&actor);
							}
						}
					} else if (!ChooseTarget(actor, 100)) {
						if (HAS_SECONDARY(NPC(&actor)))
							NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh & ~1;
						else if (!NPC_FLAG(NPC(&actor), NPC_IN_PARTY))
							IdleInCombat(&actor);
					}
				} else if (!WANTS_PRIMARY(NPC(&actor))) {
					if (!HAS_SECONDARY(NPC(&actor))) {
						NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
						i--;
					} else if (PrepareAttack(&actor))
						TakeCombatTurn(&actor);
					else if (!ApproachTarget(&actor)) {
						NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
						if (HAS_PRIMARY(NPC(&actor)) || ChooseTarget(actor, 100)) {
							if (PrepareAttack(&actor))
								TakeCombatTurn(&actor);
							else if (!ApproachTarget(&actor)) {
								NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh & ~1;
								GetNpcIbo(&nearest, ChooseNearestTarget(&actor, 0, 1, 100));
								if (nearest.valid()) {
									Npc_setTarget(&actor, (unsigned)Item_getNpcNumber(&nearest), 0);
									if (PrepareAttack(&actor))
										TakeCombatTurn(&actor);
									else if (!ApproachTarget(&actor)) {
										NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
										IdleInCombat(&actor);
									}
								} else {
									NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
									IdleInCombat(&actor);
								}
							}
						} else {
							NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh & ~1;
							IdleInCombat(&actor);
						}
					}
				}
			} else if (NPC(&actor)->workType != WORK_FOLLOW_AVT)
				Npc_runSchedule(&actor);
			else if (IS_CONJURED(NPC(&actor)))
				KeepNearAvatar(&actor);
		}
	} while (CombatGroups.someoneActed);
}

void far UpdateNPCs(unsigned char avatarStepping)
{
	if (GameTime.running() && (char) (GameTime.ticks % (TICKS_PER_MINUTE * 15) == 0)) {
		if (AvatarDontMove)
			AvatarDontMove = 2;
		else
			UpdateNpcSchedules();
	}
	if (!avatarStepping)
		UpdateCombatNPCs();
}

unsigned char far IsAvatarDead()
{
	return (GetNpcBufferForIbo(&AvatarRef)->status & NPC_DEAD) != 0;
}


void Combat::clearMarks()
{
	int i;

	for (i = 0; i < 32; i++)
		marks[i] = 0;
}


void Combat::addMovePoints()
{
	int npc;
	unsigned char dexterity;
	ItemInfo info;
	int i;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo((objref *)&npc, i);
		if (IS_VALID(npc)) {
			info = GetItemZAndStuff((objref *)&npc);
			if (!(IS_KIND(info.flags, 4) ? 0 : (int) IsItemInCellWindow(npc)))
				continue;
			if (!FLAG(Item_getQualityFlags((objref *)&npc), QUALITY_BUSY) && !IsNpcUnconscious((objref *)&npc)) {
				dexterity = Npc_getDexterity((objref *)&npc);
				if (movePoints[i] + dexterity < 56)
					movePoints[i] += dexterity;
				else
					movePoints[i] = 56;
			}
		}
	}
}


char Combat::isProtectee(int n)
{
	int npc;

	GetNpcIbo((objref *)&npc, n);
	if (IS_VALID(npc))
		return leaders[NPC_ALIGNMENT(&npc)] == Item_getNpcNumber((objref *)&npc);
	return 0;
}
