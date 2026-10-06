#ifndef INTRO_H
#define INTRO_H

#include "digital.h"
#include "font.h"
#include "iff.h"
#include "palette.h"
#include "textwin.h"
#include "xmidi.h"

/* The launcher's selector for what runs next: the main menu. */
#define EXIT_SELECTOR   2

/* A voice line: a VOCF chunk played through the speech driver, or only captions when printerOnly. */
struct Yapper {
	unsigned char printerOnly;
	VocSound *sound;
	void far *data;
	Yapper(int printerOnly);
	Yapper(char *name);
	~Yapper();
	void release();
	void load(char *name);
	void play();
	void say(char *text, int y, int justify);
	void wait(int ticks);
	unsigned char isDone();
};

extern int FadeSpeed;
extern IffFile DataFile;
extern XmidiSong *Score;
extern SoundDriver *SpeechDriver;
extern SoundDriver *MusicDriver;
extern Rgb Black;
extern void far *FontData;
extern unsigned char Speech;
extern MemHandle FontHandle;
extern ShapeFont *TextFont;
extern TextWindow *Window;
extern int Jive;
extern int GuardianMouth[];

unsigned char WaitOrKey(unsigned long ticks);
void Cleanup(int normal);
void Error(char *format, ...);
void Quit(char *message);
void ReadError();
void CheckHeap(char *file, int line);
void Checkpoint(char *file, int line, char *message);
void LoadFont(View *view);

#endif
