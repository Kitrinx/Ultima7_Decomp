#ifndef CREDITS_H
#define CREDITS_H

namespace MainMenu {

/* The screens the main menu plays after the game or on request. */
void ShowTheEnd(uint32_t pause);
void ShowQuotes(uint32_t stepTicks, int16_t unused);
void ShowCredits(uint32_t stepTicks, int16_t unused);

}

#endif
