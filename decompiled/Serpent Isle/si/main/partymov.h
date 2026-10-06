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
extern objref PartyMembers[8];         /* defined as NPCRef; includers compile against objref */
extern objref DownedPartyMembers[16];
extern char PartySize, DownedPartyCount;

void far UpdatePartyFollow(void);
int far GetPartySize(void);
unsigned char far IsPartyMember(objref *item);

extern char FormationFacing;
extern char FormationSize;
extern char FormationSlotX[8][8];
extern char FormationSlotY[8][8];
extern char FormationLeaders[8][8];
extern char FormationSwapPairs[8][8];
extern char DirectionSignRow[8];
extern char PartyFacing;
extern unsigned char PartyMemberFlags[8];
extern char PartyFollowTicks;
extern char AvatarIdleSteps;
char far GetSwapPair(int pair, char *first, char *second);
void far RotateFormation(unsigned char turn);
char far TurnFormation(char facing, char turn);
void far SeparateDownedMembers(void);
void far SwapPartyOrder(char first, char second);
char far CanSwapMembers(char first, char second);
void far ReorderParty(void);
char far FindMemberInDirection(Coord x, Coord y, char member, char direction);
void far SwapMemberPositions(char first, char second);
char far CanReachLeader(Coord x, Coord y, char z, char member, Coord leaderX, Coord leaderY, char leaderZ);
void far StepAroundMember(char first, char member, char avoid);
char far ChooseFollowStep(char member, char head, char direction, char *outDir, char *outDz, char *outOther,
	int *outDist);
char far MoveParty(char direction);
void far RemovePartyFromCollision(void);
void far AddPartyToCollision(void);
objref *far GetPartyMembers(void);

#endif
