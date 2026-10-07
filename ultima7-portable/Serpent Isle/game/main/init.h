#ifndef INIT_H
#define INIT_H

/* Start-up, shutdown and fatal errors. */

#ifdef __cplusplus
extern "C"
#endif
void FatalError(char *, ...);

#ifdef __cplusplus
void AssertFail(char *, int16_t);
void HaltWithMessage(char *, int16_t, char *, ...);
void QuitToDos(void);
#else
void FatalError(char *, ...);
#endif

#ifdef __cplusplus
extern "C" {
#endif
void InitEnvironment(void);
void ShutDown(void);
#ifdef __cplusplus
}
#endif
void EndGame(void);
void ExitForEndgame(void);

extern uint8_t PlainErrors;

extern int32_t ViewportFirstRow;
#ifdef __cplusplus
extern "C" {
#endif
extern int16_t FlatModeFlags;
#ifdef __cplusplus
}
#endif

#endif
