#ifndef U7IBUF_H
#define U7IBUF_H

extern uint8_t OinkMode;

#ifdef __cplusplus
extern "C" {
#endif
extern int16_t DeadPartyMembers[12];
extern uint8_t AvatarDontMove;
#ifdef __cplusplus
}
#endif
extern uint8_t ArmageddonDone;
extern int16_t CurrentVehicle;
extern int16_t ActiveBarge;
extern int16_t DeadPartyCount;
char * GetItemName(uint16_t ref);

#endif
