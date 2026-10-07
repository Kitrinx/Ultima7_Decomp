#ifndef INTRO_H
#define INTRO_H

namespace Intro {

/* The launcher's selector for what runs next: the main menu. */
#define EXIT_SELECTOR   2

/* A voice line: a VOCF chunk played through the speech driver, or only captions when printerOnly. */
struct Yapper {
	uint8_t printerOnly;
	VocSound *sound;
	void *data;
	Yapper(int printerOnly);      /* int, so Yapper(0) does not also match Yapper(char *) */
	Yapper(char *name);
	~Yapper();
	void release();
	void load(char *name);
	void play();
	void say(char *text, int16_t y, int16_t justify);
	void wait(int16_t ticks);
	uint8_t isDone();
};

extern int16_t FadeSpeed;
extern IffFile DataFile;
extern XmidiSong *Score;
extern SoundDriver *SpeechDriver;
extern SoundDriver *MusicDriver;
extern Rgb Black;
extern void *FontData;
extern uint8_t Speech;
extern ShapeFont *TextFont;
extern TextWindow *Window;
extern int16_t Jive;
extern const int16_t GuardianMouth[];

uint8_t WaitOrKey(uint32_t ticks);
void Cleanup(int16_t normal);
void Error(char *format, ...);
void Quit(char *message);
void ReadError();
void CheckHeap(char *file, int16_t line);
void Checkpoint(char *file, int16_t line, char *message);
void LoadFont(View *view);

}

#endif
