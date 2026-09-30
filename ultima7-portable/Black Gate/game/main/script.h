#ifndef SCRIPT_H
#define SCRIPT_H

/* A script buffer starts with its length byte; appends stop at 127 bytes. */
void AppendScriptByte(uint8_t *buf, uint8_t c);
void AppendScriptWord(uint8_t *buf, int16_t w);

struct ScriptBuffer;
extern ScriptBuffer ScriptPacket;

/* Packs byte arguments up to SCRIPT_END into ScriptPacket; returns it, length byte first. */
char *MakeScript(int32_t first, ...);

/* Script actions; MakeScript takes one per byte and stops at SCRIPT_END. */
#define SCRIPT_END              0xaeae
#define SCRIPT_CONTINUE         1
#define SCRIPT_LOOP             11
#define SCRIPT_NO_HALT          35
#define SCRIPT_WAIT             39
#define SCRIPT_FINISH           44
#define SCRIPT_REMOVE           45
#define SCRIPT_FRAME            70
#define SCRIPT_NEXT_FRAME_MAX   77
#define SCRIPT_NEXT_FRAME       78
#define SCRIPT_PREV_FRAME_MIN   79
#define SCRIPT_PREV_FRAME       80
#define SCRIPT_TELEPORT         81
#define SCRIPT_STEP             83
#define SCRIPT_MUSIC            84
#define SCRIPT_USECODE          85
#define SCRIPT_SPEECH           86
#define SCRIPT_SFX              88
#define SCRIPT_FACE             89
#define SCRIPT_WEATHER          90
#define SCRIPT_STAND_FRAME      97
#define SCRIPT_READY_FRAME      100
#define SCRIPT_RAISE1_FRAME     101
#define SCRIPT_EXTEND1_FRAME    102
#define SCRIPT_THRUST1_FRAME    103
#define SCRIPT_RAISE2_FRAME     104
#define SCRIPT_EXTEND2_FRAME    105
#define SCRIPT_THRUST2_FRAME    106
#define SCRIPT_SIT_FRAME        107
#define SCRIPT_BEND_FRAME       108
#define SCRIPT_KNEEL_FRAME      109
#define SCRIPT_LIE_FRAME        110
#define SCRIPT_UP_FRAME         111
#define SCRIPT_OUT_FRAME        112
#define SCRIPT_HIT              120

#endif
