/*
 * LCDIIC.c
 *
 *  Created on: Aug 15, 2020
 *      Author: mfawz
 */
#include "LCDIIC.h"
#include "azure_c_shared_utility/threadapi.h"
#include "i2c_driver.h"

/*
 * CLCD Commands
 */
#define CLEAR_DISPLAY_HOME_CURSOR_CMD  0x01   /*!< Clear Display and Home the Cursor */
#define GO_TO_HOME_POSITION_CMD        0x02   /*!< Return Cursor and CLCD to Home Position */
#define ENTERY_MODE_SET_CMD            0x04   /*!< Set Entry Mode (enable/disable screen + cursor moving direction) */
#define DISPLAY_ON_OFF_CONTROL_CMD     0x08   /*!< Return Cursor and CLCD to Home Position */
#define CURSOR_DISPLAY_SHIFT_CMD       0x10   /*!< Move the cursor and shift the display without changing */
#define FUNCTION_SET_CMD               0x20   /*!< Set function mode */
#define SET_CGRAM_ADDRESS_CMD          0x40   /*!< Set CGRAM address */
#define SET_DDRAM_ADDRESS_CMD          0x80   /*!< Set DDRAM address */
/*
 * Entry Mode Set
*/
#define ACCOMPANIES_DISPLAY_SHIFT      0x01
#define INCREMENT_MOVE_DIRECTION       0x02
#define DECREMENT_MOVE_DIRECTION       0x00
/*
 * Display ON/OFF Control
*/
#define DISPLAY_ON                     0x04
#define DISPLAY_OFF                    0x00
#define CURSOR_ON                      0x02
#define CURSOR_OFF                     0x00
#define BLINKING_CURSOR                0x01
#define SOLID_CURSOR                   0x00
/*
 * Cursor and display shift
*/
#define DISPLAY_SHIFT                  0x08
#define CURSOR_MOVE                    0x00
#define SHIFT_RIGHT                    0x04
#define SHIFT_LEFT                     0x00
/*
 * Function Set
*/
#define SET_4BITS_DATA_LINES           0x00
#define SET_8BITS_DATA_LINES           0x10
#define SET_TWO_LINE_DISPLAY           0x08
#define SET_ONE_LINE_DISPLAY           0x00
#define SET_FONT_SIZE_5_10_DOTS        0x04
#define SET_FONT_SIZE_5_7_DOTS         0x00

#define BCF574_ADDRESS   0x27
#define RS_DESELECT      0x01
#define RS_SELECT        0x00
#define WRITE_ENABLE     0x00
#define READ_ENABLE      0x02
#define CHIP_SELECT      0x04
#define CHIP_SELECT_LOW  0x00
#define KATHOD_ENABLE    0x08
#define KATHOS_DISABLE   0x00

#define SCREEN_WIDTH     20
#define SCREEN_HIGH      4

static void LCDIIC_vWriteCommand(uint8_t u8Command);
static void LCDIIC_vInitIO();
static void LCDIIC_vWriteData(uint8_t u8Data);
static uint8_t u8DisplayMode;
static uint8_t u8DisplayControl;
static uint8_t u8DisplayStartRawAddress[4] = {0x00, 0x40, 0x14, 0x54};
void LCDIIC_vInit(void)
{
    uint8_t  u8ControlWord = 0;
    LCDIIC_vInitIO();
    ThreadAPI_Sleep(2000);
    LCDIIC_vWriteCommand(0x03);
    ThreadAPI_Sleep(150U);
    LCDIIC_vWriteCommand( GO_TO_HOME_POSITION_CMD);
    u8ControlWord |= SET_FONT_SIZE_5_10_DOTS;
    u8ControlWord |= SET_TWO_LINE_DISPLAY;
    u8ControlWord |= SET_4BITS_DATA_LINES;
    u8ControlWord |= FUNCTION_SET_CMD;
    LCDIIC_vWriteCommand( u8ControlWord);
    /*
     * Set Display control On/OFF and cursor type Blinking/Solid
     */
    u8ControlWord = 0U;
    u8ControlWord = CURSOR_OFF;
    u8ControlWord |= (DISPLAY_ON_OFF_CONTROL_CMD |DISPLAY_ON);
    LCDIIC_vWriteCommand( u8ControlWord);
    u8DisplayControl = u8ControlWord;
    ThreadAPI_Sleep(5);
    /*
     * Entry mode setting
     */
    u8ControlWord = 0U;
    u8ControlWord |= INCREMENT_MOVE_DIRECTION;
    u8ControlWord |= ENTERY_MODE_SET_CMD;
    LCDIIC_vWriteCommand(u8ControlWord);
    u8DisplayMode = u8ControlWord;
    ThreadAPI_Sleep(5);
    /*
     * Clear the screen and go to home position
     */
    LCDIIC_vWriteCommand( CLEAR_DISPLAY_HOME_CURSOR_CMD);
    ThreadAPI_Sleep(5);
}
void LCDIIC_vScrollDisplayRight()
{
    LCDIIC_vWriteCommand( (CURSOR_DISPLAY_SHIFT_CMD | DISPLAY_SHIFT | SHIFT_RIGHT));
}
void LCDIIC_vScrollDisplayLeft()
{
   LCDIIC_vWriteCommand( (CURSOR_DISPLAY_SHIFT_CMD | DISPLAY_SHIFT | SHIFT_LEFT));
}
void LCDIIC_vLeftToRight()
{

    u8DisplayMode |= INCREMENT_MOVE_DIRECTION;
    LCDIIC_vWriteCommand( u8DisplayMode);
}
void LCDIIC_vRightToLeft()
{

    u8DisplayMode &= (~INCREMENT_MOVE_DIRECTION);
    LCDIIC_vWriteCommand( u8DisplayMode);
}
void LCDIIC_vTurnOnAutoScroll()
{

    u8DisplayMode |= ACCOMPANIES_DISPLAY_SHIFT;
    LCDIIC_vWriteCommand( u8DisplayMode);
}
void LCDIIC_vTurnOffAutoScroll()
{

    u8DisplayMode &= (~ACCOMPANIES_DISPLAY_SHIFT);
    LCDIIC_vWriteCommand( u8DisplayMode);
}
void LCDIIC_vShowCharacter(int8_t s8Character)
{
    LCDIIC_vWriteData( s8Character);
}
void LCDIIC_vShowString(char *ps8Character)
{
    char *ps8CurrentCharacter = ps8Character;

    while(*ps8CurrentCharacter != '\0')
    {
        LCDIIC_vWriteData( (uint8_t)*ps8CurrentCharacter);
        ps8CurrentCharacter++;
    }
}
void LCDIIC_vShowStringAt(uint8_t u8Column, uint8_t u8Row, char *ps8Character)
{
    char *ps8CurrentCharacter = ps8Character;
    uint8_t u8CurrentRow = u8Row, u8CurrentCoulmn = u8Column;

    LCDIIC_vSetCursorPosition(u8CurrentCoulmn, u8CurrentRow);
    while(*ps8CurrentCharacter != '\0')
    {
        LCDIIC_vWriteData( (uint8_t)*ps8CurrentCharacter);
        ps8CurrentCharacter++;
        u8CurrentCoulmn++;
        if(u8CurrentCoulmn == SCREEN_WIDTH)
        {
            u8CurrentCoulmn = 0;
            u8CurrentRow++;
            if(u8CurrentRow == SCREEN_HIGH)
            {
                u8CurrentRow = 0;
            }
            LCDIIC_vSetCursorPosition(u8CurrentCoulmn, u8CurrentRow);
        }
    }
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vClearDisplay(void)

@brief         It is responsible to Clear the CLCD


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vClearDisplay()
{
    LCDIIC_vWriteCommand(CLEAR_DISPLAY_HOME_CURSOR_CMD);
    ThreadAPI_Sleep(2000);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vGoHome(void)

@brief         It is responsible to set the cursor in the home position


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vGoHome()
{
    LCDIIC_vWriteCommand( GO_TO_HOME_POSITION_CMD);
    ThreadAPI_Sleep(2000);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vSetCursorPosition(LCDIICstrHandlerType *pstrResource, uint8_t u8Column, uint8_t u8Row)

@brief         It is responsible to set the cursor position to specific column and row


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vSetCursorPosition(uint8_t u8Column, uint8_t u8Row)
{
    uint8_t u8ControlWord;

    u8ControlWord = SET_DDRAM_ADDRESS_CMD | ((u8Column) + u8DisplayStartRawAddress[u8Row]);
    LCDIIC_vWriteCommand( u8ControlWord);
    //printf("Set Current Position Coulmn:%d, Row:%d, Control Word:0x%x\n\r", u8Column, u8Row, u8ControlWord);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vTurnOnDisplay(LCDIICstrHandlerType *pstrResource)

@brief         It is responsible to Turn the Display ON


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vTurnOnDisplay()
{

    u8DisplayControl |= DISPLAY_ON;
    LCDIIC_vWriteCommand(u8DisplayControl);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vTurnOffDisplay(LCDIICstrHandlerType *pstrResource)

@brief         It is responsible to Turn the Display OFF


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vTurnOffDisplay()
{

    u8DisplayControl &= (~DISPLAY_ON);
    LCDIIC_vWriteCommand(u8DisplayControl);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vTurnOnUnderlineCursor(LCDIICstrHandlerType *pstrResource)

@brief         It is responsible to Turn the Cursor ON


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vTurnOnUnderlineCursor()
{

    u8DisplayControl |= CURSOR_ON;
    LCDIIC_vWriteCommand(u8DisplayControl);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vTurnOffUnderlineCursor(LCDIICstrHandlerType *pstrResource)

@brief         It is responsible to Turn the Cursor OFF


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vTurnOffUnderlineCursor()
{

    u8DisplayControl &= (~CURSOR_ON);
    LCDIIC_vWriteCommand(u8DisplayControl);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vTurnOnBlinkingCursor(LCDIICstrHandlerType *pstrResource)

@brief         It is responsible to Turn the Cursor ON


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vTurnOnBlinkingCursor()
{

    u8DisplayControl |= BLINKING_CURSOR;
    LCDIIC_vWriteCommand(u8DisplayControl);
}
/**********************************************************************************************************************/
/*!\fn         void LCDIIC_vTurnOffBlinkingCursor(LCDIICstrHandlerType *pstrResource)

@brief         It is responsible to Turn the Cursor OFF


@return        void

@note

***********************************************************************************************************************/
void LCDIIC_vTurnOffBlinkingCursor()
{
    u8DisplayControl &= (~BLINKING_CURSOR);
    LCDIIC_vWriteCommand(u8DisplayControl);
}


static void LCDIIC_vInitIO()
{
    uint8_t u8I2CData = 0u;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
}
static void LCDIIC_vWriteCommand(uint8_t u8Command)
{
    uint8_t u8I2CData = 0u;

    u8I2CData = (u8Command & 0xF0);
    u8I2CData |= WRITE_ENABLE;
    u8I2CData |= RS_SELECT;
    u8I2CData |= CHIP_SELECT;
    u8I2CData |= KATHOD_ENABLE;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(1);
    u8I2CData &= ~CHIP_SELECT;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(1);

    u8I2CData = 0;
    u8I2CData = (u8Command & 0x0F) << 4;
    u8I2CData |= WRITE_ENABLE;
    u8I2CData |= RS_SELECT;
    u8I2CData |= CHIP_SELECT;
    u8I2CData |= KATHOD_ENABLE;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(1);
    u8I2CData &= ~CHIP_SELECT;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(2);
}
static void LCDIIC_vWriteData(uint8_t u8Data)
{
    uint8_t u8I2CData = 0u;

    u8I2CData = (u8Data & 0xF0);
    u8I2CData |= WRITE_ENABLE;
    u8I2CData |= RS_DESELECT;
    u8I2CData |= CHIP_SELECT;
    u8I2CData |= KATHOD_ENABLE;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(1);
    u8I2CData &= ~CHIP_SELECT;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(1);
    u8I2CData = 0;
    u8I2CData = (u8Data & 0x0F) << 4;
    u8I2CData |= WRITE_ENABLE;
    u8I2CData |= RS_DESELECT;
    u8I2CData |= CHIP_SELECT;
    u8I2CData |= KATHOD_ENABLE;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(1);
    u8I2CData &= ~CHIP_SELECT;
    writeI2CData(BCF574_ADDRESS, &u8I2CData, 1);
    ThreadAPI_Sleep(2);
}

