#ifndef UCCTRL_H
#define UCCTRL_H

#define CALL_DEPTH  100

/* the callers' saved state */
struct CallStack {
	int16_t items[CALL_DEPTH];
	int16_t count;
#ifdef __cplusplus
	CallStack() { count = 0; }
#endif
};

#ifdef __cplusplus
extern "C" {
#endif
void CallStack_push(struct CallStack *s, int16_t value);
int16_t CallStack_pop(struct CallStack *s);
#ifdef __cplusplus
}
#endif

#endif
