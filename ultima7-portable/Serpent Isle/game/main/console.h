#ifndef CONSOLE_H
#define CONSOLE_H

class ConsoleWindow {
	int16_t left, top, right, bottom;
	int16_t foreground, background;
	int8_t border;
	int16_t borderForeground, borderBackground;
public:
	int16_t cursorX, cursorY;
	ConsoleWindow(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int8_t, int16_t, int16_t);
	virtual void draw();
	void select();
};

class ConsoleSwitch {
	ConsoleWindow *previous;
public:
	ConsoleSwitch(ConsoleWindow *);
	~ConsoleSwitch();
};

void InitDebugConsole(void);
void ResetConsoleCursor(void);

#endif
