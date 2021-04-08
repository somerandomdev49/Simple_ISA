

//assembler for the ISA.
//Supports doing any of the following:
//1) Entering opcodes with their BYTE arguments (short args unsupported)
//sa 0x53
//ldb 0xFF,69
//ldb 0xFF,0x69
//sc 0xFF,0x39
//nop
//Names must be lower case and properly spelled.
//Data must be separated by a comma.
//2) Comments
//Lines beginning with /, #, or ! will be ignored
//3) Multiple commands per line
//You may have multiple commands on the same line like this:
//sa 0x53;cmp;jmpifeq;
//4) Macros
//You may declare a macro with VAR like this
//VAR#my_variable_name#0x53
//You may have at most, 65,535 macros in your file.
//5) Location retrieval
//You may use the location of the current instruction you are writing as a variable.
//It is called @ as a short, or $ as a pair of bytes.
//6) Arbitrary data
//You may specify arbitrary data as either BYTES or SHORTS
//bytes 0xFF,10,34,7
//shorts 0xff10,0x3407
//You may even use macros in the expansion
//bytes myvariablename myothervariablename
//You may use bytes and shorts in-line as if they were commands.
//bytes 5; sa 7;
//7) Arbitrary whitespace
//The assembler does not rely on white space at all to make decisions.
//8) Skipto
//The assembler allows you to "skip" until a desired address (which may wrap around to the beginning...)
//by specifying it with the SECTION tag.
//The memory in-between is *not* filled with nops.
//SECTION 0x1000
//9) Fill
//The assembler allows you to write a byte value for a number of 
//FILLBTS 0x00,0


#include "stringutil.h"
#include <stdio.h>
#include <stdint.h>
char* outfilename = "out.bin";
char* infilename = NULL;
char* variable_names[65535] = {0};
char* variable_expansions[65535] = {0};
char* insns[64] = {
	"halt",
	"lda",
	"la",
	"ldb",
	"lb",
	"sc",
	"sta",
	"stb",
	"add",
	"sub",
	"mul",
	"div",
	"mod",
	"cmp",
	"jmpifeq",
	"jmpifneq",
	"getchar",
	"putchar",
	"and",
	"or",
	"xor",
	"lsh",
	"rsh",
	"ilda",
	"ildb",
	"cab",
	"ab",
	"ba",
	"alc",
	"ahc",
	"nop",
	"cba",
	/*Special insns for 16 bit mode*/
	"lla",
	"illda",
	"llb",
	"illdb",
	/*Self-Indirecting access mode.*/
	"illdaa",
	"illdbb",
	"illdab",
	"illdba",
};
uint8_t insns_numargs[64] = {
	0,//halt
	2,1,2,1, //load and load constant comboes, lda, la, ldb, lb
	2, //load constant into C
	2,2, //store A or B.
	0,0,0,0,0, //the four elementary, + mod
	0, //cmp
	0,0, //jmp insns
	0,0, //getchar,putchar,
	0,0,0,0,0, //bitwise
	0,0, //indirect load.
	0,0,0,0,0,0,0, //Register to register moves and nop.
	
	2,0,2,0, //16 bit constant loads and loads-through-c
	0,0,//16 bit self-indirect loads
	0,0,//16 bit neighbor-indirect loads
};
char* insn_repl[64] = {
	"bytes 0;", //Halt has no arguments.
	/*The load direct load-and-store operations have args.*/
	"bytes 1,",
	"bytes 2,",
	"bytes 3,",
	"bytes 4,",
	"bytes 5,",
	/*Every single other instruction has no arguments.*/
	"bytes 6,",
	"bytes 7,",
	"bytes 8;",
	"bytes 9;",
	"bytes 10;",
	"bytes 11;",
	"bytes 12;",
	"bytes 13;",
	"bytes 14;",
	"bytes 15;",
	"bytes 16;",
	"bytes 17;",
	"bytes 18;",
	"bytes 19;",
	"bytes 20;",
	"bytes 21;",
	"bytes 22;",
	"bytes 23;",
	"bytes 24;",
	"bytes 25;",
	"bytes 26;",
	"bytes 27;",
	"bytes 28;",
	"bytes 29;",
	"bytes 30;",
	"bytes 31;",
	/*16 bit mode insns*/
	"bytes 32,",
	"bytes 33;",
	"bytes 34,",
	"bytes 35;",
	/*Neighbors/self indirection*/
	"bytes 36;",
	"bytes 37;",
	"bytes 38;",
	"bytes 39;",
};
static const uint8_t n_insns = 40;
uint16_t outputcounter = 0;
unsigned int nmacros = 4; /*0,1,2,3*/
char quit_after_macros = 0;
char debugging = 0;
char printlines = 0;

void fputbyte(uint8_t b, FILE* f){
				if(debugging)
					printf("\nWriting individual byte %u\n", b);
if(ftell(f) != outputcounter){
	/*seek to the end*/
	if(debugging)
		puts("Having to move the file counter.\n");
	fseek(f,0,SEEK_END);
	if(ftell(f) > outputcounter) /*The file is already larger than needed.*/
		fseek(f,outputcounter,SEEK_SET); 
	else /*Fill with 0's until we reach the output counter.*/
		while(ftell(f)!=outputcounter)fputc(0, f);
}

fputc(b, f); outputcounter++;
}
void fputshort(uint16_t sh, FILE* f){
	fputbyte(sh/256, f);
	fputbyte(sh, f);
}
int main(int argc, char** argv){
	variable_names[0] = "@";
	variable_names[1] = "$";
	/*Macro to remove whitespace- this assembler works without using any whitespace at all.*/
	variable_names[2] = "\t";
	variable_expansions[2] = "";
	variable_names[3] = " ";
	variable_expansions[3] = "";
	nmacros = 4;
	unsigned long long line_num = 0;
	
	for(int i = 2; i < argc; i++)
	{
		if(strprefix("-o",argv[i-1]))outfilename = argv[i];
		if(strprefix("-i",argv[i-1]))infilename = argv[i];
	}
	for(int i = 1; i < argc; i++)
	{
		if(strprefix("-E",argv[i])) {
			quit_after_macros = 1;
			puts("#Contents of file post macro expansion are as follows:");
		}
		if(strprefix("-DBG",argv[i])) {
			debugging = 1;
			puts("<ASM> Debugging.\n");
		}
		if(strprefix("-pl",argv[i])){
			printlines = 1;
			puts("<ASM> Printing lines.");
		}
	}
	FILE* infile;
	if(debugging) infile=stdin;
	FILE* ofile = fopen(outfilename, "w");
	if(!ofile){
		printf("\nUNABLE TO OPEN OUTPUT FILE %s!!!\n", outfilename);return 1;
	}
	if(infilename){
		if(debugging) printf("\nReading from a file...\n");
		infile = fopen(infilename, "r");
		if(!infile) {
			printf("\nUNABLE TO OPEN INPUT FILE %s!!!\n", infilename);return 1;
		}
	} else {
		if(debugging) printf("\nReading from stdin...");
		if(!debugging) {
			puts("\n<ASM ERROR> May not use reading from stdin when not in debugging mode.\n");
			puts("\n<ASM> Assembler Aborted.");
			return 1;
		}
	}
	while(!feof(infile)){
		unsigned long long linesize;
		char was_macro = 0;
		if(debugging) printf("\nEnter a line...\n");
		char* line = read_until_terminator_alloced(infile, &linesize, '\n', 1);
		if(!line) return 0;
		char* line_copy = strcatalloc(line,"");
		/*Step 0: Skip comment lines*/
		if(strprefix("#",line)) goto end;
		if(strprefix("//",line)) goto end;
		if(strprefix("!",line)) goto end;
		if(strlen(line) < 2) goto end; /*Contains nothing.*/
		if(!isalpha(line[0])) {
			puts("<ASM WARNING> Ignoring line beginning with illegal character...\n");
			goto end;
		}
		/*Step 1: Expand Macros on this line. This includes whitespace removal.*/
		{uint8_t have_expanded = 0; uint16_t iteration = 0;
			do{
				have_expanded = 0;
				if(debugging){
					printf("\n~~Macro Expansion Stage~~, iteration %u\nLine:\n%s", iteration, line_copy);
				}
				for(unsigned int i = 0; i<nmacros; i++){
					long long linesize = strlen(line);
					long long loc = strfind(line, variable_names[i]);
					if(loc == -1) continue;
					char* line_old = line;
					/*We know the location of a macro to be expanded and it is at loc.*/
					/*This also quit conveniently defines the recursion limit for a macro.*/
					have_expanded = 1;
					long long len_to_replace = strlen(variable_names[i]);
					char* before = str_null_terminated_alloc(line_old, loc);
					if(i > 1)
						before = strcatallocf1(before, variable_expansions[i]);
					else if (i == 0){
						char expansion[1024];
						snprintf(expansion, 1023, "%u", outputcounter);
						expansion[1023] = '\0'; /*Just in case...*/
						before = strcatallocf1(before, expansion);
					} else {
						char expansion[1024];
						snprintf(expansion, 1023, "%u,%u", (unsigned int)(outputcounter/256),(unsigned int)(outputcounter&0xff));
						expansion[1023] = '\0'; /*Just in case...*/
						before = strcatallocf1(before, expansion);
					}
					char* after = str_null_terminated_alloc(line_old+loc+len_to_replace, 
									linesize-loc-len_to_replace);
					line = strcatallocfb(before, after);
					free(line_old);
				}
			}while(have_expanded && (iteration++ < 32768));
		}
		/*Step 2: Check to see if this is a macro*/
		if(debugging){
			printf("\n~~Is this a macro?~~\n");
		}
		if(quit_after_macros || debugging) {
			printf("\n#Line post macro expansion:\n");
			puts(line); 
			if(quit_after_macros) goto end;
		}
		if(strprefix("VAR",line)){ /*It's show time!*/
			if(debugging){
				printf("\nThis is a macro!\n");
			}
			was_macro = 1;
			if(nmacros == 0xffff) {
				printf("<ASM ERROR>  Too many macros. Cannot define another one. Line %s\n", line_copy); goto error;
			}
			long long loc_pound = strfind(line, "#");
			long long loc_pound2 = 0;
			if(loc_pound == -1) {
				printf("<ASM SYNTAX ERROR>  missing leading # in macro declaration. Line %s\n", line_copy); goto error;
			}
			/*We have a macro.*/
			char* macro_name = line + loc_pound;
			if(debugging){
				printf("\nMacro Name Pre-Allocation is identified as %s\n", macro_name);
			}
			if(strlen(macro_name) == 1){
				printf("\n<ASM SYNTAX ERROR> missing second # in macro declaration. Line %s\n", line_copy); goto error;
			}
			macro_name += 1; loc_pound += 1; /*We use these again.*/
			loc_pound2 = strfind(macro_name, "#");
			if(loc_pound2 == -1) {
				printf("\n<ASM SYNTAX ERROR> missing second # in macro declaration. Line %s\n", line_copy); goto error;
			}
			macro_name = str_null_terminated_alloc(macro_name, loc_pound2);
			if(debugging){
				printf("\nMacro Name is identified as %s\n", macro_name);
			}
			if(strlen(line + loc_pound + loc_pound2) < 2){
				printf("<ASM SYNTAX ERROR> macro without a definition. line %s\n", line_copy);
				goto error;
			}
			loc_pound2++;
			/*Search to see if we've already defined this macro*/
			char is_overwriting = 0;
			uint16_t index = 0;
			for(uint16_t i = 0; i < nmacros; i++){
				if(streq(macro_name, variable_names[i])){
					printf("<ASM WARNING> redefinition of macro, line: %s\n", line_copy);
					if(i < 4){
						printf("<ASM SYNTAX ERROR> redefinition of critical macro, line: %s\n", line_copy);
						goto error;	
					}
					index = i;
				}
			}
			if(!is_overwriting){
				variable_names[nmacros] = macro_name;
				variable_expansions[nmacros++] = 
				str_null_terminated_alloc(
						line+loc_pound+loc_pound2,
						strlen(line+loc_pound+loc_pound2)
				);
			} else {
				variable_names[index] = macro_name;
				variable_expansions[index] = 
				str_null_terminated_alloc(
						line+loc_pound+loc_pound2,
						strlen(line+loc_pound+loc_pound2)
				);
			}
			if(debugging){
				printf("\nMacro Contents are %s, size %u\n", variable_expansions[nmacros-1], (unsigned int)strlen(variable_expansions[nmacros-1]));
				printf("\nMacro Name is %s, size %u\n", variable_names[nmacros-1], (unsigned int)strlen(variable_names[nmacros-1]));
			}
		}
		/*	most complicated step- handle insns.
			Everything breaks down to bytes underneath, so we convert to "bytes" commands.
			The difference between here and above is that we have to count
			how many commas we're supposed to encounter.
			the first comma beyond that before the next semicolon, is replaced with a semicolon.
		*/
		if(was_macro) goto end;
			{uint8_t have_expanded = 0; uint16_t iteration = 0;
			do{
				have_expanded = 0;
				if(debugging){
					printf("\n~~Ins Expansion Stage~~, iteration %u\nLine:\n%s", iteration, line);
				}
				for(unsigned int i = 0; i<n_insns; i++){
					long long linesize = strlen(line);
					long long loc = strfind(line, insns[i]);
					if(loc == -1) continue;
					char* line_old = line;
					/*We know the location of an insn to be expanded and it is at loc.*/
					/*Crawl along the string. */
					int num_commas_needed = insns_numargs[i] - 1; 
					//If you have three arguments, you only need two commas.
					for(unsigned int j = loc + strlen(insns[i]); strlen(line+j)>0;j++){
						if(line[j] == ','){
							if(num_commas_needed > 0) num_commas_needed--;
							else { /*Too many commas.*/
								printf("\n<ASM SYNTAX ERROR> too many arguments to insn, line %s", line_copy);
								goto error;
							}
						}
						if(line[j] == ';' || line[j] == '\n' || line[j] == '\0'){
							if(num_commas_needed > 0)
							{ /*Too many commas.*/
								printf("\n<ASM SYNTAX ERROR> Insn requires more arguments. line %s", line_copy);
								goto error;
							}
							break; /*Out of this for loop.*/
						}
					}
					if(num_commas_needed > 0)
					{ /*Too many commas.*/
						printf("\n<ASM SYNTAX ERROR> Insn requires more arguments. line %s", line_copy);
						goto error;
					}
					/**/
					have_expanded = 1;
					long long len_to_replace = strlen(insns[i]);
					char* before = str_null_terminated_alloc(line_old, loc);
					before = strcatallocf1(before, insn_repl[i]);
					char* after = str_null_terminated_alloc(line_old+loc+len_to_replace, 
									linesize-loc-len_to_replace);
					line = strcatallocfb(before, after);
					free(line_old);
				}
			}while(have_expanded && (iteration++ < 32700)); /*Probably safe right?*/
		}
		/*
			Put out bytes.
		*/
		/*Inch along*/
		char* metaproc = line;
		do{
			if(strprefix("bytes", metaproc)){
				char* proc = metaproc + 5;
				do{
					uint8_t byteval;
					byteval = strtoull(proc,NULL,0);
					fputbyte(byteval, ofile);
					/*Find the next comma.*/
					long long incr = strfind(proc, ",");
					long long incrdont = strfind(proc, ";");
					if(incr == -1) break;
					if(incrdont != -1 &&
						incr > incrdont)break;/**/
					proc += incr; proc += 1; /*Skip the comma itself.*/
				}while(strlen(proc) > 0);
				
			} else if(strprefix("shorts", metaproc)){
				char* proc = metaproc + 6;
				do{
					uint16_t shortval;
					shortval = strtoull(proc,NULL,0);
					if(debugging)
						printf("\nWriting short %u\n", shortval);
					fputshort(shortval, ofile);
					/*Find the next comma.*/
					long long incr = strfind(proc, ",");
					long long incrdont = strfind(proc, ";");
					if(incr == -1) break;
					if(incrdont != -1 &&
						incr > incrdont) break; /**/
					proc += incr; proc += 1; /*Skip the comma itself.*/
				}while(strlen(proc) > 0);
			} else if(strprefix("section", metaproc)){
				char* proc = metaproc + 7;
				if(strlen(proc) == 0){
					puts("<ASM SYNTAX ERROR> Cannot have empty SECTION tag.");
				}
				uint16_t dest = strtoull(proc, NULL, 0);
				if(dest == 0)
					printf("<ASM WARNING> Section tag at zero. Might be a bad number. Line %s", line_copy);
				if(debugging)
					printf("Moving the output counter to %u\n", dest);
				outputcounter = dest;
			} else if(strprefix("fill", metaproc)){
				char* proc = metaproc + 4;
				if(strlen(proc) == 0){
					puts("<ASM SYNTAX ERROR> Cannot have empty fill tag.");
					goto error;
				}
				uint16_t fillsize = strtoull(proc, NULL, 0);
				if(fillsize == 0){
					printf("<ASM WARNING> fill tag size is. Might be a bad number. Line %s", line_copy);
				}
				/**/
				long long next_comma = strfind(proc, ",");
				if(next_comma == -1) {
					printf("<ASM SYNTAX ERROR> Fill tag missing value. Line %s", line_copy);
					goto error;
				}
				proc += next_comma + 1;
				uint8_t fillval = strtoull(proc, NULL, 0);
				for(;fillsize>0;fillsize--)fputbyte(fillval, ofile);
			} else if(strprefix("asm_print", metaproc)){
				printf("\nRequest to print status at this insn. STATUS:\nLine:\n%s\nLine Internally:\n%s\nCounter: %04x\n", line_copy, line, outputcounter);
			} else if(strprefix("asm_quit", metaproc) || strprefix("asm_quit", metaproc)){
				printf("\nRequest to halt assembly at this insn. STATUS:\nLine:\n%s\nCounter: %04x\n", line_copy, outputcounter);
				goto error;
			} else {
				if(strlen(metaproc) > 0 &&
					strfind(metaproc,";") != 0)
					printf("<ASM WARNING> Ignoring invalid statement on line %s\nStatement:%s\n", line_copy, metaproc);
			}
			long long next_semicolon = strfind(metaproc, ";");
			if(next_semicolon == -1) break; /*We have handled all sublines.*/
			metaproc += next_semicolon + 1;
			if(strlen(metaproc) == 0) break; /*Convenient line break*/
		} while(1);
		end:
		free(line);
		free(line_copy);
		continue;
		/*Yikes!*/
		error:
		puts("\n<ASM> Assembler Aborted.");
		return 1;
	}
	puts("\n<ASM> Assembly successful.");
	fclose(ofile);
	fclose(infile);
	return 0;
}