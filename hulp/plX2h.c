#include <stdio.h>

/* Reads the first palette from an .PLx file (GraphSaurus palette)
 * and saves the first 16 entries as a table of hex uints:
 * 	"\t0x0GRB,\n"
 *
 * Usage:
 * 	plX2h GFX.PL5 GFXPAL.H
 *
 * Use GFXPAL.H as follows:
 *
 * uint palette[] = {
 * #include <GFXPAL.H>
 * };
 */
int main(int argc, char **argv)
{
	FILE *ifp, *ofp;
	int i;
	char c1, c2;
	
	if(argc != 3)
		return(1);
	if((ifp = fopen(argv[1], "rb")) == NULL)
		return(1);
	if((ofp = fopen(argv[2], "wb")) == NULL)
		return(1);

	for(i=0; i < 16; i++) {
		fscanf(ifp,"%c%c",&c1, &c2);
		fprintf(ofp,"\t0x0%X%X%X,\n", c2&15, (c1>>4)&15, c1&15);
	}

	fclose(ifp);
	fclose(ofp);
	return(0);
}
