#include "PixelScript.h"
#include "EventScript.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IS_COMMAND1(c1) (ptx->data[ptx->p_read] == '<' && ptx->data[ptx->p_read + 1] == (c1))
#define IS_COMMAND2(c1, c2) (ptx->data[ptx->p_read] == '<' && ptx->data[ptx->p_read + 1] == (c1) && ptx->data[ptx->p_read + 2] == (c2))

RECT rcPsIllust = { 0, 0, 320, 240 };
RECT rcPsEnd = { 0, 0, 48, 24 };
RECT rcPsLine = { 0, 0, SURFACE_WIDTH, 16 };

SDL_bool ReadPixelScript(PIX_SCR *ptx, char* path)
{
	//Open file
	FILE *fp = fopen(path, "rb");
	if (fp == NULL)
		return SDL_FALSE;
	
	//Get filesize
	fseek(fp, 0, SEEK_END);
	ptx->size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	//Allocate data (zeroed, so always NUL-terminated)
	ptx->data = (char*)calloc(ptx->size + 1, 1);
	if (ptx->data == NULL)
	{
		fclose(fp);
		return SDL_FALSE;
	}
	
	//Read file
	fread(ptx->data, ptx->size, 1, fp);
	fclose(fp);
	return SDL_TRUE;
}

void LoadPixelScript(PIX_SCR *ptx, char* path, char scale)
{
	//Initialize Pixel Script
	ptx->end = SDL_FALSE;
	ptx->p_read = 0;
	ptx->scale = scale;
	ptx->line = 0;
	for (int i = 0; i < MAX_PSLINES; i++)
	{
		ptx->ypos_line[i] = i * 24 * ptx->scale;
		CortBox2(&rcPsLine, 0x000000, SURFACE_ID_WORDS0 + i);
	}

	//Read script
	ReadPixelScript(ptx, path);
	DebugPutText(ptx->data);
}

int PixelScriptProc(PIX_SCR *ptx, PIYOPIYO_CONTROL *piyocont, SDL_bool ending)
{
	char c[44];
	
	//Draw illustration
	if (ending)
		PutBitmap3(&grcFull, (SURFACE_WIDTH - 320) / 2, (SURFACE_HEIGHT - 240) / 2, &rcPsIllust, SURFACE_ID_STAFF);
	else
		PutBitmap3(&grcFull, (SURFACE_WIDTH - 320) / 2, (SURFACE_HEIGHT - 240) / 2, &rcPsIllust, SURFACE_ID_MARUAME);
	
	if (ptx->end)
	{
		//If the game's ended, draw "END" text
		PutBitmap3(&grcFull, (SURFACE_WIDTH / 2) - 24, (SURFACE_HEIGHT / 2) - 14, &rcPsEnd, SURFACE_ID_END);
		return 1;
	}
	else
	{
		//Process script and lines
		for (int i = 0; i < MAX_PSLINES; i++)
		{
			//Draw line
			PutBitmap3(&grcFull, 0, ptx->ypos_line[i] / ptx->scale, &rcPsLine, SURFACE_ID_WORDS0 + i);

			//Scroll line
			ptx->ypos_line[i] -= 2;
			if ((gKey & KEY_SPACE) || (gKey & KEY_Z))
				ptx->ypos_line[i] -= 6;

			//Check if line's scrolled over the top of the screen
			if (ptx->ypos_line[i] / ptx->scale < -24)
			{
				//Clear line and move to bottom of screen
				ptx->ypos_line[i] = SURFACE_HEIGHT * ptx->scale;
				CortBox2(&rcPsLine, 0x000000, SURFACE_ID_WORDS0 + i);

				//Read new line
				memset(c, 0, sizeof(c));
				for (int j = 0; j < 43; j++)
				{
					char ch = ptx->data[ptx->p_read];
					if (ch == ' ' || ch == '-')
					{
						//Type space
						c[j] = ch;
						ptx->p_read++;
					}
					else if (ch & 0x80)
					{
						//Type Shift-JIS
						if (j + 1 >= 43)
							break;
						c[j] = ch;
						(c + 1)[j++] = ptx->data[ptx->p_read + 1];
						ptx->p_read += 2;
					}
					else if (IS_COMMAND2('p','f'))
					{
						//Fade PiyoPiyo
						ptx->p_read += 4;
						piyocont->mode = 4;
						j = 999;
					}
					else if (IS_COMMAND1('s'))
					{
						//Bring up "END" text
						ptx->end = SDL_TRUE;
						j--; //Don't leave a hole in the line buffer
						ptx->p_read += 2; //Skip the "<s" command itself
					}
					else if ((ch >= 'A' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch != 0 && strchr(".,/!?':()&>\"", ch)))
					{
						//English alphabet, numerals and printable punctuation
						c[j] = ptx->data[ptx->p_read++];
					}
					else if (IS_COMMAND1('e'))
					{
						//Start gameplay
						return 1;
					}
					else if (ch == '+')
					{
						//Print text onto line
						c[j] = 0;
						PutText2(8, 1, c, 0xFF0000, SURFACE_ID_WORDS0 + ptx->line, SDL_FALSE);
						PutText2(8, 0, c, 0xFF8800, SURFACE_ID_WORDS0 + ptx->line, SDL_FALSE);
						if (++ptx->line > (MAX_PSLINES - 1))
							ptx->line = 0;
						ptx->p_read += 2;
						j = 999;
					}
					else if (ch == '<')
					{
						//Literal '<' (e.g. "<<<Ikachan>>>")
						c[j] = ch;
						ptx->p_read++;
					}
					else
					{
						//Skip unknown character without leaving a hole in the buffer
						ptx->p_read++;
						j--;
					}
				}
			}
		}

		//If the game's ended, draw "END" text
		if (ptx->end)
			PutBitmap3(&grcFull, (SURFACE_WIDTH / 2) - 24, (SURFACE_HEIGHT / 2) - 14, &rcPsEnd, SURFACE_ID_END);
		return 0;
	}
}

void EndPixelScript(PIX_SCR *ptx)
{
	//Release data
	free(ptx->data);
}