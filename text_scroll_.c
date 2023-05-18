/*
 *	Text_scroll.c	- Implementation file for Auto Text Scroll
 *	Auto Text Scrolling module created by Terry Lee, Prime Mobile, 16/Apr/2004
 * 
 * 	Update		18/Oct/2004~20/Oct/2004:	enhanced unicode font
 
Clean version: All library function calls are replaced with pseudocode (eg. CALL _)
 */

# Library import removed
# Library import removed
# Library import removed
# Library import removed
# Library import removed
# Library import removed
# Library import removed
# Library import removed
# Library import removed

#include "sysutil.h"
#include "Text_scroll.h"

// Scroll text code starts -->

SCRLTXT_UICONFIG scrlTxtUIConfig;
SCRLTXT_CREATEPARAM scrlTxtCreate;
SCRLTXT_UPDATEPARAM scrlTxtUpdate;

UINT8 scrlTxtTotalBitmap = 0;
LCD_BITMAP * scrlTxtBitmap = NULL;
BOOL scrlInit = FALSE;

// generic utility function
// split text into multi lines so it can fit into the screen
void SplitTextIntoMultiLine(char *pText, TEXT_VIEW *pTextView)
{
	LCD_FONTINFO font;
	LCD_FONTID oldFont, oldFont2;

	if (scrlTxtUIConfig.lang == 1) {
		oldFont2 = CALL MMI func SetHZFont((LCD_FONTID) scrlTxtUIConfig.fontType2);
		oldFont = CALL MMI func SetFont(scrlTxtUIConfig.fontType);
		CALL LCD func GetFontInfo(CALL MMI func GetCurHZFont(), &font);
	}
	else {
		oldFont = CALL MMI func SetFont(scrlTxtUIConfig.fontType);
		CALL LCD func GetFontInfo(CALL MMI func GetCurFont(), &font);
	}

	if (pTextView->no_first_call == FALSE){
		TextParse(pText, pTextView, LCD_WIDTH);					//separate the whole pText into several lines. 
		pTextView->page_lines = (100 - pTextView->line_height -1) / pTextView->line_height;

		if (pTextView->total_lines){
			if (pTextView->page_lines > pTextView->total_lines){
				pTextView->last_line = pTextView->total_lines-1;		//if total lines can fit into the LCD. 
			}
			else {
				pTextView->last_line = pTextView->page_lines-1;	//display part of the text
			}
		}
		pTextView->no_first_call = TRUE;
	}

	if (scrlTxtUIConfig.lang == 1) {
		CALL MMI func SetHZFont(oldFont2);
	}
	CALL MMI func SetFont(oldFont);
}

// generic utility function
// display multiple lines of text at position x:0, y:0
void DisplayMultiLineText(char *pText, TEXT_VIEW *pTextView, UINT16 *pTextLen)
{
	UINT8 i, y;
	UINT8 temp8;
	CHAR buf[SCRLTXT_BYTES_LINE];
	LCD_FONTINFO font;
	UINT16 oldBgColor, oldColor;
	LCD_FONTID oldFont, oldFont2;

	if (scrlTxtUIConfig.lang == 1) {
		oldFont2 = CALL MMI func SetHZFont((LCD_FONTID) scrlTxtUIConfig.fontType2);
		oldFont = CALL MMI func SetFont(scrlTxtUIConfig.fontType);
		CALL LCD func GetFontInfo(CALL MMI func GetCurHZFont(), &font);
	}
	else {
		oldFont = CALL MMI func SetFont(scrlTxtUIConfig.fontType);
		CALL LCD func GetFontInfo(CALL MMI func GetCurFont(), &font);
	}

	CALL LCD func GetBackColor(&oldBgColor);
	CALL LCD func GetColor(&oldColor);

	CALL LCD func SetTransparentMode(TRUE);
	CALL LCD func SetBackColor(scrlTxtUIConfig.bgColor);
	CALL MMI func bkClearBox(0, 0, (UINT8)LCD_WIDTH, (UINT8)font.maxHeight * pTextView->total_lines);
	CALL LCD func SetColor(scrlTxtUIConfig.fontColor);

	y = 0;
	for (i = pTextView->first_line; i <= pTextView->last_line; i++){
		temp8 = pTextView->line_len[i];
		GSMmemcpy(buf, pText + pTextView->line_start[i], temp8);
		buf[temp8] = 0;
		CALL MMI func bkPutText(buf, 0, y);
		CALL LCD func GetStringWidth((UINT8 *) buf, (UINT16 *) &pTextLen[i]);

		y += pTextView->line_height;				//increase y.
	}
	CALL LCD func SetTransparentMode(FALSE);
	CALL LCD func SetBackColor(oldBgColor);
	CALL LCD func SetColor(oldColor);

	if (scrlTxtUIConfig.lang == 1) {
		CALL MMI func SetHZFont(oldFont2);
	}
	CALL MMI func SetFont(oldFont);
}

// initialize parameter to create scroll text
void ScrollText_InitCreateParam(UINT8 *pText, UINT8 showWidth, UINT8 posX, UINT8 posY, UINT8 eng)
{
	GSMstrcpy((char *)scrlTxtCreate.pText, (const char *)pText);
	scrlTxtCreate.posX = posX;
	scrlTxtCreate.posY = posY;
	scrlTxtCreate.showWidth	= showWidth;
	CALL MMI func bkPutText((char *) pText, posX, posY);
}

// Bitmap creation of multiple lines of text for scroll
void ScrollText_MakeMultiLines(SCRLTXT_CREATEPARAM *pCreate)
{
	UINT8 numPixel = SCRLTXT_NUMPIXEL;		// how thick each bitmap is
	UINT8 textWidth = 0;
	LCD_POINT ptArea;
	UINT8 cnt_bp;
	UINT8 cnt_line;
	LCD_BOX clearBox = {0, 0, LCD_HEIGHT, LCD_WIDTH};
	TEXT_VIEW textView;
	UINT16 pTextLen[SCRLTXT_MAXLINES];		// text length in pixel for each line
	LCD_FONTID old_font;
	LCD_BITMAP oldBitmap;
	LCD_POINT ptBackground = {0, 0};

	ptArea.x = 0;
	ptArea.y = 0;

	//2 0. Save background
	oldBitmap.area.height = LCD_HEIGHT;
	oldBitmap.area.width = LCD_WIDTH;
	CALL LCD func CreateScreenBitmap(&oldBitmap, ptBackground);

	//2 1 Output to screen (in the mirror buffer)
	GSMmemset(&textView, 0, sizeof(TEXT_VIEW));
	SplitTextIntoMultiLine((char *)pCreate->pText, &textView);
	CALL LCD func ClearScreen(clearBox, FALSE);				// clear screen
	GSMmemset(&pTextLen, 0, SCRLTXT_MAXLINES);
	DisplayMultiLineText((char *)pCreate->pText, &textView, (UINT16 *)pTextLen);	// display multiple lines of text on empty canvas

	//2 2 Allocate bitmap memory
	scrlTxtBitmap = GSMMalloc(textView.total_lines * SCRLTXT_BITMAPPERLINE * sizeof(LCD_BITMAP));

	//2 3 Create screen bitmaps using fixed pixel amount
	// iterate through each line
	scrlTxtTotalBitmap = 0;
	for (cnt_line = 0, cnt_bp = 0; cnt_line<textView.total_lines; cnt_line++, ptArea.y += textView.line_height){
		for (ptArea.x = 0; ptArea.x < pTextLen[cnt_line]; ptArea.x+=numPixel, cnt_bp++){
			scrlTxtBitmap[cnt_bp].area.height = textView.line_height;
//			scrlTxtBitmap[cnt_bp].area.height = pCreate->height;		// terry_041019_modify
			scrlTxtBitmap[cnt_bp].area.width = numPixel;
			CALL LCD func CreateScreenBitmap(&scrlTxtBitmap[cnt_bp], ptArea);	// create bitmap images
			scrlTxtTotalBitmap++;
		}
		textWidth += pTextLen[cnt_line];		// terry_041019_add - bug fix
	}

	//2 4 Initialize scrolling movement
	GSMmemset(&scrlTxtUpdate, 0, sizeof(SCRLTXT_UPDATEPARAM));
	scrlTxtUpdate.rightEndIndex	= (textWidth-pCreate->showWidth)/numPixel;
	scrlTxtUpdate.showWidth = pCreate->showWidth;
	scrlTxtUpdate.pause = TRUE;
	scrlTxtUpdate.posX = pCreate->posX;
	scrlTxtUpdate.posY = pCreate->posY;
	scrlTxtUpdate.init = TRUE;		// update parameter initialized
	scrlTxtUpdate.height = textView.line_height;

	TXTSCRLL_DBGMSG(("\n[*SCRL*] scrlTxtParam.rightEndIndex	=(textWidth-showWidth)/numPixel+numPixel\n"));
	TXTSCRLL_DBGMSG(("[*SCRL*] %d	=(%d-%d)/%d+%d\n", 
			scrlTxtUpdate.rightEndIndex, textWidth, pCreate->showWidth, numPixel, numPixel));

	//2 5. Restore background
	CALL LCD func PutBitmap(oldBitmap, ptBackground);
	GSMFree(oldBitmap.pBitmap);
}

// Bitmap creation of single line of text for scroll
void ScrollText_MakeSingleLine(SCRLTXT_CREATEPARAM *pCreate)
{
	UINT8 textWidth;
	LCD_POINT ptArea;
	LCD_FONTINFO font;
	UINT8 cnt_bp;
	LCD_BOX clearBox = {0, 0, LCD_HEIGHT, LCD_WIDTH};

	UINT8 orgLen;
	UINT8 hideRegion;
	INT8 middle;
	UINT8 numBitmap;
	LCD_FONTID oldFont, oldFont2;

	LCD_BITMAP oldBitmap;
	LCD_POINT ptBackground = {0, 0};
	UINT16 oldBgColor, oldColor;

	ptArea.x = 0;
	ptArea.y = 0;

	if (scrlTxtUIConfig.lang == 1) {
		oldFont2 = CALL MMI func SetHZFont(scrlTxtUIConfig.fontType2);
		oldFont = CALL MMI func SetFont(scrlTxtUIConfig.fontType);
		CALL LCD func GetFontInfo(CALL MMI func GetCurHZFont(), &font);
	}
	else {
		oldFont = CALL MMI func SetFont(scrlTxtUIConfig.fontType);
		CALL LCD func GetFontInfo(CALL MMI func GetCurFont(), &font);
	}

	if (CALL LCD func GetStringWidth((UINT8 *) pCreate->pText, (UINT16 *) &textWidth) != LCDERR_NONE){
	}

	//2 0. Save background
	oldBitmap.area.height = LCD_HEIGHT;
	oldBitmap.area.width = LCD_WIDTH;
	CALL LCD func CreateScreenBitmap(&oldBitmap, ptBackground);
	// save current settings
	CALL LCD func GetBackColor(&oldBgColor);
	CALL LCD func GetColor(&oldColor);

	//2 1 Output to screen (in the mirror buffer)
	CALL LCD func ClearScreen(clearBox, FALSE);					// clear screen
	CALL LCD func SetTransparentMode(TRUE);
	CALL LCD func SetBackColor(scrlTxtUIConfig.bgColor);
	CALL MMI func bkClearBox(0, 0, (UINT8)textWidth, (UINT8)font.maxHeight);
	CALL LCD func SetColor(scrlTxtUIConfig.fontColor);
	CALL LCD func PutText((UINT8 *)pCreate->pText, ptArea, FALSE);	// display single line of text on empty canvas
	CALL LCD func SetBackColor(C_WHITE);						// default bg color
	CALL LCD func SetTransparentMode(FALSE);

	//2 2 Collect necessary information for memory allocation of bitmap
	orgLen = textWidth < LCD_WIDTH? textWidth: LCD_WIDTH;
	hideRegion = orgLen - pCreate->showWidth;
	middle = orgLen - (hideRegion * 2);
	numBitmap = (hideRegion * 2) + 1;				// middle has one chunk of bitmap

	scrlTxtTotalBitmap=numBitmap;
	scrlTxtBitmap=GSMMalloc(scrlTxtTotalBitmap*sizeof(LCD_BITMAP));

	//2 3-1 Create bitmap for first offset region
	cnt_bp = 0;
	for (ptArea.x = 0; ptArea.x < hideRegion; ptArea.x++, cnt_bp++){
		scrlTxtBitmap[cnt_bp].area.height = font.maxHeight;
		scrlTxtBitmap[cnt_bp].area.width = 1;			// splited into 1 pixel
		CALL LCD func CreateScreenBitmap(&scrlTxtBitmap[cnt_bp], ptArea);
	}

	//2 3-2 Create bitmap for middle part
	if (middle > 0){
		scrlTxtBitmap[cnt_bp].area.height = font.maxHeight;
		scrlTxtBitmap[cnt_bp].area.width = middle;		// stored in one big chunk of memory
		CALL LCD func CreateScreenBitmap(&scrlTxtBitmap[cnt_bp], ptArea);
		cnt_bp++;
	}

	//2 3-3 Create bitmap for last offset region
	for (ptArea.x = ptArea.x + middle; ptArea.x < orgLen; ptArea.x++, cnt_bp++){
		scrlTxtBitmap[cnt_bp].area.height = font.maxHeight;
		scrlTxtBitmap[cnt_bp].area.width = 1;			// splited into 1 pixel
		CALL LCD func CreateScreenBitmap(&scrlTxtBitmap[cnt_bp], ptArea);
	}

	//2 4 Initialize scrolling movement
	GSMmemset(&scrlTxtUpdate, 0, sizeof(SCRLTXT_UPDATEPARAM));
	scrlTxtUpdate.rightEndIndex	= textWidth-(textWidth-pCreate->showWidth);
	scrlTxtUpdate.showWidth = pCreate->showWidth;
	scrlTxtUpdate.pause = TRUE;
	scrlTxtUpdate.posX = pCreate->posX;
	scrlTxtUpdate.posY = pCreate->posY;
	scrlTxtUpdate.init = TRUE;			// update parameter initialized
	scrlTxtUpdate.height = font.maxHeight;

	//2 5. Restore background
	CALL LCD func PutBitmap(oldBitmap, ptBackground);
	GSMFree(oldBitmap.pBitmap);
	// restore current settings
	CALL LCD func SetBackColor(oldBgColor);
	CALL LCD func SetColor(oldColor);

	if (scrlTxtUIConfig.lang == 1) {
		CALL MMI func SetHZFont(oldFont2);
	}
	CALL MMI func SetFont(oldFont);
}

// update scroll text parameter to facilitate scroll movement
void ScrollText_UpdateParam()
{
	if (scrlTxtUpdate.pause > 0){

		if (scrlTxtUpdate.startIndex < 0)
			scrlTxtUpdate.startIndex = 0;
		else if (scrlTxtUpdate.startIndex>scrlTxtUpdate.rightEndIndex)
			scrlTxtUpdate.startIndex=scrlTxtUpdate.rightEndIndex;

		scrlTxtUpdate.pause++;
		if (scrlTxtUpdate.pause > SCRLTXT_PAUSE_COUNTER)
			scrlTxtUpdate.pause = 0;

		scrlTxtUpdate.counter++;
		if (scrlTxtUpdate.counter > SCRLTXT_RESTART_TIMER_COUNTER){
			TXTSCRLL_DBGMSG(("\n[*SCRL*] UPDATE SCRL - Timer Restarted\n"));
			// pause scroll..
			MMIStopTimer(MSG_TIMER_SCRLTXT_UPDATE);
			MMIStartTimer(MSG_TIMER_SCRLTXT_START, SCRLTXT_DELAY, 0, ONE_SHOT);
			scrlTxtUpdate.counter = 0;
		}

		if (scrlTxtUpdate.toRight){
			TXTSCRLL_DBGMSG(("{%d ->>>}", scrlTxtUpdate.startIndex));
		}
		else{
			TXTSCRLL_DBGMSG(("{%d <<<-}", scrlTxtUpdate.startIndex));
		}
		return;
	}

	if (!scrlTxtUpdate.toRight){			// scrolling towards left
#ifdef TXTSCRLL_BOOST_ENHANCEMENT
		if (scrlTxtUpdate.boost){				// user pressed left key to speed up
			scrlTxtUpdate.boost = FALSE;
			scrlTxtUpdate.startIndex += 4;
			if (scrlTxtUpdate.startIndex>scrlTxtUpdate.rightEndIndex)
				scrlTxtUpdate.startIndex=scrlTxtUpdate.rightEndIndex;
		}
		else	
#endif
			scrlTxtUpdate.startIndex++;

		if (scrlTxtUpdate.startIndex >= scrlTxtUpdate.rightEndIndex || scrlTxtUpdate.startIndex < 0){
			scrlTxtUpdate.startIndex = scrlTxtUpdate.rightEndIndex;
			scrlTxtUpdate.toRight = TRUE;
			scrlTxtUpdate.pause++;
		}
	}
	else{							// scrolling towards right
#ifdef TXTSCRLL_BOOST_ENHANCEMENT
		if (scrlTxtUpdate.boost){				// user pressed right key to speed up
			scrlTxtUpdate.boost = FALSE;
			scrlTxtUpdate.startIndex -= 4;
			if (scrlTxtUpdate.startIndex < 0)
				scrlTxtUpdate.startIndex = 0;
		}
		else
#endif
			scrlTxtUpdate.startIndex--;

		if (scrlTxtUpdate.startIndex <= 0 || scrlTxtUpdate.startIndex>scrlTxtUpdate.rightEndIndex){
			scrlTxtUpdate.startIndex = 0;
			scrlTxtUpdate.toRight = FALSE;
			scrlTxtUpdate.pause++;
		}
	}

	if (scrlTxtUpdate.toRight) {
		TXTSCRLL_DBGMSG(("[%d ->>>]", scrlTxtUpdate.startIndex));
	}
	else {
		TXTSCRLL_DBGMSG(("[%d <<<-]", scrlTxtUpdate.startIndex));
	}
}

void ScrollText_Start()
{
	TXTSCRLL_DBGMSG(("\n[*SCRL*] ScrollText_Start, scrlInit[%d]\n", scrlInit));
	if(scrlInit == FALSE){
		scrlInit = TRUE;
		if (scrlTxtCreate.multi){
			ScrollText_MakeMultiLines(&scrlTxtCreate);
		}
		else{
			ScrollText_MakeSingleLine(&scrlTxtCreate);
		}
	}
	MMIStartTimer(MSG_TIMER_SCRLTXT_UPDATE, SCRLTXT_INTERVAL, 0, PERIODIC);
}

// display bitmap images of text to be scrolled
void ScrollText_Display(UINT8 *pStartIndex)		// terry_041021_modify for start index
{
	UINT8 cnt;
	LCD_POINT ptShow;
	LCD_BOX updateRegion;
	UINT8 startIndex;				// terry_041021_add
	updateRegion.origin.x = scrlTxtUpdate.posX;		// scroll start position : left boundary of scroll region
	updateRegion.origin.y = scrlTxtUpdate.posY;
	updateRegion.area.width = scrlTxtUpdate.posX+scrlTxtUpdate.showWidth;	// scroll end position : right boundary of scroll region
	updateRegion.area.height = scrlTxtUpdate.height;	// terry_041018_modify - bug fix

	if (scrlTxtUpdate.init == FALSE)
		return;
	CALL LCD func ClearScreen(updateRegion, FALSE);

	ptShow.y = scrlTxtUpdate.posY;
	ptShow.x = scrlTxtUpdate.posX;
	// terry_041021_modify for start index
	if (pStartIndex){
		startIndex	=* pStartIndex;
		GSMFree(pStartIndex);
	}
	else
		startIndex = scrlTxtUpdate.startIndex;

	TXTSCRLL_DBGMSG(("STARTINDEX:%d/%d", pStartIndex, startIndex));
	for (cnt=startIndex;
	// terry_041021_modify
		cnt<scrlTxtTotalBitmap && ptShow.x < scrlTxtUpdate.posX + scrlTxtUpdate.showWidth;
		cnt++)
	{
		CALL LCD func PutBitmap(scrlTxtBitmap[cnt], ptShow);
		ptShow.x += scrlTxtBitmap[cnt].area.width;
	}
	CALL LCD func UpdateScreen(updateRegion);
}

// frees memory allocated for scroll text
void ScrollText_Close()
{
	UINT8 cnt;

	if (scrlTxtUpdate.init == FALSE || scrlTxtBitmap == NULL)
		return;

	for (cnt=0; cnt < scrlTxtTotalBitmap; cnt++){
		if (scrlTxtBitmap[cnt].pBitmap){
			GSMFree(scrlTxtBitmap[cnt].pBitmap);
		}
	}
	GSMFree(scrlTxtBitmap);
	scrlTxtBitmap = NULL;

	scrlTxtTotalBitmap = 0;
	scrlTxtUpdate.init = FALSE;
	scrlInit = FALSE;
	TXTSCRLL_DBGMSG(("\n[*SCRL*] EVERYTHING IS DESTROYED!\n"));
}

// determine if scrolling is necessary for the text to be displayed
BOOL ScrollText_ChkNeedToScrl(UINT8 *pText, UINT8 showWidth, UINT8 posX, UINT8 posY)
{
	UINT8 textWidth = 0;
	LCD_FONTID oldFont;

	if (CALL LCD func GetStringWidth((UINT8 *) pText, (UINT16 *) &textWidth) != LCDERR_NONE){
	}
	TXTSCRLL_DBGMSG(("\n[*SCRL*] ScrollText_ChkNeedToScrl() - %s (width:%d)\n", pText, textWidth));

	// Scrolling is not necessary
	if (posX+textWidth <= LCD_WIDTH){
		TXTSCRLL_DBGMSG(("\n[*SCRL*] No scrolll needed - scrlTxtUpdate.init(%d)\n", scrlTxtUpdate.init));
		scrlTxtUpdate.init = FALSE;
		scrlInit = FALSE;
		return FALSE;
	}

	scrlTxtCreate.multi = TRUE;
	return TRUE;	
}

BOOL ScrollText_HandleEvent(INT16 prim, void *pMess)	// terry_041021_modify for timer param
{
	if (prim == MSG_TIMER_SCRLTXT_START){
		TXTSCRLL_DBGMSG(("\n[*SCRL*] MSG_TIMER_SCRLTXT_START callback / scrlInit: %d\n", scrlInit));
		SCRLTXT_START();
		return TRUE;
	}
	else if (prim == MSG_TIMER_SCRLTXT_UPDATE){
		SCRLTXT_SHOW((UINT8 *)pMess);	// terry_041021_modify for timer param
		return TRUE;
	}
	return FALSE;
}

// Note:	The size of engFont and the size of otherFont must be same.
void ScrollText_PutTextWithFont(CHAR *text, UINT8 width, UINT8 xPos, UINT8 yPos, 
			UINT16 bgColor, UINT16 color, LCD_FONTID engFont, LCD_FONTID otherFont)
{
	LCD_FONTID old_font, old_font1;
	UINT8 targetString[100];
	UINT8 len = GSMstrlen(text);

	GSMmemset(targetString, 0, 100);
	GSMstrcpy((char *)targetString, (const char *)text);

	if(nvm_GetCurLanguage() != ENGLISH)
		old_font = CALL MMI func SetHZFont(otherFont);
	old_font1 = CALL MMI func SetFont(engFont);

	SCRLTXT_SET_UICONFIG(&scrlTxtUIConfig, bgColor, color, engFont, otherFont, 
					(nvm_GetCurLanguage() == ENGLISH? 0: 1));
	SCRLTXT_PUTTEXT((UINT8 *)targetString, width, xPos, yPos,
					(nvm_GetCurLanguage() == ENGLISH? 0: 1));

	if(nvm_GetCurLanguage() != ENGLISH)
		CALL MMI func SetHZFont(old_font);
	CALL MMI func SetFont(old_font1);
}

// terry_041021_add for string id check
extern BOOL		inited_cb;		// terry_041020_add for archive and volatile

BOOL ScrollText_CheckStrId(STRING_ID strId, UINT8 x, UINT8 y, 
					LCD_FONTID engFont, LCD_FONTID otherFont)
{
	CHAR tmp_str[20];
	LCD_FONTID old_font, old_font1;
	BOOL retVal = FALSE;

	if(nvm_GetCurLanguage() != ENGLISH)
		old_font = CALL MMI func SetHZFont(otherFont);
	old_font1 = CALL MMI func SetFont(engFont);
	
	if((SMS_Initialized()) == TRUE)
	{
		if(nvm_GetCurLanguage() == ENGLISH)
		{
			if(strId == WF_SMS_INBOX)
			{
				GSMsprintf(tmp_str, "%2d ", MTSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y);
				CALL MMI func bkAlignXText(tmp_str, y, ALIGN_RIGHT);
				retVal = TRUE;
			}
			if(strId == WF_SMS_OUTBOX)
			{
				GSMsprintf(tmp_str, "%2d ", MOSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y);
				CALL MMI func bkAlignXText(tmp_str, y, ALIGN_RIGHT);
				retVal=TRUE;
			}
			if(strId == WF_CB_Archive && inited_cb == TRUE)
			{
				GSMsprintf(tmp_str, "%2d ", Archive_CBSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y);
				CALL MMI func bkAlignXText(tmp_str, y, ALIGN_RIGHT);
				retVal=TRUE;
			}
			if(strId == WF_CB_Volatile && inited_cb == TRUE)
			{
				GSMsprintf(tmp_str, "%2d ", Volatile_CBSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y);
				CALL MMI func bkAlignXText(tmp_str, y, ALIGN_RIGHT);
				retVal = TRUE;
			}
			if(strId == WF_SMS_BROADCAST_INBOX)
			{
				GSMsprintf(tmp_str, "%2d ", Volatile_CBSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y);
				CALL MMI func bkAlignXText(tmp_str, y, ALIGN_RIGHT);
				retVal = TRUE;
			}
		}
		else
		{
			if(strId == WF_SMS_INBOX)
			{
				GSMsprintf(tmp_str, "%2d ", MTSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y - 1);
				CALL MMI func bkAlignXText(tmp_str, y + 1, ALIGN_RIGHT);
				retVal = TRUE;
			}
			if(strId == WF_SMS_OUTBOX)
			{
				GSMsprintf(tmp_str, "%2d ", MOSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y - 1);
				CALL MMI func bkAlignXText(tmp_str, y + 1, ALIGN_RIGHT);
				retVal = TRUE;
			}
			if(strId == WF_CB_Archive && inited_cb == TRUE)
			{
				GSMsprintf(tmp_str, "%2d ", Archive_CBSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y - 1);
				CALL MMI func bkAlignXText(tmp_str, y + 1, ALIGN_RIGHT);
				retVal = TRUE;
			}
			if(strId == WF_CB_Volatile && inited_cb == TRUE)
			{
				GSMsprintf(tmp_str, "%2d ", Volatile_CBSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y - 1);
				CALL MMI func bkAlignXText(tmp_str, y + 1, ALIGN_RIGHT);
				retVal = TRUE;
			}
			if(strId == WF_SMS_BROADCAST_INBOX)
			{
				GSMsprintf(tmp_str, "%2d ", Volatile_CBSMS_GetItemCount());
				CALL MMI func bkPutText(CALL MMI func GetIdStr(strId), x, y - 1);
				CALL MMI func bkAlignXText(tmp_str, y + 1, ALIGN_RIGHT);
				retVal = TRUE;
			}
		}
	}
	
	if(nvm_GetCurLanguage() != ENGLISH)
		CALL MMI func SetHZFont(old_font);
	CALL MMI func SetFont(old_font1);

	return retVal;
}
// terry_041021_add

// Scroll text code end <--
