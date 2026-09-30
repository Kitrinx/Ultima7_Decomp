#ifndef MAINCTRL_H
#define MAINCTRL_H

extern const uint8_t DirectionForDirectionKey[11];
extern const uint8_t DirectionForNumberKey[9];
extern uint8_t SingleStepMode;
extern uint8_t AudioDisabled;
extern uint8_t MovementDirection;
extern char TildeString[];
void SteerVehicle(uint8_t direction);
void PollAndProcessKey(uint8_t *steps);

void UseAtPointer(int16_t mouseX, int16_t mouseY);
void ShowCheatKeys();
void ShowVersion();
void ProcessKey(uint16_t key, int16_t mouseX, int16_t mouseY, uint8_t *steps);

#endif
