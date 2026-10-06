#ifndef MAINCTRL_H
#define MAINCTRL_H

extern unsigned char DirectionForDirectionKey[11];
extern unsigned char DirectionForNumberKey[9];
extern unsigned char SingleStepMode;
extern unsigned char AudioDisabled;
extern unsigned char MovementDirection;
extern char TildeString[];
void far SteerVehicle(unsigned char direction);
void far PollAndProcessKey(unsigned char *steps);

void far UseAtPointer(int mouseX, int mouseY);
void far ShowCheatKeys();
void far ShowVersion();
void far ProcessKey(unsigned key, int mouseX, int mouseY, unsigned char *steps);

#endif
