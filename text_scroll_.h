/*
 *	Text_scroll.h	- Header file for Auto Text Scroll
 *	Auto Text Scrolling module created by Terry Lee, Prime Mobile, 16/Apr/2004
 */

#ifndef _TEXT_SCROLL_H_
#define _TEXT_SCROLL_H_

#define SCRLTXT_INTERVAL		90						// timer interval for every scrolling update
#define SCRLTXT_DELAY			100						// delay before start scrolling
#define SCRLTXT_NUMPIXEL		2						// number of pixels to be scrolled in one timer update
#define SCRLTXT_BITMAPPERLINE	LCD_WIDTH/SCRLTXT_NUMPIXEL	// used to scroll multiple lines of text
														// make sure this division yields reminder of zero
#define SCRLTXT_MAXLINES					100
#define SCRLTXT_SMALLEST_FONTSIZE			8
#define SCRLTXT_BYTES_LINE					(LCD_WIDTH/SCRLTXT_SMALLEST_FONTSIZE)*2
												// buffer bytes in a line
#define SCRLTXT_PAUSE_COUNTER				8
#define SCRLTXT_RESTART_TIMER_COUNTER	3
#define SCRLTXT_MAXWORDS					40

/*!
 * Debug messages output
 */
#define _DEBUG_SCRLTXT_		// Uncomment this, when debug..
#ifndef _GSMPRINTF_OFF_
	#ifdef _DEBUG_SCRLTXT_
	#define TXTSCRLL_DBGMSG(a)			GSMprintf a
	#else
	#define TXTSCRLL_DBGMSG(a)
	#endif
#else
#define TXTSCRLL_DBGMSG(a)
#endif

/*!
 * Enhancement - currently not supported due to small-sized handset keypad
 */
//#define TXTSCRLL_BOOST_ENHANCEMENT		// Uncomment this, when support

typedef struct tag_SCRLTXT_UICONFIG{
	UINT16						bgColor;
	UINT16						fontColor;
	LCD_FONTID					fontType;
	LCD_FONTID					fontType2;		// Fot alternative language: eg.Chinese font
	UINT8						lang;			// terry_041016_add for more language support
}SCRLTXT_UICONFIG;

typedef struct tag_SCRLTXT_CREATEPARAM{
	UINT8						pText[SCRLTXT_MAXWORDS];
	UINT8						showWidth;
	UINT8						posX;
	UINT8						posY;
	BOOL						multi;
}SCRLTXT_CREATEPARAM;

typedef struct tag_SCRLTXT_UPDATEPARAM{
	UINT8						posX;
	UINT8						posY;

	INT16						startIndex;		// start index of bitmap buffer for display
	INT16						rightEndIndex;	// end index on the right for display
	BOOL						toRight;			// scrolling direction : left or right
	UINT8						pause;			// scrolling pause when direction changes
	UINT8						showWidth;		// width of scrolling region (in pixel)
	UINT8						height;
	UINT8						counter;			// disables scrolling for certain period

	UINT8						unitPixel;		// how many pixels to be scrolled in single timer update
#ifdef TXTSCRLL_BOOST_ENHANCEMENT
	BOOL						boost;			// for those who are impatient and furious.. :)
#endif
	BOOL						init;
} SCRLTXT_UPDATEPARAM;

extern SCRLTXT_UICONFIG				scrlTxtUIConfig;
extern SCRLTXT_CREATEPARAM			scrlTxtCreate;
extern SCRLTXT_UPDATEPARAM			scrlTxtUpdate;

extern UINT8							scrlTxtTotalBitmap;
extern LCD_BITMAP *					scrlTxtBitmap;
extern BOOL							scrlInit;

void		SplitTextIntoMultiLine(char *pText, TEXT_VIEW *pTextView);
void		DisplayMultiLineText(char *pText, TEXT_VIEW *pTextView, UINT16 *pTextLen);

void		ScrollText_InitCreateParam(UINT8 *pText, UINT8 showWidth, UINT8 posX, UINT8 posY, UINT8 eng);
void		ScrollText_MakeMultiLines(SCRLTXT_CREATEPARAM *pCreate);
void		ScrollText_MakeSingleLine(SCRLTXT_CREATEPARAM *pCreate);

void		ScrollText_UpdateParam(void);
void		ScrollText_Start(void);
void		ScrollText_Display(UINT8 *pStartIndex);			// terry_041021_modify
void		ScrollText_Close(void);
BOOL	ScrollText_ChkNeedToScrl(UINT8 *pText, UINT8 showWidth, UINT8 posX, UINT8 posY);
BOOL	ScrollText_HandleEvent(INT16 prim, void *pMess);	// terry_041021_modify
void		ScrollText_PutTextWithFont(CHAR *text, UINT8 width, UINT8 xPos, UINT8 yPos, 
					UINT16 bgColor, UINT16 color, LCD_FONTID engFont, LCD_FONTID otherFont);
// terry_041021_add for check string id
BOOL	ScrollText_CheckStrId(STRING_ID strId, UINT8 x, UINT8 y, 
					LCD_FONTID engFont, LCD_FONTID otherFont);
// terry_041021_add

#define SCRLTXT_INIT(PBITMAP, PTOTAL)\
									(*PBITMAP)=NULL;(*PTOTAL)=0;
#define SCRLTXT_PUTTEXT(TEXT, SHOW_WIDTH, X, Y, LANG)\
									TXTSCRLL_DBGMSG(("\n[*SCRL*] SCRLTXT_PUTTEXT() called\n"));\
									MMIStopTimer(MSG_TIMER_SCRLTXT_START);\
									MMIStopTimer(MSG_TIMER_SCRLTXT_UPDATE);\
									ScrollText_Close();\
									ScrollText_InitCreateParam(TEXT, SHOW_WIDTH, X, Y, LANG);\
									if(ScrollText_ChkNeedToScrl(TEXT, SHOW_WIDTH, X, Y))\
									{MMIStartTimer(MSG_TIMER_SCRLTXT_START, SCRLTXT_DELAY, 0, ONE_SHOT);\
									TXTSCRLL_DBGMSG(("\n[*SCRL*] MSG_TIMER_SCRLTXT_START timer started...\n"));}
#define SCRLTXT_START()				\
									ScrollText_Start();
// terry_041021_modify for start index
#define SCRLTXT_SHOW(START)				\
							if (scrlTxtUpdate.init){ScrollText_UpdateParam();\
							ScrollText_Display(START);}
// terry_041021_modify
#define SCRLTXT_CLOSE()				\
									MMIStopTimer(MSG_TIMER_SCRLTXT_START);	\
									MMIStopTimer(MSG_TIMER_SCRLTXT_UPDATE);\
									ScrollText_Close();
#define SCRLTXT_HANDLE_LKEY(SCRL_PARAM)\
									if ((*SCRL_PARAM).startIndex-4<=0)\
									{(*SCRL_PARAM).startIndex=0;break;}\
									(*SCRL_PARAM).pause=0;\
									if (!(*SCRL_PARAM).toRight) {(*SCRL_PARAM).boost=TRUE;}\
									(*SCRL_PARAM).toRight=FALSE;
#define SCRLTXT_HANDLE_RKEY(SCRL_PARAM)\
									if ((*SCRL_PARAM).startIndex+4>=(*SCRL_PARAM).rightEndIndex)\
									{(*SCRL_PARAM).startIndex=(*SCRL_PARAM).rightEndIndex;break;}\
									(*SCRL_PARAM).pause=0;\
									if ((*SCRL_PARAM).toRight){(*SCRL_PARAM).boost=TRUE;}\
									(*SCRL_PARAM).toRight=TRUE;
#define SCRLTXT_SET_UICONFIG(UICONFIG, BGCOLOR, COLOR, FONTID, FONTID2, LANG)\
									(*UICONFIG).bgColor=BGCOLOR;\
									(*UICONFIG).fontColor=COLOR;\
									(*UICONFIG).fontType=FONTID;\
									(*UICONFIG).fontType2=FONTID2;\
									(*UICONFIG).lang=LANG;
// terry_041021_add for pause scrolling during alarm
#define SCRLTXT_PAUSE	\
							{if (scrlInit==TRUE)\
							MMIStopTimer(MSG_TIMER_SCRLTXT_UPDATE);\
							TXTSCRLL_DBGMSG(("\n[*SCRL*] ***** Update Paused (%d)\n", scrlInit));}
#define SCRLTXT_RESUME\
							{if (scrlInit==TRUE) {\
							UINT8 *pStart=GSM_NEW(UINT8);\
							*pStart=scrlTxtUpdate.startIndex;\
							MMIStartTimer(MSG_TIMER_SCRLTXT_UPDATE, 90, (UINT32)pStart, PERIODIC);}\
							TXTSCRLL_DBGMSG(("\n[*SCRL*] ***** Update Resumed (%d)\n", scrlInit));}
// terry_041021_add

#endif
