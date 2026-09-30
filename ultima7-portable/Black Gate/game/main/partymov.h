#ifndef PARTYMOV_H
#define PARTYMOV_H

#include "objref.h"

/* PartyMemberFlags bits. */
#define MEMBER_STEPPED  1   /* moved this turn */
#define MEMBER_LOST     2   /* could not keep up with its leader */
#define MEMBER_ON_PATH  4   /* walking a route back to the avatar */

/* The party and the eight compass directions. */
extern char DirectionDX[8], DirectionDY[8];
extern char DirectionBySign[3][3];
extern "C" objref PartyMembers[8];         /* defined as NPCRef; includers compile against objref */
extern "C" objref DownedPartyMembers[16];
extern int8_t PartySize, DownedPartyCount;

void UpdatePartyFollow(void);
int16_t GetPartySize(void);
uint8_t IsPartyMember(objref *item);

extern int8_t FormationFacing;
extern int8_t FormationSize;
extern char FormationSlotX[8][8];
extern char FormationSlotY[8][8];
extern char FormationLeaders[8][8];
extern char FormationSwapPairs[8][8];
extern char DirectionSignRow[8];
extern int8_t PartyFacing;
extern uint8_t PartyMemberFlags[8];
extern int8_t PartyFollowTicks;
extern int8_t AvatarIdleSteps;
int8_t GetSwapPair(int16_t pair, int8_t *first, int8_t *second);
void RotateFormation(uint8_t turn);
int8_t TurnFormation(int8_t facing, int8_t turn);
void SeparateDownedMembers(void);
void SwapPartyOrder(int8_t first, int8_t second);
int8_t CanSwapMembers(int8_t first, int8_t second);
void ReorderParty(void);
int8_t FindMemberInDirection(Coord x, Coord y, int8_t member, int8_t direction);
void SwapMemberPositions(int8_t first, int8_t second);
int8_t CanReachLeader(Coord x, Coord y, int8_t z, int8_t member, Coord leaderX, Coord leaderY, int8_t leaderZ);
void StepAroundMember(int8_t first, int8_t member, int8_t avoid);
int8_t ChooseFollowStep(int8_t member, int8_t head, int8_t direction, int8_t *outDir, int8_t *outDz, int8_t *outOther,
	int16_t *outDist);
int8_t MoveParty(int8_t direction);
void RemovePartyFromCollision(void);
void AddPartyToCollision(void);
objref * GetPartyMembers(void);

#endif
