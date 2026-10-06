#ifndef CONSOLE_H
#define CONSOLE_H

class ConsoleWindow {
	int left, top, right, bottom;
	int foreground, background;
	char border;
	int borderForeground, borderBackground;
public:
	int cursorX, cursorY;
	ConsoleWindow(int, int, int, int, int, int, char, int, int);
	virtual void draw();
	void select();
};

class ConsoleSwitch {
	ConsoleWindow *previous;
public:
	ConsoleSwitch(ConsoleWindow *);
	~ConsoleSwitch();
};

void far InitDebugConsole(void);
void ResetConsoleCursor(void);

#endif
