/* $Id: snow.c,v 1.4 2003/12/30 23:50:52 eric Exp $
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

#define MAXNOFTVS 8
#define TV_H      32
#define TV_W      32
#define TV_Y      80

#define SCROLL_SPD 4

#define ST_DEAD 0
#define ST_DYING 1
#define ST_ALIVE 2

/**********************************************************************/

typedef struct
{
	unsigned char oldx[2];
	unsigned char oldy[2];
	unsigned char x;
	unsigned char y; // in VRAM
	unsigned char imx;
	unsigned char imy; // on the source image page
	char vx;
	char vy;
	char state; 
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
static uchar dpage=0;

/**********************************************************************
 * AUXILIARY ROUTINES                                                 *
 **********************************************************************/

/**********************************************************************
 * cpyv2v_wrap() - better cpy
 */
void cpyv2v_wrap(uint sx1, uint sy1, uint sx2, uint sy2, uchar sp,
						     uint dx,  uint dy, uchar dp, uchar logop)
{
	uint h = sy2 - sy1;
	uint h1 = 255 - sy1;
	uint h2;
	dy &= 255; // afromen die handel!
	h2 = 255 - dy;
	
	if (h1 > h ) h1=h;
	if (h2 > h ) h2=h;

	if (h2 < h1) { h2^=h1; h1^=h2; h2^=h1; } // swap h1, h2

	cpyv2v(sx1, sy1, sx2, sy1+h1, sp, dx, dy, dp, logop);
	
	if ((h1!=h) && (h1!=h2)) 
		cpyv2v(sx1, (sy1+h1+1) & 255, sx2, (sy1+h2) & 255, sp, dx, (dy+h1+1) & 255, dp, logop);
	if (h2!=h)
		cpyv2v(sx1, (sy1+h2+1) & 255, sx2, sy2 & 255, sp, dx, (dy+h2+1) & 255, dp, logop);

}
											 
/**********************************************************************
 * init () - initalisation routine
 */
static void init(void)
{
	uchar i;
	char  filename[13];

	//DBG_mode((uchar)0x1F); /* single byte, decimal) */
	
	/* Initialize graphics */
	ginit();

	color(15, 0, 0);
	screen(5);
	wrtvdp(8, 10);
	disscr();
	
	for(i = 0; i < MAXNOFTVS; i++) {
		tvarray[i].state = ST_DEAD;
		tvarray[i].x = FONT_W;
		tvarray[i].vx = 0;
		tvarray[i].vy = 0;
	}

	for(i = 0; i < 16; i++)
		setpal(i, palette[i]);

	/* Load the real stuff... */
	strcpy(filename, NOISE_FILE); 
	gs2loadgrp(5, NOISE_PG, filename);
	strcpy(filename, GRAFX_FILE); 
	gs2loadgrp(5, GRAFX_PG, filename);
	
	/* hack to 'grey' the normally invisible area of the screen */
	cpyv2v(0, 100, 255, 255-212+100, NOISE_PG, 0, 212, NOISE_PG, PSET);
	/* debug*/
#if DEBUG
	setpg(0,NOISE_PG);
	boxfill(0,212,255,255,8,PSET);
	setpg(0,0);
#endif
	/* end debug/hack */
	
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
	setpg(dpage, 1-dpage);
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
	cpyv2v_wrap(0,   y,    FONT_W-1,  y + FONT_H-1, NOISE_PG, 0, y, 1-c_apage, PSET);
	cpyv2v_wrap(sx, sy, sx+FONT_W-1, sy + FONT_H-1, GRAFX_PG, 0, y, 1-c_apage, TPSET);
	cpyv2v_wrap(0,   y,    FONT_W-1,  y + FONT_H-1, NOISE_PG, 0, y, c_apage, PSET);
	cpyv2v_wrap(sx, sy, sx+FONT_W-1, sy + FONT_H-1, GRAFX_PG, 0, y, c_apage, TPSET);
	textidx++;
	if (scrolltext[textidx]=='\0')
		textidx=0;
}

/**********************************************************************
 * new_tv() - create a new tv
 */
static void new_tv(void)
{
	tvobj *tvp = tvarray;
	uchar  tvx = (rand() % 4) * TV_W;
	uchar  tvy = (rand() % 2) * TV_H + TV_Y;
	uchar  i;

	for(i = 0; i < MAXNOFTVS; i++, tvp++) {
		if (tvp->state == ST_DEAD) {
			tvp->state = ST_ALIVE;
			tvp->imx=tvx;
			tvp->imy=tvy;
			tvp->vy = (rand()%((SCROLL_SPD<<1)-1)) - SCROLL_SPD + 1;
			tvp->vx = ((rand()%((SCROLL_SPD<<1)-1)) - SCROLL_SPD + 1)/2;
	        tvp->y = vdp23 - TV_H;
	        tvp->oldy[0] = tvp->y;
	        tvp->oldy[1] = tvp->y;
	        tvp->oldx[0] = tvp->x;
	        tvp->oldx[1] = tvp->x;
			tvp->x = rand() % (256 - TV_W - FONT_W);
			tvp->x += FONT_W;
			break;
		}
	}
}

/**********************************************************************
 * move_tvs() - Calc. new coordinates
 */
static void move_tvs(void)
{
	uchar  i;
	tvobj *tvp = tvarray;

	for(i = 0; i < MAXNOFTVS; i++, tvp++) {
		if (tvp->state == ST_ALIVE) {
			tvp->oldy[dpage]=tvp->y;
			tvp->y+=tvp->vy; // autowrap: uchars
			tvp->oldx[dpage]=tvp->x;
			tvp->x+=tvp->vx; // autowrap: uchars
			if (tvp->x<FONT_W)
			{
				tvp->x=FONT_W;
				tvp->vx=-tvp->vx;
			} else if (tvp->x > (255-TV_W))
			{
				tvp->x=255-TV_W;
				tvp->vx=-tvp->vx;
			}
		
			if ( (tvp->y >= (uchar)(vdp23 + 212)) &&   /* in the invisible area */
				(tvp->y <  (uchar)(vdp23 - TV_H - SCROLL_SPD))) 
				{ tvp->state=ST_DYING; }
		}
	}
}

/**********************************************************************
 * anim_tvs() - Move them on the screen 
 */
static void anim_tvs(void)
{
	uchar  i;
	tvobj *tvp = tvarray;

	for(i = 0; i < MAXNOFTVS; i++, tvp++) {
		if (tvp->state != ST_DEAD) {
			uint oldy = tvp->oldy[1-dpage];
			uint oldx = tvp->oldx[1-dpage];
			cpyv2v_wrap(oldx, oldy, oldx+TV_W-1,  oldy+TV_H-1, NOISE_PG,
						oldx, oldy, 1-dpage, PSET);
			if (tvp->state == ST_DYING)
			{
				oldy = tvp->oldy[dpage];
				oldx = tvp->oldx[dpage];
				cpyv2v_wrap(oldx, oldy, oldx+TV_W-1,  oldy+TV_H-1, NOISE_PG,
						    oldx, oldy, dpage, PSET);
				tvp->state = ST_DEAD; // object is dead
			}
		}
	}
	tvp = tvarray;
	for(i = 0; i < MAXNOFTVS; i++, tvp++) {
		if (tvp->state == ST_ALIVE) {
			cpyv2v_wrap(tvp->imx, tvp->imy, tvp->imx+TV_W-1, tvp->imy+TV_H-1, GRAFX_PG,
						tvp->x, tvp->y, 1-dpage, TPSET);
		}
	}
}

/**********************************************************************
 * MAIN ROUTINE
 */

int main ()
{
	uchar charcnt = 0, i;
	uchar clicksw = CLICKSW;

	/* switch off key click */
	clicksw = CLICKSW;
	CLICKSW = 0;

	init();

	do {
		while(JIFFY<2)
			/* just wait */ ;
		setpg(dpage, 1-dpage);
		JIFFY=0;

		loop_colors();
		
		vdp23 -= SCROLL_SPD;
		charcnt += SCROLL_SPD;
		wrtvdp(23, vdp23);

		if (charcnt >= FONT_H)
		{
			next_char();
			charcnt = 0;
		}
//		remove_old_tvs();
//		if (!(vdp23 & ((TV_H)-1)))
		{
			new_tv();
		}
		move_tvs();
		anim_tvs();
		/* flip */
		dpage=1-dpage;
	}
	while(!kbhit());
	kilbuf();
	wrtvdp(23, 0);
	for (i=0; i<4; i++)	
	{
		setpg(i,i);
		JIFFY=0;
		while(JIFFY<100);
	}	
	sound(8,0);
	screen(0);
	CLICKSW=clicksw;

	return 0;
}

