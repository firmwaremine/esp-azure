/*
 * LCDIIC.h
 *
 *  Created on: Aug 15, 2020
 *      Author: mfawz
 */

#ifndef MAIN_LCDIIC_H_
#define MAIN_LCDIIC_H_
#include <inttypes.h>

void                LCDIIC_vInit();
void                LCDIIC_vGoHome();
void                LCDIIC_vSetCursorPosition(uint8_t u8Column, uint8_t u8Row);
void                LCDIIC_vShowStringAt(uint8_t u8Column, uint8_t u8Row, char *ps8Character);
void                LCDIIC_vTurnOnDisplay();
void                LCDIIC_vTurnOffDisplay();
void                LCDIIC_vTurnOnUnderlineCursor();
void                LCDIIC_vTurnOffUnderlineCursor();
void                LCDIIC_vTurnOnBlinkingCursor();
void                LCDIIC_vTurnOffBlinkingCursor();
void                LCDIIC_vShowCharacter(int8_t s8Character);
void                LCDIIC_vShowString(char *s8Character);
void                LCDIIC_vScrollDisplayLeft();
void                LCDIIC_vScrollDisplayRight();
void                LCDIIC_vRightToLeft();
void                LCDIIC_vLeftToRight();
void                LCDIIC_vTurnOnAutoScroll();
void                LCDIIC_vTurnOffAutoScroll();
void                LCDIIC_vClearDisplay();


#endif /* MAIN_LCDIIC_H_ */
