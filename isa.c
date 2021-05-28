#include "d.h"
#include "isa.h"
/*
	Textmode emulator for SISA16.
*/
static FILE* F;

int main(int rc,char**rv){
	size_t img_size;
	size_t i=0;
	UU j=0;
	M = malloc((((UU)1)<<24));
	if(!M){puts("Failed Malloc.");return 1;}
	if(
		(sizeof(U) != 2) ||
		(sizeof(u) != 1) ||
		(sizeof(UU) != 4)
	){
		puts("SISA16 ERROR!!! Compilation Environment conditions unmet.");
		if(sizeof(U) != 2)
			puts("U is not 2 bytes. Try using something other than unsigned short (default).");
		if(sizeof(u) != 1)
			puts("u is not 2 bytes. Try using something other than unsigned char (default).");
		if(sizeof(UU) != 4)
			puts("UU is not 4 bytes. Try toggling -DUSE_UNSIGNED_INT. the default is to use unsigned int as UU.");
		return 1;
	}
	if(rc<2){
		puts("SISA-16 Emulator written by DMHSW for the Public Domain");
		printf("\nUsage: %s myprogram.bin\n", rv[0]);
		return 1;
	}
	F=fopen(rv[1],"rb");
	if(!F){
		puts("SISA16 emulator cannot open this file.");
		exit(1);
	}
	fseek(F,0,SEEK_END);
	/*determine the size of the image*/
	img_size = ftell(F);
	if(img_size > 0x1000000){
		M = realloc(M, img_size);
	}
	rewind(F);
		for(i=0;F&&!feof(F);){M[i++]=fgetc(F);}
	fclose(F);
	for(i=e();i<(1<<24)-31&&rc>2;i+=32)	
		for(j=i,printf("%s\n%04lx|",(i&255)?"":"\n~",(unsigned long)i);j<i+32;j++)
			printf("%02x%c",M[j],((j+1)%8)?' ':'|');
	if(R==1)puts("\n<Errfl, 16 bit div by 0>\n");
	if(R==2)puts("\n<Errfl, 16 bit mod by 0>\n");
	if(R==3)puts("\n<Errfl, 32 bit div by 0>\n");
	if(R==4)puts("\n<Errfl, 32 bit mod by 0>\n");
}
