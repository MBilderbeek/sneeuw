/* $Id: snow.c,v 1.1.1.1 2003/12/30 15:23:03 eric Exp $
 **********************************************************************
 * (C) 2003 Copyright Aurora - M. Bilderbeek & E. Boon
 *
 * DESCRIPTION :
 *   Main program of the incredible snowTV demo
 *   
 **********************************************************************/

/**********************************************************************
 * INCLUDES                                                           *
 **********************************************************************/

#include "stdlib.h"
#include "msxbios.h"
#include "glib.h"

/**********************************************************************
 * EXTERNAL REFERENCES (Eventually, these should go into .h files!)   *
 **********************************************************************/

int gs2loadgrp(uchar scrnmode, uchar page, char *filename);
int kbhit();

void DBG_mode(uchar);
void DBG_out(uchar);

/**********************************************************************
 * DEFINITIONS                                                        *
 **********************************************************************/

//#define DEBUG  1

#define SYSVAR(type,x) (*((type *)(x)))

#define CLICKSW SYSVAR(uchar,0xF3DB)
#define JIFFY   SYSVAR(uint, 0xFC9E)

#define NOISE_FILE "snowbg2.sr5"
#define GRAFX_FILE "tvs4.sr5"

#define NOISE_PG 2
#define GRAFX_PG 3

#define FONT_Y    0
#define FONT_CPL 16 // Nof characters on a line
#define FONT_W   16
#define FONT_H   12

#define MAXNOFTVS 16
#define TV_H      32
#define TV_W      32
#define TV_Y      80

/**********************************************************************/

typedef struct
{
	unsigned char x;
	unsigned char y; // in VRAM
} tvobj;

/**********************************************************************
 * STATIC VARS                                                        *
 **********************************************************************/

static char scrolltext[] = 
	"Dit is de verschrikkelijke sneeuwmanscroll! Hoezee! "
	"Of moet het nog langer!? ";

static uint palette[] = 
{	/*-GRB       */
	0x0000, /* 0 */
	0x0000, /* 1 */
	0x0222, /* 2 */
	0x0444, /* 3 */
	0x0444, /* 4 */
	0x0555, /* 5 */
	0x0666, /* 6 */
	0x0340, /* 7 */
	0x0171, /* 8 */
	0x0373, /* 9 */
	0x0434, /*10 */
	0x0000, /*11 */
	0x0070, /*12 */
	0x0700, /*13 */
	0x0007, /*14 */
	0x0777  /*15 */
};

static tvobj tvarray [MAXNOFTVS];
static uchar vdp23,
             textidx;

/**********************************************************************
 * AUXILIARY ROUTINES                                                 *
 **********************************************************************/

/**********************************************************************
 * init () - initalisation routine
 */
static void init(void)
{
	uchar i;
	char  filename[13];

#ifdef DEBUG
	DBG_mode((uchar)0x1F); /* single byte, decimal) */
#endif
	
	/* Initialize graphics */
	ginit();

	color(15, 0, 0);
	screen(5);
	wrtvdp(8, 10);
	disscr();
	
	for(i = 0; i < MAXNOFTVS; i++) {
		tvarray[i].x = 0;
		tvarray[i].y = 0;
	}
	
	for(i = 0; i < 16; i++)
		setpal(i, palette[i]);

	/* Load the real stuff... */
	strcpy(filename, NOISE_FILE); 
	gs2loadgrp(5, NOISE_PG, filename);
	strcpy(filename, GRAFX_FILE); 
	gs2loadgrp(5, GRAFX_PG, filename);
	
	/* hack to 'grey' the normally invisible area of the screen */
	cpyv2v(0, 100, 255, 256-212+100, NOISE_PG, 0, 212, NOISE_PG, PSET);
	/* debug*/
//	setpg(0,NOISE_PG);
//	boxfill(0,212,255,255,8,PSET);
//	setpg(0,0);
	/* end hack */
	
	/* Fill display pages with noise */
	cpyv2v(0, 0, 255, 255, NOISE_PG, 0, 0, 0, PSET);
	cpyv2v(0, 0, 255, 255, NOISE_PG, 0, 0, 1, PSET);
	
	/* Initialize 'sound' system */
	gicini();

	/* Snow sound ;-) */
	sound(6, 13);   /* noise frequency           */
	sound(7, 0xB7); /* 10.110.111 channel select */
	/*sound(8, 10);*/   /* volume on channel 1       */

	/* hit it! */
	vdp23   = 0;
	textidx = 0;
	srand(JIFFY);
	enascr();
}

/**********************************************************************
 * loop_colors() - Color animations for noise in background and on TVs
 */
static void loop_colors(void)
{
	static int i = 0;

	/* Cycle colors 1..3 through palette entries 1..3 */
	setpal(1, palette[ (i        ) + 1]);
	setpal(2, palette[((i + 1) %3) + 1]);
	setpal(3, palette[((i + 2) %3) + 1]);
	
	/* Cycle colors 4..6 through palette entries 4..6 */
	setpal(4, palette[ (i      % 3) + 4]);
	setpal(5, palette[((i + 1) % 3) + 4]);
	setpal(6, palette[((i + 2) % 3) + 4]);
	i++;
	i %= 3;
}

/**********************************************************************
 * next_char() - puts next character of scroll text on screen
 */
static void next_char()
{
	uchar y  = (vdp23 - FONT_H),
	      ch = (scrolltext[textidx] - ' ');
	uint  sx = ((ch % FONT_CPL) * FONT_W),
	      sy = (FONT_Y + ((ch / FONT_CPL) * FONT_H));
		  
	/* copy background noise */
	cpyv2v(0,   y,    FONT_W-1,  y + FONT_H-1, NOISE_PG, 0, y, c_apage, PSET);
	cpyv2v(sx, sy, sx+FONT_W-1, sy + FONT_H-1, GRAFX_PG, 0, y, c_apage, TPSET);

	textidx++;
	if (scrolltext[textidx]=='\0')
		textidx=0;
}

/**********************************************************************
 * remove_tvs() - removes non-used TVs
 */
static void remove_old_tvs()
{
	tvobj *tvp = tvarray;
	uchar  i;
	
	for(i = 0; i < MAXNOFTVS; i++, tvp++) {
		if ((tvp->x != 0)                  &&     /* this TV is alive and  */
			(tvp->y >= (uchar)(vdp23 + 212)) &&   /* in the invisible area */
			(tvp->y <  (uchar)(vdp23 + 256 - TV_H))) {
			cpyv2v(tvp->x, tvp->y, tvp->x + TV_W-1, tvp->y + TV_H-1, NOISE_PG,
				   tvp->x, tvp->y, c_apage, TPSET);
		//	boxfill(tvp->x, tvp->y, tvp->x+TV_W-1, tvp->y+TV_H-1,13,PSET);
			tvp->x=0;
		}
	}
}

/**********************************************************************
 * CREATE A NEW TV
 */
static void new_tv(void)
{
	static uchar lastx = 0;

	tvobj *tvp = tvarray;
	uchar  tvx = (rand() % 4) * TV_W;
	uchar  tvy = (rand() % 2) * TV_H + TV_Y;
	uchar  i;

	for(i = 0; i < MAXNOFTVS; i++, tvp++) {
		if (tvp->x == 0) {
	        tvp->y = vdp23 - TV_H;
			if(tvp->y <= (255 - TV_H)) {
				tvp->x = rand() % (256 - (TV_W*3) - FONT_W);
				tvp->x += FONT_W;
				if(tvp->x >= (lastx-TV_W))
					tvp->x += 2*TV_W;
#ifdef DEBUG
				{
					if(tvp->x > lastx)
							DBG_out((uchar)(tvp->x - lastx));
					else
							DBG_out((uchar)(lastx - tvp->x));
				}
#endif
				lastx = tvp->x;
				cpyv2v(tvx, tvy,tvx + TV_W-1, tvy + TV_H-1, GRAFX_PG,
				   	tvp->x,tvp->y, c_apage, TPSET);
#ifdef DEBUG
				while(JIFFY < 50)
						;
#endif
				break;
			}
		}
	}
}

/**********************************************************************
 * MAIN ROUTINE
 */

int main ()
{
	uchar clicksw = CLICKSW;

	/* switch off key click */
	clicksw = CLICKSW;
	CLICKSW = 0;

	init();

	do {
		while(JIFFY<3)
			/* just wait */ ;
		JIFFY=0;

		loop_colors();
		
		vdp23-=2;
		wrtvdp(23, vdp23);

		if (!(vdp23 & 15))
		{
			next_char();
		}
		remove_old_tvs();
		if (!(vdp23 & ((TV_H)-1)))
		{
			new_tv();
		}
	}
	while(1);
	kilbuf();
	
	sound(8,0);
	screen(0);
	CLICKSW=clicksw;

	return 0;
}

