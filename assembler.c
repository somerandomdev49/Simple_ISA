/*
assembler for the ISA.
Supports doing any of the following:
1) Entering opcodes with their BYTE arguments (short args unsupported)
sa 0x53
ldb 0xFF,69
ldb 0xFF,0x69
sc 0xFF,0x39
nop
Names must be lower case and properly spelled.
Data must be separated by a comma.
2) Comments
Lines beginning with /, , or # are comments.
3) Multiple commands per line
You may have multiple commands on the same line like this:
sa 0x53;cmp;jmpifeq;
4) Macros
You may declare a macro with VAR like this
VAR#my_variable_name#0x53
You may have at most, 65,535 macros in your file.
5) Location retrieval
You may use the location of the current instruction you are writing as a variable.
It is called @ as a short, or $ as a pair of bytes.
6) Arbitrary data
You may specify arbitrary data as either BYTES or SHORTS
bytes 0xFF,10,34,7
shorts 0xff10,0x3407
You may even use macros in the expansion
bytes myvariablename myothervariablename
You may use bytes and shorts in-line as if they were commands.
bytes 5; sa 7;
7) (Mostly) Arbitrary whitespace (On a single line)
You can omit whitespace between keywords in most cases.
However some operations require that there be no whitespace.
Character literals for instance must start at the beginning of a line with no preceding whitespace.
8) Skipto
The assembler allows you to "skip" until a desired address (which may wrap around to the beginning...)
by specifying it with the SECTION tag.
If the space-in-between in the ROM does not exist it will be filled in with zeroes.
section 0x1000
9) Fill
The assembler allows you to write a byte value for a number of 
fill 0xFFFF,0
10) Strings
The assembler allows you to directly write ascii characters into your output bin.
Lines beginning with ! are character literal lines and they will be written to the current output location.
11) Assembly-time directives
%short%... converts number to a split short. It must be literal.
Assembly-time directives must have NO leading or trailing whitespace.
12) Builtin macros
You can retrieve the current location in the binary as a short with @, and as a byte pair with $
You can offset these like this: $+93+ or @+15+
13) asm_macro_call#macroname#arg1#arg2#
you can call a macro of name macroname with _arg1, _arg2, _arg3... being automatically defined for you.
14) |

*/

#include "stringutil.h"
#include <stdio.h>
#include <string.h>
#include "d.h"
#include "isa.h"
char* outfilename = "outsisa16.bin";
char run_sisa16 = 0;
char clear_output = 0;
char use_tempfile = 0;
void del_outfile(){
	if(use_tempfile)
		remove(outfilename);
	return;
}
void ASM_PUTS(const char* s){if(!clear_output)puts(s);}
char* infilename = NULL;
char* variable_names[65535] = {0};
char* variable_expansions[65535] = {0};
char variable_is_redefining_flag[65535] = {0};
char* insns[207] = {
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
	"ca",
	"cb",
	"ac",
	"bc",
	"ista",
	"istb",
	"istla",
	"istlb",
	"jmp",
	"stla",
	"stlb",
	"stc",
	"push",
	"pop",
	"pusha",
	"popa",
	"astp",
	"bstp",
	"compl",
	/*Call structure*/
	"cpc",
	"call",
	"ret",
	/*Far Memory Access*/
	"farillda",
	"faristla",	
	"farilldb",
	"faristlb",
	"farpagel",
	"farpagest",
	"lfarpc",
	"farcall",
	"farret",
	"farilda",
	"farista",
	"farildb",
	"faristb",
	"imul15.1",
	"imul14.2",
	"imul13.3",
	"imul12.4",
	"imul11.5",
	"imul10.6",
	"imul9.7",
	"imul8.8",
	"imul7.9",
	"imul6.10",
	"imul5.11",
	"imul4.12",
	"imul3.13",
	"imul2.14",
	"imul1.15",
	"imul.16",
	/*quickly push registers onto the stack*/
	"alpush",
	"blpush",
	"cpush",
	"apush",
	"bpush",
	/*quickly pop registers from the stack*/
	"alpop",
	"blpop",
	"cpop",
	"apop",
	"bpop",
	"interrupt",
	"clock",
	/*Extra register loads and stores.*/
	/*rx0*/
	"arx0",
	"brx0",
	"crx0",
	"rx0a",
	"rx0b",
	"rx0c",
	/*rx1*/
	"arx1",
	"brx1",
	"crx1",
	"rx1a",
	"rx1b",
	"rx1c",
	/*rx2*/
	"arx2",
	"brx2",
	"crx2",
	"rx2a",
	"rx2b",
	"rx2c",
	/*rx3*/
	"arx3",
	"brx3",
	"crx3",
	"rx3a",
	"rx3b",
	"rx3c",
	/*Register-to-register moves for RX registers.*/
	"rx0_1",
	"rx0_2",
	"rx0_3",

	"rx1_0",
	"rx1_2",
	"rx1_3",

	"rx2_0",
	"rx2_1",
	"rx2_3",

	"rx3_0",
	"rx3_1",
	"rx3_2",
	/*lRXX*/
	"lrx0",
	"lrx1",
	"lrx2",
	"lrx3",
	/*farildRXX*/
	"farildrx0",
	"farildrx1",
	"farildrx2",
	"farildrx3",
	/*faristRXX*/
	"faristrx0",
	"faristrx1",
	"faristrx2",
	"faristrx3",
	/*RXmath*/
	"rxadd",
	"rxsub",
	"rxmul",
	"rxdiv",
	"rxmod",
	"rxrsh",
	"rxlsh",
	"rx0push",
	"rx1push",
	"rx2push",
	"rx3push",
	"rx0pop",
	"rx1pop",
	"rx2pop",
	"rx3pop",
	"rxand",
	"rxor",
	"rxxor",
	"rxcompl",
	"rxcmp",
	"seg_ld",
	"seg_st",
	"seg_realloc",
	"fltadd",
	"fltsub",
	"fltmul",
	"fltdiv",
	"fltcmp",
	"seg_pages",
	"ilrx0_1",
	"ilrx0_0",
	"farjmprx0",
	"istrx0_1",
	"istrx1_0",
	"cbrx0",
	"carx0",
	"rxidiv",
	"rximod",
	/*requires 3 byte arguments.*/
	"farldrx0",
	"farldrx1",
	"farldrx2",
	"farldrx3",
	"farllda",
	"farlldb",
	"farldc",
	"farstrx0",
	"farstrx1",
	"farstrx2",
	"farstrx3",
	"farstla",
	"farstlb",
	"farstc",
	"aincr",
	"adecr",
	"rxincr",
	"rxdecr"
};
unsigned char insns_numargs[207] = {
	0,/*halt*/
	2,1,2,1, /*load and load constant comboes, lda, la, ldb, lb*/
	2, /*load constant into C*/
	2,2, /*store A or B.*/
	0,0,0,0,0, /*the four elementary, + mod*/
	0, /*cmp*/
	0,0, /*jmp insns*/
	0,0, /*getchar,putchar,*/
	0,0,0,0,0, /*bitwise*/
	0,0, /*indirect load.*/
	0,0,0,0,0,0,0, /*Register to register moves and nop.*/
	
	2,0,2,0, /*16 bit constant loads and loads-through-c*/
	0,0,/*16 bit self-indirect loads*/
	0,0,/*16 bit neighbor-indirect loads*/
	0,0,/*move to C*/
	0,0,/*move from C*/
	0,0,0,0,/*Indirect Stores*/
	0, /*Direct jump.*/
	2,2,2, /*store shorts A,B,and C*/
	2,2,0,0, /*stack pointer ops*/
	0,0, /*retrieve the stack pointer and put it in a or b*/
	0, /*Compl*/
	/*call structure.*/
	0,/*cpc*/
	0,/*call*/
	0,/*ret*/
	0,0, /*Far memory access- a*/
	0,0, /*Far memory access- b*/
	0,0, /*Far page load and store*/
	0,0,0, /*lfarpc, farcall, farret,*/
	0,0,0,0, /*Far memory access- a and b, as single bytes*/
	/*Fixed point multiplies.*/
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	/*More stack ops*/
	0,0,0,
	0,0, 
	/*register pops*/
	0,0,0,
	0,0, 
	/*extended interrupt/device interaction*/
	0,
	/*Clock.*/
	0,
	/*extended registers.*/
	/*RX0*/
		0,0,0,
		0,0,0,
	/*RX1*/
		0,0,0,
		0,0,0,
	/*RX2*/
		0,0,0,
		0,0,0,
	/*RX3*/
		0,0,0,
		0,0,0,
	/*Register-to-register moves for RX registers.*/
		0,0,0,
		0,0,0,
		0,0,0,
		0,0,0,
	/*lRXX*/
		4,4,4,4,
	/*farildRXX and st*/
		0,0,0,0,
		0,0,0,0,
	/*RXmath and CMP*/
		0,0,0,0,
		0,/*mod*/
		0,0,/*shifts*/
		/*push/pop*/
		0,0,0,0,
		0,0,0,0,
		/*bitwise and compl*/
		0,0,0, /*rxand,rxor,rxxor*/
		0,
		/*rxcmp*/
		0,
		/*segment*/
		0,0,0,
		/*float*/
		0,0,0,0,0,
		/*seg_pages*/
		0,
		/*ilrx0_1 and 0_0*/
		0,0,
		/*farjmprx0*/
		0,
		/*istrx0_1 and istrx1_0*/
		0,0,
		/*cbrx0 and carx0*/
		0,0,
		/*rxidiv and rximod*/
		0,0,
		/*farld*/
		3,3,3,3, 3,3,3,
		/*farst*/
		3,3,3,3, 3,3,3,
		/*incr decr*/
		0,0,0,0
		
};
char* insn_repl[207] = {
	"bytes0;", 
	/*The direct load-and-store operations have args.*/
	"bytes1,",
	"bytes2,",
	"bytes3,",
	"bytes4,",
	"bytes5,",
	/*stores have arguments*/
	"bytes6,",
	"bytes7,",
	/*have no arguments.*/
	"bytes8;",
	"bytes9;",
	"bytes10;",
	"bytes11;",
	"bytes12;",
	"bytes13;",
	"bytes14;",
	"bytes15;",
	"bytes16;",
	"bytes17;",
	"bytes18;",
	"bytes19;",
	"bytes20;",
	"bytes21;",
	"bytes22;",
	"bytes23;",
	"bytes24;",
	"bytes25;",
	"bytes26;",
	"bytes27;",
	"bytes28;",
	"bytes29;",
	"bytes30;",
	"bytes31;",
	/*16 bit mode insns*/
	"bytes32,",
	"bytes33;",
	"bytes34,",
	"bytes35;",
	/*self, neighbors indirection*/
	"bytes36;",
	"bytes37;",
	"bytes38;",
	"bytes39;",
	/*direct move to c*/
	"bytes40;",
	"bytes41;",
	/*direct move from c*/
	"bytes42;",
	"bytes43;",
	/*indirect stores*/
	"bytes44;",
	"bytes45;",
	"bytes46;",
	"bytes47;",
	"bytes48;",
	/*Store shorts to constant address*/
	"bytes49,",
	"bytes50,",
	"bytes51,",
	/*Stack pointer operations.*/
	/*Push and pop by constant amounts*/
	"bytes52,",
	"bytes53,",
	/*using a*/
	"bytes54;",
	"bytes55;",
	/*stack pointer retrieval into a or b*/
	"bytes56;",
	"bytes57;",
	/*compl of a*/
	"bytes58;",
	/*cpc*/
	"bytes59;",
	"bytes60;",  /*call*/
	"bytes61;",  /*ret*/
	/*far memory.*/
	"bytes62;", /*farillda*/
	"bytes63;", /*faristla*/
	"bytes64;", /*farilldb*/
	"bytes65;", 
	"bytes66;", 
	"bytes67;", 
	"bytes68;", 
	"bytes69;", 
	"bytes70;", 
	"bytes71;", 
	"bytes72;", 
	"bytes73;", 
	"bytes74;", 
	/*Fixed point ops*/
	"bytes75;","bytes76;","bytes77;","bytes78;",
	"bytes79;","bytes80;","bytes81;","bytes82;",
	"bytes83;","bytes84;","bytes85;","bytes86;",
	"bytes87;","bytes88;","bytes89;","bytes90;",
	/*Stack ops- pushes*/
	"bytes91;",
	"bytes92;",
	"bytes93;",
	/*single byte stack ops*/
	"bytes94;",
	"bytes95;",
	/*Stack ops- pops*/
	"bytes96;",
	"bytes97;",
	"bytes98;",
	"bytes99;",
	"bytes100;",
	/*interrupt*/
	"bytes101;",
	/*clock*/
	"bytes102;",
	/*extended register interactions.*/
	/*RX0*/
		"bytes103;",
		"bytes104;",
		"bytes105;",

		"bytes106;",
		"bytes107;",
		"bytes108;",
	/*RX1*/
		"bytes109;",
		"bytes110;",
		"bytes111;",

		"bytes112;",
		"bytes113;",
		"bytes114;",
	/*RX2*/
		"bytes115;",
		"bytes116;",
		"bytes117;",

		"bytes118;",
		"bytes119;",
		"bytes120;",
	/*RX3*/
		"bytes121;",
		"bytes122;",
		"bytes123;",

		"bytes124;",
		"bytes125;",
		"bytes126;",
	/*Register-to-register moves for RX registers.*/
		"bytes127;",
		"bytes128;",
		"bytes129;",

		"bytes130;",
		"bytes131;",
		"bytes132;",

		"bytes133;",
		"bytes134;",
		"bytes135;",

		"bytes136;",
		"bytes137;",
		"bytes138;",
	/*lRXX*/
		"bytes139,",
		"bytes140,",
		"bytes141,",
		"bytes142,",
	/*farildRXX*/
		"bytes143;",
		"bytes144;",
		"bytes145;",
		"bytes146;",
	/*faristRXX*/
		"bytes147;",
		"bytes148;",
		"bytes149;",
		"bytes150;",
	/*More RX stuff.*/
		"bytes151;",
		"bytes152;",
		"bytes153;",
		"bytes154;",
		"bytes155;",
		"bytes156;",
		"bytes157;",
		"bytes158;",
		"bytes159;",
		"bytes160;",
		"bytes161;",
		"bytes162;",
		"bytes163;",
		"bytes164;",
		"bytes165;",
		"bytes166;",
		"bytes167;",
		"bytes168;",
		"bytes169;",
	/*rxcmp*/
		"bytes170;",
	/*segment*/
		"bytes171;",
		"bytes172;",
		"bytes173;",
	/*float*/
		"bytes174;",
		"bytes175;",
		"bytes176;",
		"bytes177;",
	/*cmp*/
		"bytes178;",
	/*seg_pages*/
		"bytes179;",
	/*ilrx0_1 and ilrx0_0*/
		"bytes180;",
		"bytes181;",
	/*farjmprx0*/
		"bytes182;",
		"bytes183;",
		"bytes184;",
	/*cbrx0 and carx0*/
		"bytes185;",
		"bytes186;",
	/*rxidiv and rximod*/
		"bytes187;",
		"bytes188;",
	/*the 14 instructions that require 3 bytes as argument.*/
		"bytes189,","bytes190,","bytes191,","bytes192,",   "bytes193,","bytes194,","bytes195,",
		"bytes196,","bytes197,","bytes198,","bytes199,",   "bytes200,","bytes201,","bytes202,",
	/*incr decr*/
		"bytes203;","bytes204;","bytes205;","bytes206;"
};
static const unsigned int n_insns = 207;
char int_checker(char* proc){
	char int_mode = 0; /*starting with 0x means */
	char first_character = 1;
	if(proc[0] == ',' || proc[0] == ';' || proc[0] == '\0') return 1;
	if(proc[0] == '0') {int_mode = 1;proc++;} /*octal*/
	if(*proc && int_mode == 1) {if(proc[0] == 'x') {int_mode = 2;proc++;} else return 0;} /*hex*/
	if(int_mode == 1 && (proc[0] == ',' || proc[0] == ';' || proc[0] == '\0')) return 0; /*zero.*/
	for(;!(proc[0] == ',' || proc[0] == ';' || proc[0] == '\0');proc++){
		/*Check hex*/
		if(int_mode==2 && 
		!( (*proc <= '9' && *proc >= '0') || (*proc <= 'F' && *proc >= 'A') || (*proc <= 'f' && *proc >= 'a'))
		){
			return 1; /*they have 0x, but no numbers!*/
		}
		if(int_mode==0 && 
		!(*proc <= '9' && *proc >= '0')){
			return 1;
		}
		if(int_mode==1 && 
		!( (*proc <= '8' && *proc >= '0') )){
			return 1;
		}
		first_character = 0;
	}
	if(first_character && int_mode != 1) return 1; /*There were no characters in the number.*/
	return 0;
}


unsigned long outputcounter = 0;
unsigned long nmacros = 5; /*0,1,2,3,4*/
char quit_after_macros = 0;
char debugging = 0;
unsigned long npasses = 0;
char printlines = 0;
unsigned long linesize = 0;char* line = NULL, *line_copy = NULL;
unsigned long region_restriction = 0;
char region_restriction_mode = 0; /*0 = off, 1 = block, 2 = region*/
static void fputbyte(unsigned char b, FILE* f){
	if(!run_sisa16 && !quit_after_macros)
		if((unsigned long)ftell(f) != outputcounter){
			/*seek to the end*/
			if(debugging)
				ASM_PUTS("Having to move the file counter.\n");
			fseek(f,0,SEEK_END);
			if((unsigned long)ftell(f) > outputcounter) /*The file is already larger than needed.*/
				fseek(f,outputcounter,SEEK_SET); 
			else /*Fill with 0's until we reach the output counter.*/
				while((unsigned long)ftell(f)!=outputcounter)fputc(0, f);
		}
	switch(region_restriction_mode){
		case 0: break;
		case 1:{
			if (((outputcounter>>8) & 0xFFFF)  != region_restriction){
				printf("<ASM COMPILATION ERROR> block restriction failed. Outputcounter exited 256 byte bounds. Line:\n%s", line_copy); 
				exit(1);
			}
		}
		break;
		case 2:{
			if (((outputcounter>>16) & 0xFF)  != region_restriction){
				printf("<ASM COMPILATION ERROR> region restriction failed. Outputcounter exited 64k bounds. Line:\n%s", line_copy);
				exit(1);
			}
		}
		break;
		default: puts("<ASM INTERNAL ERROR> invalid region_restriction_mode set somehow."); exit(1);
	}
	if(!quit_after_macros){
		if(!run_sisa16){
			if(npasses == 1)
				fputc(b, f);
		} else {
			if(npasses == 1)
				M[outputcounter]=b;
		}
	}
	outputcounter++; outputcounter&=0xffffff;
}
static void putshort(unsigned short sh, FILE* f){
	fputbyte(sh/256, f);
	fputbyte(sh, f);
}
#define ASM_MAX_INCLUDE_LEVEL 20
static FILE* fstack[ASM_MAX_INCLUDE_LEVEL];
unsigned long compiled_variable_allocation_start = 0xF00000;

char* compiled_variable_names[65535] = {0};
unsigned long compiled_variable_addresses[65535] = {0};
unsigned long compiled_variable_array_lengths[65535] = {0};
unsigned int compiled_variable_types[65535] = {0};
unsigned long n_compiled_variables = 0;

static int check_ident(char* ident){
	unsigned long i;
	if(ident[0] == '\0') return 0; /*End of the damn string!*/
	if(!(isalpha(ident[0]) || ident[0] == '_')) return 0; /*Bad identifier*/
	for(i=1;ident[i]!=';' && ident[i] != '\0' && ident[i] != ' ';i++){
		if(isalnum(ident[i]) || ident[i] == '_')
			continue;
		else
			return 0; /*bad identifier!*/
	}
	if(ident[i] == '\0') return 0; /*Bad Identifier*/
	if(ident[i] == ' ') return 1; /*Valid*/
	if(ident[i] == ';') return 1; /*Valid*/
	return 0; /*Theoretically unreachable.*/
}

static char* compile_line(char* line_in){
	char* line_proc = strcatalloc(line_in+strlen("ASM_COMPILE"), "");
	char* line_out = strcatalloc("","");
	while(strfind(line_proc, "\t") != -1) line_proc = str_repl_allocf(line_proc, "\t", " ");
	while(strfind(line_proc, "  ") != -1) line_proc = str_repl_allocf(line_proc, "  ", " ");
	while(strfind(line_proc, " ;") != -1) line_proc = str_repl_allocf(line_proc, " ;", ";");
	while(strfind(line_proc, "; ") != -1) line_proc = str_repl_allocf(line_proc, "; ", ";");
	while(strprefix(" ", line_proc)) {
		char* old_line_proc = line_proc;
		line_proc = strcatalloc(line_proc+1, "");
		free(old_line_proc);
	}
	/*the line is now ready for processing.*/
	if(strprefix("var ", line_proc)){
		/*
			FORMAT:
											Required Space
											   |Optional array declarator from [ to ]
											   VV
			var <type> @<location> <identifier> [<array length>];
					   ^			^
						optional	must start with alpha character or underscore, rest isalnum or is underscore.
		*/
		char* metaproc = line_proc + strlen("var ");
		unsigned long vartype = 0;
		unsigned long varaddr = compiled_variable_allocation_start;
		unsigned long vararrlen = 0; /*Not an array!*/
		char* varname = NULL;
		/*handle a variable declaration.*/
		if(strprefix("byte ", metaproc)){
			vartype = 0; metaproc += strlen("byte ");
			
		} else if(strprefix("short ", metaproc)){
			vartype = 1; metaproc += strlen("short ");
		} else if(strprefix("u32 ", metaproc)){
			vartype = 2; metaproc += strlen("u32 ");
		} else if(strprefix("i32 ", metaproc)){
			vartype = 3; metaproc += strlen("i32 ");
		} else if(strprefix("f32 ", metaproc)){
			vartype = 4; metaproc += strlen("f32 ");
		} else {
			puts("<COMPILED LINE SYNTAX ERROR> variable declaration without valid type or possibly missing space");
			puts("Line:");
			puts(line_in);
			exit(1);
		}
		if(strprefix("@", metaproc)){
			long loc_next_space;
			metaproc++;
			varaddr = strtoul(metaproc,0,0);
			compiled_variable_allocation_start = varaddr;
			loc_next_space = strfind(metaproc, " ");
			if(loc_next_space == -1) {
				puts("<COMPILED LINE SYNTAX ERROR> variable declaration is incomplete.");
				puts("Line:");
				puts(line_in);
				exit(1);
			}
			metaproc += loc_next_space+1;
		}
		if(!check_ident(metaproc)){
			puts("<COMPILED LINE SYNTAX ERROR> variable declaration with invalid identifier. Remember that arrays must be declared");
			puts("Line:");
			puts(line_in);
			exit(1);
		}

		{long loc_next_space;
			loc_next_space = strfind(metaproc, " ");
			if(loc_next_space == -1) loc_next_space = strfind(metaproc, ";");
			if(loc_next_space == -1) {
				puts("<COMPILED LINE SYNTAX ERROR> variable declaration is incomplete. Identifier must be followed by a space or semicolon.");
				puts("Line:");
				puts(line_in);
				exit(1);
			}
			varname = str_null_terminated_alloc(metaproc, loc_next_space);
			metaproc += loc_next_space;
			if(*metaproc == ' ')metaproc++;
		}
		/*test to see if this is an array declaration.*/
		if(*metaproc == '[' /*]*/){
			long loc_end_bracket;
			metaproc++;
			vararrlen = strtoul(metaproc, 0,0);
			loc_end_bracket = strfind(metaproc, /*[*/ "]");
			if(loc_end_bracket == -1){
				puts("<COMPILED LINE SYNTAX ERROR> Array declarators must have an ending bracket!");
				puts("Line:");
				puts(line_in);
				exit(1);
			}
			metaproc += loc_end_bracket+1;
		}
		if(metaproc[0] != ';'){
			puts("<COMPILED LINE SYNTAX ERROR> Variable declaration lacking post semicolon.");
			puts("Line:");
			puts(line_in);
			exit(1);
		}
		compiled_variable_types[n_compiled_variables] = vartype;
		compiled_variable_names[n_compiled_variables] = varname;
		if(varname == NULL || strlen(varname) == 0){
			puts("<COMPILED LINE INTERNAL ERROR> Variable declaration stmt reached completion with no name.");
			puts("Line:");
			puts(line_in);
			exit(1);
		}
		compiled_variable_addresses[n_compiled_variables] = varaddr;
		if(vararrlen == 0){
			if(vartype == 0) compiled_variable_allocation_start++;
			else if(vartype == 1)  compiled_variable_allocation_start += 2;
			else if(vartype == 2)  compiled_variable_allocation_start += 4;
			else if(vartype == 3)  compiled_variable_allocation_start += 4;
			else if(vartype == 4)  compiled_variable_allocation_start += 4;
		} else if(vartype == 0){
			compiled_variable_allocation_start += vararrlen;
		} else if(vartype == 1){
			compiled_variable_allocation_start += vararrlen*2;
		} else if(vartype == 2 || vartype == 3 ){
			compiled_variable_allocation_start += vararrlen*4;
		}
		compiled_variable_allocation_start &= 0xffFFff;
		compiled_variable_array_lengths[n_compiled_variables] = vararrlen;
		n_compiled_variables++;
	} else {
		puts("<COMPILED LINE SYNTAX ERROR>Unknown/unparseable compile line.");
		puts("Line:");
		puts(line_in);
		exit(1);
	}
	free(line_in);
	return line_out;
}
static int disassembler(char* fname, unsigned long location, unsigned long SISA16_DISASSEMBLER_MAX_HALTS){
	/*Disassemble for exactly 64k.*/
	unsigned long n_halts = 0;
	FILE* f; unsigned long i = location & 0xffFFff;
	f = fopen(fname, "rb");
	if(!f){
		puts("//ERROR: Could not open file to disassemble.");
		exit(1);
	}
	location &= 0xffFFff;
	fseek(f, location, SEEK_SET);
	printf("\nsection 0x%lx;\n", location);
	for(i = location; i < 0x1000000;){
		unsigned char opcode;
		if(i >= 0x1000000){
			puts("//Disassembly reached end of memory.");
			goto end;
		}else if((i & 0xffFF) == 0){
			puts("//<Region Boundary>");
		}
		if((unsigned long)ftell(f) != i){
			puts("//<Disassembly could not proceed, reason: end of file>");
			goto end;
		}
		opcode = fgetc(f);i++;
		if(opcode == 0 || opcode >= n_insns) n_halts++;
		else n_halts = 0;
		if(
			opcode < n_insns &&  /*Valid opcode*/
			(insns_numargs[opcode] > 0) && /*With arguments*/
			(((i-1)+insns_numargs[opcode])>>16) != ((i-1)>>16)
		){
			printf("\n//(E_BAD_BINARY)\n//op is '%s' which has %u args but crosses the region boundary.\n//This is most likely the result of interpreting embedded data as code.\n",insns[opcode],insns_numargs[opcode]);
		}
		if(opcode >= n_insns){
			puts("//!!!Illegal!!! opcode, equivalent to a NOP:");
			printf("%-9s 0x%x                ;//0x%06lx  :(E_ILLEGAL_OPCODE)\n","bytes", (unsigned int)opcode, i-1);
			continue;
		}else{
			unsigned long arg_i = 0;
			unsigned char bad_flag = 0;
			unsigned long required_spaces = 20;
			unsigned long opcode_i = i-1;
			printf("%-9s ",insns[opcode]);
			for(arg_i = 0; arg_i < insns_numargs[opcode]; arg_i++){
				unsigned char thechar;
				if(  ((i & 0xffFF) == 0) || (i >= 0x1000000)){
					bad_flag = 1;
				}
				if(arg_i > 0) {
					printf(",");
					required_spaces -= 5;
				}else{
					required_spaces -= 4;
				}
				if(feof(f) || (unsigned long)ftell(f) != i){
					puts("\n//Disassembly halted here. Reason: Missing opcode arguments.");
					exit(0);
				}
				thechar = fgetc(f);
				printf("0x%02x",(unsigned int)thechar);i++;
			}
			if(required_spaces < 50)
			{unsigned long li; for(li=0;li<required_spaces;li++){
				printf(" ");
			}}
			printf(";//0x%06lx  :", opcode_i);
			if(bad_flag) printf("(E_WONT_RUN)");
			if(streq(insns[opcode], "farret")){
				puts("~~~~~~~~~~End of Procedure");
			}else if(streq(insns[opcode], "ret")){
				puts("~~~~~~~~~~End of Region-Local Procedure");
			} else if(opcode == 0 && n_halts == 1){
				puts("~~~~~~~~~~End of Control Flow");
			} else if(streq(insns[opcode], "jmp")){
				 puts("~~~~~~~~~~Unconditional Jump");
			}else if(streq(insns[opcode], "jmpifeq")){
				puts("~~~~~~~~~~Conditional Jump");
			}else if(streq(insns[opcode], "cbrx0")
					||streq(insns[opcode], "carx0")	){
				puts("~~~~~~~~~~Likely: Far memory array access through RX0");
			}else if(streq(insns[opcode], "jmpifneq")){
				puts("~~~~~~~~~~Conditional Jump");
			}else if(streq(insns[opcode], "sc")){
				puts("~~~~~~~~~~Likely: jump target");
			}else if(streq(insns[opcode], "crx0")
					||streq(insns[opcode], "crx1")
					||streq(insns[opcode], "crx2")
					||streq(insns[opcode], "crx3")){
				puts("~~~~~~~~~~Likely: Computed Jump through RX register.");
			}else if(streq(insns[opcode], "ca")){
				puts("~~~~~~~~~~Maybe: Computed Jump through register A");
			}else if(streq(insns[opcode], "cb")){
				puts("~~~~~~~~~~Maybe: Computed Jump through register B");
			}else if(streq(insns[opcode], "call")){
				puts("~~~~~~~~~~Region-Local Procedure Call");
			}else if(streq(insns[opcode], "farcall")){
				puts("~~~~~~~~~~Procedure Call");
			}else if(streq(insns[opcode], "lfarpc")){
				puts("~~~~~~~~~~Region Jump");
			}else if(streq(insns[opcode], "farjmprx0")){
				puts("~~~~~~~~~~Far Jump");
			}else if( streq(insns[opcode], "farllda")
					||streq(insns[opcode], "farlldb")
					||streq(insns[opcode], "farldc")
					||streq(insns[opcode], "farldrx0")
					||streq(insns[opcode], "farldrx1")
					||streq(insns[opcode], "farldrx2")
					||streq(insns[opcode], "farldrx3")
					||streq(insns[opcode], "lda")
					||streq(insns[opcode], "ldb")
					||streq(insns[opcode], "ldc")
					||streq(insns[opcode], "llda")
					||streq(insns[opcode], "lldb")
			){
				puts("~~~~~~~~~~Likely: Loading from fixed position variable.");
			}else if( streq(insns[opcode], "farstla")
					||streq(insns[opcode], "farstlb")
					||streq(insns[opcode], "farstc")
					||streq(insns[opcode], "farstrx0")
					||streq(insns[opcode], "farstrx1")
					||streq(insns[opcode], "farstrx2")
					||streq(insns[opcode], "farstrx3")
					||streq(insns[opcode], "sta")
					||streq(insns[opcode], "stb")
					||streq(insns[opcode], "stc")
					||streq(insns[opcode], "stla")
					||streq(insns[opcode], "stlb")
			){
				puts("~~~~~~~~~~Likely: Storing to fixed position variable.");
			}else if(streq(insns[opcode], "cpc")){
				puts("~~~~~~~~~~Likely: loop top or future jump target.");
			}else {printf("\n");}
		}
		if(n_halts > SISA16_DISASSEMBLER_MAX_HALTS){
			puts("\n//Reached Halt/Invalid Opcode Limit. Disassembly finished.");
			goto end;
		}
	}
	puts("\n//Reached End. Disassembly Over.\n");
	end:
	fclose(f);
	return 0;
}

char temporary_name[512] = "tmpsisa_XXXXXX";
int main(int argc, char** argv){
	FILE* infile,*ofile; char* metaproc;
	unsigned long include_level = 0;
	const unsigned long nbuiltin_macros = 7; 
	const unsigned long maxmacrocalls = 0x100000;
	unsigned long line_num = 0; 
	variable_names[0] = "@";
	variable_expansions[0] = "";
	variable_names[1] = "$";
	variable_expansions[1] = variable_expansions[0];
	variable_names[2] = "%";
	variable_expansions[2] = variable_expansions[0];
	/*Macros to remove whitespace- this assembler works without using any whitespace at all.*/
	variable_names[3] = "\t";
	variable_expansions[3] = variable_expansions[0];
	variable_names[4] = " ";
	variable_expansions[4] = variable_expansions[0];
	variable_names[5] = "  ";
	variable_expansions[5] = variable_expansions[0];
	variable_names[6] = ";;";
	variable_expansions[6] = ";";
	/*Do math.*/
	/*Assembly-time directives.*/
	nmacros = 7;

	{int i;for(i = 2; i < argc; i++)
	{
		if(strprefix("-o",argv[i-1]))outfilename = argv[i];
		if(strprefix("-i",argv[i-1]))infilename = argv[i];
		if(strprefix("-run",argv[i-1])){
			/*FILE* f; unsigned long which = 0;*/
			infilename = argv[i];
			run_sisa16 = 1;
			clear_output = 1;
			outfilename = NULL;

		}
		if(strprefix("--disassemble",argv[i-2]) || strprefix("-dis",argv[i-2]) ){
			unsigned long loc;
			puts("//Beginning Disassembly");
			loc = strtoul(argv[i],0,0) & 0xffFFff;
			disassembler(argv[i-1], loc, 3);
			exit(0);
		}
		if(strprefix("--full-disassemble",argv[i-2]) || strprefix("-fdis",argv[i-2]) ){
			unsigned long loc;
			puts("//Beginning Disassembly");
			loc = strtoul(argv[i],0,0) & 0xffFFff;
			disassembler(argv[i-1], loc, 0x1000001);
			exit(0);
		}
	}}
	{int i;for(i = 1; i < argc; i++)
	{
		if(strprefix("-E",argv[i])) {
			quit_after_macros = 1;
			ASM_PUTS("#Contents of file post macro expansion are as follows:");
		}
		if(strprefix("-C",argv[i])){
			puts("SISA-16 Assembler and Emulator written by D[MHS]W for the Public Domain");
			puts("This program is Free Software that respects your freedom, you may trade it as you wish.");
			puts("Developer's original repository: https://github.com/gek169/Simple_ISA.git");
			puts("Please submit bug reports and... leave a star if you like the project! Every issue will be read.");
			puts("Programmer Documentation for this virtual machine is provided in the provided manpage sisa16_asm.1");
			puts("~~COMPILETIME ENVIRONMENT INFORMATION~~");
#if defined(NO_FP)
			puts("Floating point unit was disabled during compilation. Float ops generate error code 8.");
#else
			puts("Floating point unit was enabled during compilation. You may use fltadd, fltsub, fltmul, fltdiv, and fltcmp");
			if(sizeof(float) != 4) puts("Floats are an incorrect size. You may not use the floating point unit. Disable the floating point unit.");
#endif
#if defined(NO_SIGNED_DIV)
			puts("32 bit signed integer division instructions were disabled during compilation, they generate error code 10.");
			puts("You can manually implement signed twos-complement integer division.");
#else
			puts("32 bit signed integer division instructions were enabled during compilation. Don't divide by zero!");
#endif

			printf("Size of u is %u, it should be 1, any other result is UB.\n", (unsigned int)sizeof(u));
			printf("Size of U is %u, it should be 2, any other result is UB.\n", (unsigned int)sizeof(U));
			printf("Size of UU is %u, it should be 4, any other result is UB.\n", (unsigned int)sizeof(UU));
			printf("Size of SUU is %u, it should be 4, any other result is UB.\n", (unsigned int)sizeof(SUU));

#ifdef __STDC_IEC_559__
#if __STDC_IEC_559__ == 0
		puts("Floating point compatibility of the environment is specifically not guaranteed.");
#else
		puts("Floating point compatibility of the environment is specifically guaranteed.");
#endif

#else
		puts("the STDC_IEC_559 define is not defined. Standard behavior is not guaranteed.");
#endif
		if(sizeof(void*) == 2) puts("Compiled for a... 16 bit architecture? This is going to be interesting...");
		if(sizeof(void*) == 4) puts("Compiled for a 32 bit architecture.");
		if(sizeof(void*) == 8) puts("Compiled for a 64 bit architecture.");
#if defined(__i386__) || defined(_M_X86) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__IA32__) || defined(__INTEL__) || defined(__386)
		puts("Compiled for x86");
#endif
#if defined(__ia64__) || defined(_IA64) || defined(__ia64) || defined(_M_IA64) || defined(__itanium__)
		puts("Compiled for Itanium.")
#endif
#if defined(__m68k__) || defined(M68000) || defined(__MC68K__)
		puts("Compiled for the M68k");
#endif
#if defined(__mips__) || defined(__mips) || defined(_MIPS_ISA) || defined(__MIPS__)
		puts("Compiled for MIPS.");
#endif
#if defined(__powerpc__) || defined(__powerpc) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC) || defined(_ARCH_PPC) || defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || defined(__ppc)
		puts("Compiled for PowerPC");
#endif
#if defined(__THW_RS6000)
		puts("Compiled for RS/6000");
#endif
#if defined(__sparc__) || defined(__sparc) || defined(__sparc_v8__) || defined(__sparc_v9__) || defined(__sparcv8) || defined(__sparcv9)
		puts("Compiled for sparc.");
#endif
#if defined(__sh__)
		puts("Compiled for SuperH.");
#endif
#if defined(__370__) || defined(__s390__) || defined(__s390x__) || defined(__zarch__) || defined(__SYSC_ZARCH__)
		puts("Compiled for SystemZ");
#endif
#if defined(_TMS320C2XX) || defined(_TMS320C5X) || defined(_TMS320C6X)
		puts("Compiled for TMS320");
#endif
#if defined(__TMS470__)
		puts("Compiled for TMS470");
#endif
#if defined(__SISA16__)
		puts("Compiled for... This architecture!\nIf you're reading this from the source code, let this be a challenge to you:"
		"\nWrite a C compiler for SISA16 and tell me about it on Github. Then, show me some screenshots.\nI'll give you a special prize.");
		puts("The first person to get sisa16_asm to be self-hosting will win something. If that's you, Submit an issue! https://github.com/gek169/Simple_ISA");
#endif

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
		puts("Compiled for x86_64");
#endif
#if defined(__alpha__) || defined(_M_ALPHA)
		puts("Compiled for Alpha.");
#endif
#if defined(__arm__) || defined(_ARM) || defined(_M_ARM)|| defined(_M_ARMT) || defined(__thumb__) || defined(__TARGET_ARCH_ARM)|| defined(__TARGET_ARCH_THUMB)
		puts("Compiled for arm.");
#endif
#if defined(__aarch64__)
		puts("Compiled for aarch64");
#endif
#if defined(__bfin)
		puts("Compiled for blackfin.");
#endif
#if defined(__convex__)
		puts("Compiled for convex.");
#endif
#if defined(__epiphany__)
		puts("Compiled for epiphany.");
#endif
#if defined(__hppa__) || defined(__HPPA__)
		puts("Compiled for hppa.");
#endif

		{
			volatile UU a;
			a = 1;
			if(*((unsigned char*) &a)){
				puts("Executing on a Little-Endian Architecture.");
			} else {
				if(*(((unsigned char*)(&a))+2))
					puts("Executing on a PDP-Endian Architecture... That's pretty strange.");
				else if(*(((unsigned char*)(&a))+1))
					puts("Executing on an unknown Middle-Endian Architecture... How Unusual.");
				else
					puts("Executing on a Big-Endian Architecture.");
			}
		}
#if defined(WIN32) || defined(_WIN32)
			puts("Compiled for Macrohard Doors.");
#endif
#if defined(__unix__)
			puts("Targetting some Nix-like OS.");	
#endif
#if defined(linux) || defined(__linux__) || defined(__linux) || defined (_linux) || defined(_LINUX) || defined(__LINUX__)
			puts("Targetting Linux. Free Software Is Freedom.");
#endif
#if defined(__MINGW32__)
			puts("Compiler self-identifying as MINGW.");
#endif
#if defined(__TenDRA__)
			puts("Compiled with TenDRA. Gotta say, that's a cool name for a compiler.");
			return 0;
#endif

#ifdef __INTEL_LLVM_COMPILER
			puts("Compiled with Intel LLVM. Duopoly Inside.");
			return 0;
#endif

#ifdef __cilk
			puts("Compiled with Cilk. Duopoly inside.");
			return 0;
#endif


#ifdef __TINYC__
			puts("Compiled with TinyCC. All Respects to F. Bellard and Crew~~ Try TinyGL! https://github.com/C-Chads/tinygl/");
			return 0;
#endif

#ifdef _MSVC_VER
			puts("Gah! You didn't really compile my beautiful software with that disgusting compiler?");
			puts("MSVC is the worst C compiler on earth. No, not just because Microsoft wrote it. They make *some* good products.");
			puts("The bytecode is horrible and the compiler is uncooperative and buggy.");
			puts("Herb Sutter can go suck an egg! He's trying to kill C++.");
			return 0;
#endif

#ifdef __SDCC
			puts("Compiled with SDCC. Please leave feedback on github about your experiences compiling this with SDCC, I'd like to know.");
			return 0;
#endif

#ifdef __clang__
			puts("Compiled with Clang. In my testing, GCC compiles this project faster *and* produces faster x86_64 code.");
			return 0;
#endif

#ifdef __GNUC__
			puts("Compiled with GCC. Free Software Is Freedom.");
			return 0;
#endif
			puts("The C compiler does not expose itself to be one of the ones recognized by this program. Please tell me on Github what you used.");
			return 0;
		}
		if(strprefix("-DBG",argv[i])) {
			debugging = 1;
			ASM_PUTS("<ASM> Debugging.\n");
		}
		if(strprefix("-pl",argv[i])){
			printlines = 1;
			ASM_PUTS("<ASM> Printing lines.");
		}
		if(
			strprefix("-h",argv[i]) ||
			strprefix("-v",argv[i]) ||
			strprefix("--help",argv[i]) ||
			strprefix("--version",argv[i])
		){
			if(!clear_output)printf("Usage: %s [ARGS...]\n", argv[0]);
			ASM_PUTS("Optional argument: -i: specify input file.");
			ASM_PUTS("Optional argument: -dis, --disassemble: disassemble a file, requires an input file and a location to start disassembling.");
			ASM_PUTS("Optional argument: -fdis, --full-disassemble: disassemble a file, without ending on halts/illegal opcodes. Same semantics as -dis");
			ASM_PUTS("Optional argument: -o: specify output file. If not specified it is: outsisa16.bin");
			ASM_PUTS("Optional argument: -DBG: debug the assembler. do not specify an infile if you want to use stdin.");
			ASM_PUTS("Optional argument: -E: Print macro expansion only do not write to file");
			ASM_PUTS("Optional argument: -pl: Print lines");
			ASM_PUTS("Optional argument: -C: display compiletime environment information (What C compiler you used) as well as Author.");
			ASM_PUTS("Optional argument: -run: Build and Execute assembly file, like -i. Compatible with shebangs on *nix machines.\nTry adding `#!/usr/bin/sisa16_asm -run` to the start of your programs!");
			ASM_PUTS("Optional argument: -v, -h, --help, --version: This printout.");
			ASM_PUTS("\n\nSISA-16 Macro Assembler, Disassembler, and Emulator in Pure Glorious ANSI/ISO C90, Version 1.6");
			ASM_PUTS("\"Let all that you do be done with love\"");
			ASM_PUTS("Authored by DMHSW for the Public Domain. Enjoy.\n\n");
			return 0;
		}
	}}
	if(debugging) infile=stdin;

	if(infilename){
		if(debugging) if(!clear_output)printf("\nReading from a file...\n");
			infile = fopen(infilename, "rb");
		if(!infile) {
			if(!clear_output)printf("\nUNABLE TO OPEN INPUT FILE %s!!!\n", infilename);
			return 1;
		}
	} else {
		if(debugging) if(!clear_output)printf("\nReading from stdin...");
		if(!debugging) {
			puts("\n<ASM ERROR> May not use reading from stdin when not in debugging mode.\n");
			puts("\n<ASM> Assembler Aborted.");
			return 1;
		}
	}
	ofile = NULL;
	if(!quit_after_macros && !run_sisa16){
		ofile=fopen(outfilename, "wb");
	}
	if(!run_sisa16)
		if(!ofile && !quit_after_macros){
			if(!clear_output)
				printf("\nUNABLE TO OPEN OUTPUT FILE %s!!!\n", outfilename);
			return 1;
		}
	/*Second pass to allow goto labels*/
	for(npasses = 0; npasses < 2; npasses++, fseek(infile, 0, SEEK_SET), outputcounter=0)
	while(1){
		char was_macro = 0;	unsigned long nmacrocalls = 0;
		if(!infile) {
			puts("<ASM INTERNAL ERROR> infile is null? This should never happen.");
		}
		if(feof(infile)){
			/*try popping from the fstack*/
			if(include_level > 0){
				fclose(infile); infile = NULL;
				include_level -= 1;
				infile = fstack[include_level];
				continue;
			}
			/*else, break. End of pass.*/
			break;
		}
		if(debugging) if(!clear_output)printf("\nEnter a line...\n");
		line = read_until_terminator_alloced(infile, &linesize, '\n', 1);
		/*if this line ends in a backslash...*/
		if(!line) {
			puts("<ASM COMPILATION ERROR> cannot retrieve line.");
			goto error;
		}
		while(strprefix(" ",line) || strprefix("\t",line)){ /*Remove preceding whitespace... we do this twice, actually...*/
			char* line_old = line;
			line = strcatalloc(line+1,"");
			free(line_old);
		}
		while(!feof(infile) && strlen(line) > 0 && !strprefix("!",line) && !strprefix("//",line) && !strprefix("#",line) && line[strlen(line)-1] == '\\'){char* line_temp;
			line[strlen(line)-1] = '\0';
			line_temp = read_until_terminator_alloced(infile, &linesize, '\n', 1);
			line = strcatallocfb(line,line_temp);
			linesize = strlen(line);
		}
		line_copy = strcatalloc(line,"");line_num++;

		
		if(strprefix("#",line)) goto end;
		if(strprefix("//",line)) goto end;
		if(strprefix("!",line)) {unsigned long i;
			/*We are writing out individual bytes for the rest of the line.*/
			if(debugging)
				if(!clear_output)printf("Detected Character Literal on Line:\n%s\n", line_copy);
			if(printlines)ASM_PUTS(line);
			if(!quit_after_macros)
				for(i = 1; i < strlen(line);i++)
					fputbyte(line[i], ofile);
			goto end;
		}
		if(strprefix("ASM_header ", line)){
			FILE* tmp; char* metaproc;
			const char* env_sisa16bin = getenv("SISA16BIN");
			const char* env_home = getenv("HOME");
			metaproc = line + strlen("ASM_header ");
			if(include_level >= ASM_MAX_INCLUDE_LEVEL){
				puts("<ASM COMPILATION ERROR> Include level maximum reached.");
				goto error;
			}
			tmp = fopen(metaproc, "r");
			if(!tmp && env_sisa16bin &&
				(
					env_sisa16bin[strlen(env_sisa16bin)-1] == '/' ||
					env_sisa16bin[strlen(env_sisa16bin)-1] == '\\'
				)
			) 
			{
				char* bruh = strcatalloc(env_sisa16bin, metaproc);
				tmp = fopen(bruh, "r");
				free(bruh);
			}
			if(!tmp && env_sisa16bin &&
				(
					env_sisa16bin[strlen(env_sisa16bin)-1] == '/' ||
					env_sisa16bin[strlen(env_sisa16bin)-1] == '\\'
				)
			) 
			{
				char* bruh = strcatallocf1(strcatalloc(env_sisa16bin,"/"), metaproc);
				tmp = fopen(bruh, "r");
				free(bruh);
			}
			if(!tmp) {
				char* bruh = strcatalloc("/usr/include/sisa16/", metaproc);
				tmp = fopen(bruh, "r");
				free(bruh);
			}
			if(!tmp && env_home) {
				char* bruh = NULL;
				if(env_home[strlen(env_home)-1] == '/')
					bruh = strcatallocf1(strcatalloc(env_home,"sisa16/"), metaproc);
				else
					bruh = strcatallocf1(strcatalloc(env_home,"/sisa16/"), metaproc);
				tmp = fopen(bruh, "r");
				free(bruh);
			}
			if(!tmp) { char* bruh = strcatalloc("C:\\SISA16\\", metaproc);
				tmp = fopen(bruh, "r");
				free(bruh);
			}
			if(!tmp) {
				printf("<ASM COMPILATION ERROR> Unknown/unreachable header file %s\n", metaproc); 
				goto error;
			}
			fstack[include_level] = infile;
			include_level++;
			infile = tmp;
			goto end;
		}
		if(strprefix("ASM_data_include ", line)){
			/*data include! format is
			[newline]asm_data_include filename
			*/
			FILE* tmp; char* metaproc; unsigned long len;
			metaproc = line + strlen("ASM_data_include ");
			tmp = fopen(metaproc, "rb");
			if(!tmp) {
				char* bruh = strcatalloc("/usr/include/sisa16/", metaproc);
				tmp = fopen(bruh, "rb");
				free(bruh);
			}
			if(!tmp) {
				char* bruh = strcatalloc("~/sisa16/", metaproc);
				tmp = fopen(bruh, "rb");
				free(bruh);
			}
			if(!tmp) { char* bruh = strcatalloc("C:\\SISA16\\", metaproc);
				tmp = fopen(bruh, "rb");
				free(bruh);
			}
			if(!tmp) {
				printf("<ASM COMPILATION ERROR> Unknown/unreachable data file %s\n", metaproc); 
				goto error;
			}
			fseek(tmp, 0, SEEK_END);
			len = ftell(tmp);
			if(len > 0x1000000) {
				printf("<ASM COMPILATION ERROR> data file %s larger than entire address space.\n", metaproc); 
				goto error;
			}
			if(len == 0){
				printf("<ASM COMPILATION ERROR> data file %s is empty.\n", metaproc); 
				goto error;
			}
			fseek(tmp, 0, SEEK_SET);
			for(;len>0;len--)fputbyte(fgetc(tmp), ofile);
			fclose(tmp);
			if(printlines)ASM_PUTS(line);
			goto end;
		}
		/*Step 0: PRE-PRE PROCESSING. Yes, this is a thing.*/
		pre_pre_processing:

		if(nmacrocalls > maxmacrocalls){
			printf("<ASM COMPILATION ERROR> the recursion limit for macro calls has been reached.Line:\n%s\n", line_copy);
			goto error;
		}
		nmacrocalls++;
		while(strprefix(" ",line) || strprefix("\t",line)){ /*Remove preceding whitespace.*/
			char* line_old = line;
			line = strcatalloc(line+1,"");
			free(line_old);
		}
		if(strprefix("ASM_COMPILE", line)){
			puts("<ASM UNFINISHED FEATURE ERROR> Unfinished feature. You may not use ASM_COMPILE.");
			exit(1);
			line = compile_line(line);
			nmacrocalls = 0;
			goto pre_pre_processing;
		}
		if(strprefix("ASM_COPYRIGHT", line)){
			ASM_PUTS("SISA-16 Assembler,(C)David M.H.S. Webster 2021\navailable to you under the Creative Commons Zero license.");
			goto end;
		}
		if(strprefix("asm_help", line) || strprefix("ASM_HELP", line)){
			ASM_PUTS("For help, See: man sisa16_asm");
			goto end;
		}

		else if ((!strprefix("VAR#", line)) /*Do not parse asm_pleq's inside of a VAR# line.*/
					&& (strfind(line, "asm_pleq#") != -1 || strfind(line, "asm_muleq#") != -1) /*We have found one!*/
					){ /*SYNTAX: asm_pleq#variable_name#variable_name# or asm_muleq*/
			char expansion[1024]; long val1; long val2; long loc_middle_pound = -1; long len_to_replace = 0;
			char* proc; char* line_old; char var2_is_variable=0;
			char* varname1 = NULL; 
			char* varname2 = NULL; char* before; char* after;
			char* command_name; long loc_pleq=-1, loc_muleq=-1, loc = -1; 
			long loc_call = -1;
			long var1ind = -1;
			long var2ind = -1;
			unsigned long variable_i;
			long loc_end_pound = -1;
			/*CODE SECTION*/
			line_old = line;
			/*First we have to figure out what this even is. Is this a pleq command? Muleq?
			And, is there a call we should evaluate first? */
			loc_pleq = strfind(line, "asm_pleq#");
			loc_muleq = strfind(line, "asm_muleq#");
			loc_call = strfind(line, "asm_call#");
			if(loc_pleq != -1 && loc_muleq == -1)
				{loc = loc_pleq;command_name = "asm_pleq#";}
			else if(loc_pleq == -1 && loc_muleq != -1)
				{loc = loc_muleq;command_name = "asm_muleq#";}
			else if(loc_pleq < loc_muleq)
				{loc = loc_pleq;command_name = "asm_pleq#";}
			else if(loc_muleq < loc_pleq)
				{loc = loc_muleq;command_name = "asm_muleq#";}
			else {
				printf("<ASM INTERNAL ERROR> Faulty math statement recognition. Line:\n%s\n", line_copy);
				goto error;
			}
			if(loc_pleq == -1 && loc_muleq == -1){
				printf("<ASM INTERNAL ERROR> Faulty math statement recognition. Line:\n%s\n", line_copy);
				goto error;
			}
			len_to_replace = strlen(command_name);
			/*If there is a call before us, we must process it first.*/
			if(loc_call != -1 && 
				loc_call < loc) 
			{
				/*if(!clear_output)printf("\nDeferring to a call statement before us. Line internally:\n%s\nUs:\n%s\nCall:\n%s\n", line, line + loc, line + loc_call);*/
				goto process_asm_call;
			} /*We have to evaluate it first.*/
			/*Sequence break before this point- do not allow to continue.*/
			if(strfind(line, "|") != -1 &&
				strfind(line, "|") < loc)
				goto process_asm_call;
			proc = line_old + loc + strlen(command_name);
			before = str_null_terminated_alloc(line_old, loc);
			if(proc[0] != '\\'){
				printf("<ASM SYNTAX ERROR> %s cannot reference a variable. It is not escaped. Line:\n%s\n", command_name, line_copy);
				goto error;
			}
			proc++;len_to_replace++;
			if(proc[0] == 0){
				printf("<ASM SYNTAX ERROR> %s is at the end of the line. Line:\n%s\n",command_name, line_copy);
				goto error;
			}
			
			loc_middle_pound = strfind(proc,"#");
			if(loc_middle_pound < 1){
				printf("<ASM SYNTAX ERROR> %s lacks second pound. Line:\n%s\n",command_name, line_copy);
				goto error;
			}
			
			varname1 = str_null_terminated_alloc(proc, loc_middle_pound);
			proc += loc_middle_pound+1;
			len_to_replace += loc_middle_pound+1;
			loc_end_pound = strfind(proc,"#");
			if(loc_middle_pound < 1){
				printf("<ASM SYNTAX ERROR> %s lacks end pound. Line:\n%s\n", command_name, line_copy);
				goto error;
			}
			if(proc[0] == '\\'){
				var2_is_variable = 1;
				proc++; len_to_replace++;
				loc_end_pound--;
			}
			varname2 = str_null_terminated_alloc(proc, loc_end_pound);
			len_to_replace++; len_to_replace+=loc_end_pound;
			if(npasses == 1 && debugging)
			if(!clear_output)printf("\n%s invoked with \'%s\' and \'%s\'. the second is %s \n",command_name, varname1, varname2,
				var2_is_variable?"a variable.":"an integer literal.");
			for(variable_i = nbuiltin_macros; variable_i < nmacros; variable_i++){
				if(streq(varname1, variable_names[variable_i]))
					var1ind = variable_i;
				if(streq(varname2, variable_names[variable_i]))
					var2ind = variable_i;
			}
			if(var1ind == -1){
				printf("<ASM SYNTAX ERROR> asm_pleq cannot identify macro \'%s\'. Line:\n%s\n", varname1, line_copy);
				goto error;
			}
			variable_is_redefining_flag[var1ind] = 1;
			val1 = strtol(variable_expansions[var1ind], NULL, 0);
			if(var2_is_variable)
				val2 = strtol(variable_expansions[var2ind], NULL, 0);
			else
				val2 = strtol(varname2, NULL, 0);
			if(debugging)
			if(!clear_output)printf("val 1 is %ld, 2 is %ld\n", val1, val2);
			/*Free variable names.*/
			free(varname1);
			free(varname2);var2ind=-1;
			
			/*Do math.*/
			{	unsigned long uval2, uval1;
				memcpy(&uval1, &val1, sizeof(unsigned long));
				memcpy(&uval2, &val2, sizeof(unsigned long));
				if(streq(command_name, "asm_pleq#"))
					uval1+=uval2;
				else if(streq(command_name, "asm_muleq#"))
					uval1*=uval2;
				memcpy(&val1, &uval1, sizeof(unsigned long));
			}
			
			sprintf(expansion, "%ld", val1);
			
			if(variable_expansions[var1ind])
				free(variable_expansions[var1ind]);
			else {
				printf("<ASM INTERNAL ERROR> variable had a null expansion? Line:\n%s\n", line_copy);
				goto error;
			}
			
			variable_expansions[var1ind] = strcatalloc(expansion, "");
			/*TODO: handle before, after, and line expansion stuff.*/
			after = strcatalloc(line+loc+len_to_replace, "");
			if(debugging)
				if(!clear_output)printf("before: \n%s\n after: \n%s", before, after);
			line = strcatallocfb(
				before,
				after
			);
			free(line_old);
			goto pre_pre_processing;
		}
		process_asm_call:;
		if(
			(!strprefix("VAR#", line)) /*Do not parse asm_call's inside of a VAR# line.*/
			&& (strfind(line, "asm_call#") != -1) /*We have found one!*/
			&& !((strfind(line, "|") != -1) && strfind(line, "|") < strfind(line, "asm_call#")  ) /*No sequence point operator.*/
			){
			char* f, *before, *after, *content; long next_pound = -1; long found_macro_id=-1;
			unsigned long i = 0;char* macro_name=0;
			f = line + strfind(line, "asm_call#") + strlen("asm_call#");
			before = str_null_terminated_alloc(line, strfind(line, "asm_call#"));
			/*The string immediately after the first # is the macro to call.
			the arguments thereafter are arg1, arg2, arg3... */
			next_pound = strfind(f, "#");
			if(next_pound == -1) {
				printf("<ASM SYNTAX ERROR> macro call lacking any succeeding pounds.Line:\n%s\n", line_copy);
				goto error;
			}
			macro_name = str_null_terminated_alloc(f, next_pound);
			/*if(!clear_output)printf("\nMacro call attempt detected on macro \'%s\'", macro_name);*/
			{
				unsigned long macro_i;
				for(macro_i = 0; macro_i < nmacros; macro_i++)
					if(streq(macro_name, variable_names[macro_i])){found_macro_id=macro_i;break;}
				if(found_macro_id == -1){
					printf("<ASM SYNTAX ERROR> macro call on non-existent macro.Line:\n%s\n", line_copy);
					goto error;
				}
			}
			content = strcatalloc(variable_expansions[found_macro_id],"");
			if(debugging) if(!clear_output)printf("\nMacro call attempt on macro \'%s\'", macro_name);
			f+=next_pound+1;
			for(i=1;*f!='\0' && *f!='#';i++){
				unsigned long macro_i;
				char namebuf[64]; /*Holds name of argXXX*/
				/*Used for defining the macro.*/
				char* varname = NULL;
				unsigned long replacements = 0;
				const unsigned long max_repl = 0x10000;
				char* vardef = NULL;
				long overwriting = -1;/*if not negative, this means it's replacing something.*/
				/*Check to see if we even can define a macro.*/
				if(nmacros == 0xffff) {
					printf("<ASM ERROR>  Too many macros. Cannot define another one. Line:\n%s\n", line_copy); 
					goto error;
				}
				/*There is enough room for a macro.*/
				sprintf(namebuf, "_arg%lu_", i);
				varname = strcatalloc(namebuf, "");
				if(debugging)
					if(!clear_output)printf("\nReplacing %s...", varname);
				/*We need to see if this macro already exists.*/
				for(macro_i = 0; macro_i < nmacros; macro_i++)
					if(streq(varname, variable_names[macro_i]))
						overwriting = macro_i; /*It does!*/
				if(overwriting == -1) {
					overwriting = nmacros++;
				}

				/*get the definition*/
				next_pound = strfind(f, "#");
				if(next_pound == -1) {
					printf("<ASM SYNTAX ERROR> macro call lacking end first pound.Line:\n%s\n", line_copy);
					goto error;
				}
				vardef = str_null_terminated_alloc(f, next_pound);
				f+=next_pound+1;
				if(variable_names[overwriting]){free(variable_names[overwriting]);variable_names[overwriting] = NULL;}
				if(variable_expansions[overwriting]){free(variable_expansions[overwriting]);variable_expansions[overwriting] = NULL;}
				variable_names[overwriting] = varname;
				variable_expansions[overwriting] = vardef;
				if(debugging)
					if(!clear_output)printf("\nReplacing %s with %s", varname, vardef);
				/*replace _argXXX in content.*/
				while(strfind(content, varname)!= -1 && replacements < max_repl){
					char* content_before, *content_after;replacements++;
					if(debugging) ASM_PUTS("Found an instance of a required replacement!");
					content_before = str_null_terminated_alloc(content,strfind(content, varname));
					content_after = strcatalloc(content+strfind(content, varname)+strlen(varname),"");
					free(content); /**/
					content = strcatallocfb(
						content_before,
						strcatallocf2(vardef, content_after)
					);
				}
			}
			if(*f != '#'){
				printf("<ASM SYNTAX ERROR> macro call lacking end second pound.Line:\n%s\n", line_copy);
				goto error;
			}f++;
			/*Get the after.*/
			after = strcatalloc(f,"");
			if(line) free(line);
			line = strcatallocfb(
				before,
				strcatallocfb(content, after)
			);
			free(macro_name);
			if(debugging) if(!clear_output)printf("\nThe line is now \n%s\nas a result of a macro call!\n", line);
			goto pre_pre_processing; /*Allow recursive calling.*/
		}

		if(strlen(line) < 1) goto end; /*the line is empty*/
		if(strlen(line) == 1 && !isalpha(line[0])) goto end; /*Cannot possibly be a macro, it's the end of file thing.*/
		if(!isalpha(line[0]) && line[0] != ' ' && line[0] != ';' && line[0] != '\t'
			&& line[0] != '\\' && line[0] != '|'){
			if(!clear_output)printf("<ASM WARNING> Ignoring line beginning with illegal character... Line:\n%s\n", line_copy);
			goto end;
		}
		/*Step 1: Expand Macros on this line. This includes whitespace removal.*/
		/*Not performed on MACRO Lines.*/
		
		if(strfind(line,"VAR#")!= -1) was_macro=1; else was_macro = 0;
		{unsigned char have_expanded = 0; unsigned short iteration = 0; long i;
			do{
				have_expanded = 0;
				if(debugging){
					if(!clear_output)printf("\n~~Macro Expansion Stage~~, iteration %u\nLine:\n%s", iteration, line);
					if(was_macro)
						ASM_PUTS("\n~~~~~~~~~~~~~~~This is a macro line~~~~~~~~~~~~~~~\n");
				}
				{
				long loc_vbar = strfind(line, "|");
				if(loc_vbar == -1) loc_vbar = strlen(line);
				if(!was_macro)
					if( (strfind(line, "asm_call") != -1   && strfind(line, "asm_call") <loc_vbar) ||
						(strfind(line, "asm_pleq") != -1   && strfind(line, "asm_pleq") <loc_vbar) ||
						(strfind(line, "asm_muleq") != -1  && strfind(line, "asm_muleq")<loc_vbar)
						){
						goto pre_pre_processing; /*A macro has been expanded, resulting in an asm_call or math forming.*/
					}
				}
				for(i = (was_macro?nbuiltin_macros:nmacros)-1; i>=0; i--){ /*Only check builtin macros when writing a macro.*/
					char* line_old; long loc; long linesize; char found_longer_match;
					long len_to_replace; char* before;char* after;
					long j, loc_vbar = 0;;

					loc_vbar = strfind(line, "|");
					linesize = strlen(line);
					if((loc_vbar == -1)) loc_vbar = linesize;
					loc = strfind(line, variable_names[i]);
					if(loc == -1) continue;
					if(loc >= loc_vbar) continue; /*Respect the sequence operator.*/
					if(loc > 0 && *(line+loc-1) == '\\') continue;
					if(was_macro && i==2){
						continue; /*Do not parse the split directive inside of a macro.*/
					}
					if(was_macro && (i==3 || i==4)){
						continue; /*Do not parse the space or tab inside of a macro.*/
						/*Also, don't parse muleq or pleq*/
					}
					line_old = line;
					if(debugging)if(!clear_output)printf("\nDiscovered possible Macro \"%s\"!\n", variable_names[i]);
										/*Check to make sure that this isn't some other, longer macro.*/
					found_longer_match = 0;
					if(!was_macro)
					for(j = 0; j<(long)nmacros; j++){
						if(j == i) continue;
						if(strlen(variable_names[j]) > strlen(variable_names[i])){
							long checkme;
							checkme = strfind(line, variable_names[j]);
							if(checkme == -1) continue;
							/*Does this match intersect?*/
							if(
								(
									checkme+strlen(variable_names[j]) >= loc + strlen(variable_names[i]) &&
																	checkme <= loc
								) || (
									(checkme < (long)(loc + strlen(variable_names[i]))) &&
									(checkme >= loc)
								) || (
									(checkme+strlen(variable_names[j]) < loc + strlen(variable_names[i])) &&
									((long)(checkme+strlen(variable_names[j])) >= loc)
								)
							){
								found_longer_match = 1;
								break;
							}
						}
					}
					if(found_longer_match)
					{
						if(debugging){ASM_PUTS("Found longer Macro.");}
						continue;
					}
					/*We know the location of a macro to be expanded and it is at loc.*/
					/*This also quit conveniently defines the recursion limit for a macro.*/
					have_expanded = 1;
					
					len_to_replace = strlen(variable_names[i]);
					before = str_null_terminated_alloc(line_old, loc);
					if(i > 2) /*0,1,2 are special cases. 3,4,X are not.*/
						before = strcatallocf1(before, variable_expansions[i]);
					else if (i == 0){ /*SYNTAX: @+7+*/
						char expansion[1024];
						unsigned long addval = 0;
						/*We need to check if there is a plus sign immediately after the at sign. */
						if(strprefix("+",line_old+loc+1)){
							char* add_text = line_old+loc+2;
							/*Find the next plus*/
							long loc_eparen = strfind(line_old+loc+2,"+");
							if(loc_eparen == -1){
								printf("<ASM SYNTAX ERROR> @ with no ending plus. Line:\n%s\n", line_copy);
								goto error;
							}
							if(loc_eparen == 0){
								if(!clear_output)printf("<ASM WARNING> @ with empty add section. Line:\n%s\n", line_copy);
							}
							addval = strtoul(add_text,0,0);
							if(addval == 0 && npasses == 1)
								if(!clear_output)printf("<ASM WARNING> @ with add evaluating to zero. Line:\n%s\n", line_copy);
							if(addval)
							len_to_replace += (loc_eparen-len_to_replace+3);
						}
						addval += outputcounter;
						sprintf(expansion, "%lu", addval);
						expansion[1023] = '\0'; /*Just in case...*/
						before = strcatallocf1(before, expansion);
					} else if (i==1){
						char expansion[1024]; unsigned long addval;
						
						addval = 0;
						if(strprefix("+",line_old+loc+1)){char* add_text; long loc_eparen;
							add_text = line_old+loc+2;
							/*Find the next plus*/
							loc_eparen = strfind(line_old+loc+2,"+");
							if(loc_eparen == -1){
								printf("<ASM SYNTAX ERROR> $ with no ending plus. Line:\n%s\n", line_copy);
								goto error;
							}
							if(loc_eparen == 0){
								if(!clear_output)printf("<ASM WARNING> $ with empty add section. Line:\n%s\n", line_copy);
							}
							addval = strtoul(add_text,0,0);
							if(addval == 0 && npasses == 1)
								if(!clear_output)printf("<ASM WARNING> $ with add evaluating to zero. Line:\n%s\n", line_copy);
							len_to_replace += (loc_eparen-len_to_replace+3);
						}
						addval += outputcounter;
						
						/*snif(!clear_output)printf(expansion, 1023, "%lu,%lu", (unsigned long)(addval/256),(unsigned long)(addval&0xff));*/
						sprintf(expansion, "%lu,%lu", (unsigned long)(addval/256),(unsigned long)(addval&0xff));
						expansion[1023] = '\0'; /*Just in case...*/
						before = strcatallocf1(before, expansion);
					} else if (i==2){long loc_qmark;long loc_slash;long loc_ampersand;char do_32bit=0;long loc_eparen;char expansion[1024]; 
					unsigned long res; /*Split directive.*/
						/*Locate the next ending one*/
						if(strlen(line_old+loc) == 0){
							printf("<ASM SYNTAX ERROR> SPLIT (%%) is at end of line. Line:\n%s\n", line_copy);
							goto error;
						}
						loc_eparen = strfind(line_old+loc+1,"%");
						loc_slash = strfind(line_old+loc+1,"/");
						loc_qmark = strfind(line_old+loc+1,"?");
						loc_ampersand = strfind(line_old+loc+1,"&");
						if(loc_eparen == -1){
							printf("<ASM SYNTAX ERROR> SPLIT (%%) without ending %%. At location:\n%s\nLine:\n%s\n",line_old+loc,line_copy);
							goto error;
						}
						if(loc_eparen == 0){
							/*if(!clear_output)printf("<ASM WARNING> SPLIT (%%) is empty. At location:\n%s\nLine:\n%s\n",line_old+loc,line_copy);*/
						}
						if(loc_slash==0) do_32bit = 1;
						if(do_32bit == 0 && loc_qmark == 0){
							do_32bit = 2;
						} else if(do_32bit == 1 && loc_qmark != -1 && loc_qmark < loc_eparen){
							puts(
								"<ASM SYNTAX ERROR> Both a slash and a question mark are present in this split."
								"\nThe split is already 32 bit if you use question mark."
							);
							printf("\nAt location:\n%s\nLine:\n%s\n",line_old+loc,line_copy);
							exit(1);
						}
						if(loc_ampersand==0) do_32bit = 3;
						/*the character we were going to replace anyway, plus
						the length of the stuff inbetween, plus the */
						len_to_replace+=(loc_eparen-len_to_replace+2);
						
						if(do_32bit == 0)
							res = strtoul(line_old+loc+1, NULL, 0);
						else if(do_32bit == 1 || do_32bit == 3) /*Skip the forward slash or ampersand*/
							res = strtoul(line_old+loc+2, NULL, 0);
						else if(do_32bit == 2){
#if defined(NO_FP)
							puts("<ASM ENV ERROR> Floating point unit was disabled during compilation. You may not use floating point SPLIT directives.");
							exit(1);
#else
							float a;UU d1;
							if(sizeof(a) != 4){puts("<ASM ENV ERROR> Floating point environment INCOMPATIBLE.");exit(1);}
							a = atof(line_old+loc+2);
							memcpy(&d1, &a, 4);
							if(sizeof(UU) == 4)
								res = d1;
							else {
								puts("<ASM ENV ERROR> UU is not 32 bit, try toggling -DUSE_UNSIGNED_INT");
								exit(1);
							}
#endif
						}
						if(!do_32bit){
							if(res == 0  && npasses == 1 && line_old[loc+1] != '%' && line_old[loc+1] != '0')
								if(!clear_output)printf("<ASM WARNING> Unusual SPLIT (%%) evaluates to zero. Line:\n%s\nValue:\n%s\nInternal:\n%s\n", line_copy, line_old+loc+1, line_old);
						} else {
							
							if(res == 0  && npasses == 1 && line_old[loc+2] != '%' && line_old[loc+2] != '0')
								if(!clear_output)printf("<ASM WARNING> Unusual 32-bit SPLIT (%%) evaluates to zero. Line:\n%s\nValue:\n%s\nInternal:\n%s\n", line_copy, line_old+loc+1, line_old);
						}
						if(debugging) if(!clear_output)printf("\nSplitting value %lu\n", res);
						/*Write the upper and lower halves out, separated, to expansion.*/
						if(do_32bit == 0) {
							sprintf(expansion, "%u,%u", ((unsigned int)(res/256)&0xff),(unsigned int)(res&0xff));
						} else if(do_32bit == 1 || do_32bit == 2) {
							sprintf(expansion, "%u,%u,%u,%u", (unsigned int)((res/(256*256*256))&0xff),
																(unsigned int)((res/(0x10000))&0xff),
																(unsigned int)((res/(256))&0xff),
																(unsigned int)((res)&0xff)
																);
						} else if(do_32bit == 3) {
							sprintf(expansion, "%u,%u,%u",
									(unsigned int)((res/(0x10000))&0xff),
									(unsigned int)((res/(256))&0xff),
									(unsigned int)((res)&0xff)
									);
						} else {
							puts("<ASM INTERNAL ERROR> Invalid do_32bit mode in a split directive.");
							exit(1);
						}
						before = strcatallocf1(before, expansion);
					} 
					after = str_null_terminated_alloc(line_old+loc+len_to_replace, 
									linesize-loc-len_to_replace);
					line = strcatallocfb(before, after);
					free(line_old);
					break; /*we have expanded something.*/
				}
			}while(have_expanded && (iteration++ < 32768));
		}
		/*Check again to see if this is a comment line.*/
		if(strprefix("#",line)) goto end;
		if(strprefix("//",line)) goto end;
		if(strprefix("!",line) || strfind(line, "!") != -1) {
			printf("<ASM SYNTAX ERROR> Invalid character literal.");
			printf("Character literals must be on their own line with no preceding whitespace.");
			printf("\nLine:\n%s\n", line_copy);
			goto error;
		}
		
		/*Step 2: Check to see if this is a macro*/
		if(debugging){
			if(!clear_output)printf("\n~~Is this a macro?~~\n");
		}

		if(strprefix("VAR",line)){ long loc_pound, loc_pound2; char* macro_name;char is_overwriting; /*It's show time!*/
			unsigned short index;
			if(debugging){
				if(!clear_output)printf("\nThis is a macro!\n");
			}
			was_macro = 1;
			if(nmacros == 0xffff) {
				printf("<ASM ERROR>  Too many macros. Cannot define another one. Line:\n%s\n", line_copy); 
				goto error;
			}
			loc_pound = strfind(line, "#");
			loc_pound2 = 0;
			if(loc_pound == -1) {
				printf("<ASM SYNTAX ERROR>  missing leading # in macro declaration. Line:\n%s\n", line_copy); 
				goto error;
			}
			/*We have a macro.*/
			macro_name = line + loc_pound;
			if(debugging){
				if(!clear_output)printf("\nMacro Name Pre-Allocation is identified as %s\n", macro_name);
			}
			if(strlen(macro_name) == 1){
				printf("\n<ASM SYNTAX ERROR> missing second # in macro declaration. Line:\n%s\n", line_copy); 
				goto error;
			}
			macro_name += 1; loc_pound += 1; /*We use these again.*/
			loc_pound2 = strfind(macro_name, "#");
			if(loc_pound2 == -1) {
				printf("\n<ASM SYNTAX ERROR> missing second # in macro declaration. Line:\n%s\n", line_copy); 
				goto error;
			}
			macro_name = str_null_terminated_alloc(macro_name, loc_pound2);
			if(debugging){
				if(!clear_output)printf("\nMacro Name is identified as %s\n", macro_name);
			}
			if(strlen(line + loc_pound + loc_pound2) < 2){
				printf("<ASM SYNTAX ERROR> macro without a definition. Line:\n%s\n", line_copy);
				goto error;
			}
			loc_pound2++;
			/*Search to see if we've already defined this macro*/
			is_overwriting = 0;
			index = 0;
			{unsigned short i;for(i = 0; i < n_insns; i++){
				if( (strfind(insns[i],macro_name)>-1) ||
					(strfind(macro_name, insns[i])==0) 
				){
					printf("<ASM SYNTAX ERROR> This macro would prevent instruction %s from being used:\n%s\n", insns[i], line_copy);
					goto error;	
				}
			}}
			/*
				Prevent macros from being defined which contain other 
			*/
			/*
				Check and make sure this is not a reserved name
			*/
			if(
				strfind(macro_name, "!") != -1 ||
				strfind(macro_name, "?") != -1 ||
				strfind(macro_name, "[") != -1 ||
				strfind(macro_name, "]") != -1 ||
				strfind(macro_name, "\\") != -1 ||
				strfind(macro_name, "//") != -1 ||
				strfind(macro_name, "/") != -1 ||
				strfind(macro_name, " ") != -1 ||
				strfind(macro_name, "\t") != -1 ||
				strfind(macro_name, ";") != -1 ||
				strfind(macro_name, "|") != -1 ||
				strfind(macro_name, "#") != -1 ||
				strfind(macro_name, "$") != -1 ||
				strfind(macro_name, "&") != -1 ||
				strfind(macro_name, "@") != -1 ||
				strfind(macro_name, "%") != -1 ||
				strfind(macro_name, "bytes") != -1 ||
				strfind(macro_name, "shorts") != -1 ||
				strfind(macro_name, "asm_") != -1 ||
				strfind(macro_name, "ASM_") != -1 ||
				strfind(macro_name, "_arg") != -1 ||
				/*begins with a number*/
				(macro_name[0] <= '9' && macro_name[0] >= '0')
			)
			{
				printf("<ASM SYNTAX ERROR> This macro deliberately attempts to define or trample a reserved name or character. You may not use this name:\n%s\n", macro_name);
				ASM_PUTS("NOTE: You cannot use names beginning with numbers, either.");
				goto error;	
			}

			if(
				strfind("!", macro_name) != -1 ||
				strfind("?", macro_name) != -1 ||
				strfind("[", macro_name) != -1 ||
				strfind("]", macro_name) != -1 ||
				strfind("\\", macro_name) != -1 ||
				strfind("//", macro_name) != -1 ||
				strfind("/", macro_name) != -1 ||
				strfind(" ", macro_name) != -1 ||
				strfind("\t", macro_name) != -1 ||
				strfind(";", macro_name) != -1 ||
				strfind("|", macro_name) != -1 ||
				strfind("#", macro_name) != -1 ||
				strfind("$", macro_name) != -1 ||
				strfind("&", macro_name) != -1 ||
				strfind("@", macro_name) != -1 ||
				strfind("%",macro_name) != -1 ||
				strfind("bytes",macro_name) != -1 ||
				strfind("shorts",macro_name) != -1 ||
				strfind("asm_",macro_name) != -1 ||
				strfind("ASM_", macro_name) != -1 ||
				strfind("_arg", macro_name) != -1 ||
				strfind("section", macro_name) != -1 ||
				strfind("fill", macro_name) != -1 ||
				strfind("asm_help", macro_name) != -1 ||
				strfind("asm_call", macro_name) != -1 ||
				strfind("asm_fix_outputcounter", macro_name) != -1 ||
				strfind("asm_print", macro_name) != -1 ||
				strfind("asm_vars", macro_name) != -1 ||
				strfind("asm_begin_region_restriction", macro_name) != -1 ||
				strfind("asm_begin_page_restriction", macro_name) != -1 ||
				strfind("asm_begin_block_restriction", macro_name) != -1 ||
				strfind("asm_end_region_restriction", macro_name) != -1 ||
				strfind("asm_end_page_restriction", macro_name) != -1 ||
				strfind("asm_end_block_restriction", macro_name) != -1 ||
				strfind("asm_end_restriction", macro_name) != -1 ||
				strfind("asm_quit", macro_name) != -1
			)
			{
				printf("<ASM SYNTAX ERROR> This macro would prevent language features from being used. You may not use this name:\n%s\n", macro_name);
				goto error;	
			}
			{unsigned short i;for(i = 0; i < nmacros; i++){
				if(i < nbuiltin_macros)
					if( (strfind(variable_names[i],macro_name)>-1) ||
						streq(macro_name, variable_names[i]))
					{
						printf("<ASM SYNTAX ERROR> This macro would prevent critical macro %s from being used.Line:\n%s\n", variable_names[i], line_copy);
						goto error;	
					}
				if(
					(!streq(macro_name, variable_names[i]))
					&& (npasses==1) && 
					(
						(strfind(variable_names[i],macro_name)>-1) ||
						(strfind(macro_name, variable_names[i])>-1)
					)
				){
					if(!clear_output)printf("<ASM WARNING> This Macro may produce a conflict with other Macro: \"%s\"Line:\n%s\n",variable_names[i], line_copy);
				}
				
				if(streq(macro_name, variable_names[i])){
					if(is_overwriting){
						printf("<ASM INTERNAL ERROR> Multiple macros exist with the same name. Line:\n%s\n", line_copy);
						goto error;
					}
					is_overwriting = 1;
					if(i < nbuiltin_macros){
						printf("<ASM SYNTAX ERROR> attempted redefinition of critical macro, Line:\n%s\n", line_copy);
						goto error;	
					}
					index = i;
				}
			}}
			if(!is_overwriting){
				variable_names[nmacros] = macro_name;
				variable_expansions[nmacros++] = 
				str_null_terminated_alloc(
						line+loc_pound+loc_pound2,
						strlen(line+loc_pound+loc_pound2)
				);
			} else {char* temp;
				if(npasses == 0)
					{
						/*if(!clear_output)printf("<ASM WARNING> redefining macro, potentially desyncing compilation. Check that. line: %s\n", line_copy);*/
						variable_is_redefining_flag[index] = 1;
					}
				if(variable_names[index]) free(variable_names[index]);
				variable_names[index] = macro_name;
				temp = 
				str_null_terminated_alloc(
						line+loc_pound+loc_pound2,
						strlen(line+loc_pound+loc_pound2)
				);
				if(npasses == 1 && 
					!variable_is_redefining_flag[index] && 
					!strprefix("_arg", variable_names[index])
					)
				{/*Ensure that the macro evaluates to the exact same piece of text as the last time.*/
					if(!streq(temp, variable_expansions[index])){
					printf("<ASM HUGE WARNING>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\nConfirmed!!! Macro Desynchronization between passes Line:\n%s\nInternally:\n%s\n",line_copy,line);
						free(variable_expansions[index]);
						variable_expansions[index] = temp;
					}
					free(variable_expansions[index]);
					variable_expansions[index] = temp;
				} else {
					free(variable_expansions[index]);
					variable_expansions[index] = temp;
				}
			}
			if(debugging){
				if(!clear_output)printf("\nMacro Contents are %s, size %u\n", variable_expansions[nmacros-1], (unsigned int)strlen(variable_expansions[nmacros-1]));
				if(!clear_output)printf("\nMacro Name is %s, size %u\n", variable_names[nmacros-1], (unsigned int)strlen(variable_names[nmacros-1]));
			}
		}
		if(quit_after_macros || debugging) {
			if(!clear_output)printf("\n#Line post macro expansion and evaluation:\n");
			ASM_PUTS(line); 
			if(quit_after_macros) goto end;
		}

		if(was_macro) goto end;
		/*
			if(strfind(line, "|")!=-1){
				line[strfind(line, "|")] = ';';
				goto pre_pre_processing;
			}
		*/
		/*We must first determine if this line contains a line comment. Don't search past the line comment for insns.*/
		if(strfind(line, "//") != -1){
			long loc_line_comment = strfind(line, "//");
			line[loc_line_comment] = '\0';
		}
		
		/* 
			Assembly Directives

		*/
		
		/*	most complicated step- handle insns.
			Everything breaks down to bytes underneath, so we convert to "bytes" commands.
			The difference between here and above is that we have to count
			how many commas we're supposed to encounter.
			the first comma beyond that before the next semicolon, is replaced with a semicolon.
		*/
		
			{unsigned char have_expanded = 0; unsigned short iteration = 0;
			do{
				have_expanded = 0;
				if(debugging){
					if(!clear_output)printf("\n~~Insn Expansion Stage~~, iteration %u\nLine:\n%s", iteration, line);
				}
				{unsigned long i;for(i = 0; i<n_insns; i++){char* line_old; long loc, linesize; unsigned long j;
					char found_longer_match; int num_commas_needed;
					linesize = strlen(line);
					loc = strfind(line, insns[i]);					
					line_old = line;
					if(loc == -1) continue;
					if(strfind(line, "|")!= -1 &&
						strfind(line, "|") < loc) continue;

					/*Check to make sure that this isn't some other, longer insn.*/
					found_longer_match = 0;
					for(j = 0; j<n_insns; j++){
						if(j == i) continue;
						if(strlen(insns[j]) > strlen(insns[i])){
							long checkme = strfind(line, insns[j]);
							if(checkme == -1) continue;
							/*Does this match contain us?*/
							if(
								/*
								checkme+strlen(insns[j]) >= loc + strlen(insns[i]) &&
								checkme <= loc*/

								(
									checkme+strlen(insns[j]) >= loc + strlen(insns[i]) &&
									checkme <= loc
								) ||
								(
									((checkme+(long)strlen(insns[j])) < loc + (long)strlen(insns[i])) &&
									((checkme+(long)strlen(insns[j])) >= loc)
								) || 
								(
									((checkme) < loc + (long)strlen(insns[i])) &&
									((checkme) >= loc)
								)
							)
							{
								if(debugging) ASM_PUTS("Found longer insn.");
									found_longer_match = 1;
									break;
							}
						}
					}
					if(found_longer_match) continue;
					if(loc>0){
						/*Check to make sure this isn't prefixed by something which is obviously erroneous.*/
						if(*(line+loc-1) != ';')
							if(!clear_output)printf("<ASM WARNING> Instruction parsing reached with "
									"unusual prefixing character \'%c\'. May be syntax error. Instruction to parse is %s. Line:\n%s\nInternally:\n%s\n",*(line+loc-1),insns[i],line_copy, line);
					}
					if(
						*(line+loc+strlen(insns[i])) != ';' &&  
						*(line+loc+strlen(insns[i])) != '\0' && 
						!(
							*(line+loc+strlen(insns[i])) >= '0' &&
							*(line+loc+strlen(insns[i])) <= '9'
						) &&
						*(line+loc+strlen(insns[i])) != ' '
					)
						if(!clear_output)printf("<ASM WARNING> Instruction parsing reached with "
								"unusual postfixing character \'%c\'. May be syntax error. Instruction to parse is %s. Line:\n%s\nInternally:\n%s\n",*(line+loc+strlen(insns[i])),insns[i],line_copy,line);
					/*We know the location of an insn to be expanded and it is at loc.*/
					/*Crawl along the string. */
					num_commas_needed = insns_numargs[i] - 1;
					/*If you have three arguments, you only need two commas.*/
					for(j = loc + strlen(insns[i]); strlen(line+j)>0;j++){
						if(line[j] == ','){
							if(num_commas_needed > 0) num_commas_needed--;
							else { /*Too many commas.*/
								printf("\n<ASM SYNTAX ERROR> too many arguments to insn %s, line %s", insns[i], line_copy);
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
					{char* before; char* after; long len_to_replace;
					len_to_replace = strlen(insns[i]);
					before = str_null_terminated_alloc(line_old, loc);
					before = strcatallocf1(before, insn_repl[i]);
					after = str_null_terminated_alloc(line_old+loc+len_to_replace, 
									linesize-loc-len_to_replace);
					line = strcatallocfb(before, after);
					free(line_old);
					}
				}}
			}while(have_expanded && (iteration++ < 32700)); /*Probably safe right?*/
		}
		/*
			Put out bytes.
		*/
		/*Inch along*/
		if(printlines && npasses==1){
			if(!clear_output)printf("%s\n",line);
		}
		metaproc = line; /*declaration at beginning of main.*/
		do{ long incr, incrdont;
			if(strprefix("bytes", metaproc)){ char* proc;
				proc = metaproc + 5;
				do{
					unsigned char byteval; unsigned long preval; 
					if(int_checker(proc)){
						printf("<ASM SYNTAX ERROR> invalid integer literal for bytes command. Line:\n%s\nInternal:\n%s\n",line_copy, line);
						goto error;
					}
					
					preval = strtoul(proc,NULL,0);
					byteval = preval & 255;
					fputbyte(byteval, ofile);
					/*Find the next comma.*/
					incr = strfind(proc, ",");
					incrdont = strfind(proc, ";");
					if(incr == -1) break;
					if(incrdont != -1 &&
						incr > incrdont)break;/**/
					proc += incr; proc += 1; /*Skip the comma itself.*/
				}while(strlen(proc) > 0);
				
			} else if(strprefix("shorts", metaproc)){
				char* proc = metaproc + 6;
				do{
					unsigned short shortval;
					if(int_checker(proc)){
						printf("<ASM SYNTAX ERROR> invalid integer literal for shorts command. Line:\n%s\nInternal:\n%s\n",line_copy, line);
						goto error;
					}
					shortval = strtoul(proc,NULL,0);
					putshort(shortval, ofile);
					/*Find the next comma.*/
					incr = strfind(proc, ",");
					incrdont = strfind(proc, ";");
					if(incr == -1) break;
					if(incrdont != -1 &&
						incr > incrdont) break; /**/
					proc += incr; proc += 1; /*Skip the comma itself.*/
				}while(strlen(proc) > 0);
			} else if(strprefix("section", metaproc)){ unsigned long dest;
				char* proc = metaproc + 7;
				if(strlen(proc) == 0){
					puts("<ASM SYNTAX ERROR> Cannot have empty SECTION tag.");
					goto error;
				}
				if(int_checker(proc)){
					printf("<ASM SYNTAX ERROR> invalid integer literal for section command. Line:\n%s\nInternal:\n%s\n",line_copy, line);
					goto error;
				}
				dest = strtoul(proc, NULL, 0);
				if(dest == 0){
				/*Explicitly check to see if they actually typed zero.*/
					if(proc[0]!='0'  && npasses == 1)
					if(!clear_output)printf("<ASM WARNING> Section tag at zero. Might be a bad number. Line %s\n", line_copy);

				}
				if(debugging)
					if(!clear_output)printf("Moving the output counter to %lu\n", dest);
				outputcounter = dest;
			} else if(strprefix("fill", metaproc)){ unsigned long fillsize; long next_comma; unsigned char fillval;
				char* proc = metaproc + 4;
				if(strlen(proc) == 0){
					puts("<ASM SYNTAX ERROR> Cannot have empty fill tag.");
					goto error;
				}
				if(int_checker(proc)){
					printf("<ASM SYNTAX ERROR> invalid integer literal for fill command. Line:\n%s\nInternal:\n%s\n",line_copy, line);
					goto error;
				}
				fillsize = strtoul(proc, NULL, 0);
				if(fillsize == 0){
					if(proc[0]!='0') /*Check if they actually typed zero.*/
					if(!clear_output)printf("<ASM WARNING> fill tag size is zero. Might be a bad number. Line %s", line_copy);
				}
				/**/
				next_comma = strfind(proc, ",");
				if(next_comma == -1) {
					printf("<ASM SYNTAX ERROR> Fill tag missing value. Line %s", line_copy);
					goto error;
				}
				proc += next_comma + 1;
				if(int_checker(proc)){
					printf("<ASM SYNTAX ERROR> invalid integer literal for fill command. Line:\n%s\nInternal:\n%s\n",line_copy, line);
					goto error;
				}
				fillval = strtoul(proc, NULL, 0);
				if(fillval == 0) /*potential for a mistake*/
					if(proc[0]!='0') /*Did they actually MEAN zero?*/
						if(!clear_output)printf("<ASM WARNING> fill tag value is zero. Might be a bad number. Line:\n%s\n", line_copy);
				for(;fillsize>0;fillsize--)fputbyte(fillval, ofile);
			} else if(strprefix("asm_fix_outputcounter", metaproc)){ /*Perform a second-pass correction of the output counter.*/
				char* proc; char mode; unsigned long outputcounterold;
				if(npasses == 1){
					proc = metaproc + strlen("asm_fix_outputcounter");
					if(proc[0] == '+') mode=0; else
					if(proc[0] == '-') mode=1; else
					{
						printf("\n<ASM SYNTAX ERROR> asm_correct_outp lacks a + or -.Line:\n%s\n", line_copy);
						goto error;
					}proc++;
					outputcounterold = outputcounter;
					if(int_checker(proc)){
						printf("<ASM SYNTAX ERROR> invalid integer literal for asm_fix_outputcounter command. Line:\n%s\nInternal:\n%s\n",line_copy, line);
						goto error;
					}
					if(!mode)
						outputcounter += strtoul(proc, NULL, 0);
					else
						outputcounter -= strtoul(proc, NULL, 0);
					outputcounter &= 0xffFFff;
					if(!clear_output)printf("<ASM WARNING> output counter was:%lx, now: %lx. Line:\n%s\n", outputcounterold, outputcounter, line_copy);
				}
			} else if(strprefix("asm_print", metaproc)){
				if(npasses == 1 && !printlines)
					if(!clear_output)printf("\nRequest to print status at this insn. STATUS:\nLine:\n%s\nLine Internally:\n%s\nCounter: %04lx\n", line_copy, line, outputcounter);
			} else if(strprefix("asm_begin_region_restriction", metaproc)){
				/*The assembler will warn you if the region changes during the creation of the function.*/
				region_restriction = (outputcounter>>16) & 0xFF;
				region_restriction_mode = 2; /*region*/
			} else if(strprefix("asm_end_region_restriction", metaproc)){
				region_restriction_mode = 0; /*end region*/
			} else if(strprefix("asm_begin_block_restriction", metaproc) || strprefix("asm_begin_page_restriction", metaproc)){
				/*The assembler will warn you if the block changes during the creation of the function.*/
				region_restriction = (outputcounter>>8) & 0xFFFF;
				region_restriction_mode = 1; /*block*/
			} else if(strprefix("asm_end_block_restriction", metaproc)|| 
					strprefix("asm_end_page_restriction", metaproc)||
					strprefix("asm_end_restriction", metaproc)
				)
			{
				region_restriction_mode = 0; /*end block*/
			} else if(strprefix("asm_vars", metaproc)){
				unsigned long i;
				if(!clear_output)printf("\nSTATUS:\nLine:\n%s\nLine Internally:\n%s\nCounter: %04lx\n", line_copy, line, outputcounter);
				if(!clear_output)printf("<DUMPING SYMBOL TABLE ON PASS %ld>\n", npasses+1);
				for(i=0;i<nmacros;i++){
					if(i==nmacros) ASM_PUTS("Showing User Macros");
					if(strprefix("_arg", variable_names[i])) if(!clear_output)printf("\n<auto>:");
					if(i==0){char expansion[1024];
						sprintf(expansion, "%lu", outputcounter);
						expansion[1023] = '\0'; /*Just in case...*/
						if(!clear_output)printf("VAR#%s#%s\n",variable_names[i],expansion);
					} else if(i==1){char expansion[1024];
						sprintf(expansion, "%lu,%lu", (unsigned long)(outputcounter/256),(unsigned long)(outputcounter&0xff));
						expansion[1023] = '\0'; /*Just in case...*/
						if(!clear_output)printf("VAR#%s#%s\n",variable_names[i],expansion);	
					}else
						if(!clear_output)printf("VAR#%s#%s\n",variable_names[i],variable_expansions[i]);
				}
			} else if(strprefix("asm_quit", metaproc)){
				if(npasses == 1)
					if(!clear_output)printf("\nRequest to halt assembly at this insn. STATUS:\nLine:\n%s\nCounter: %04lx\n", line_copy, outputcounter);
				goto error;
			}else if(strprefix("!", metaproc)){
				printf("<ASM SYNTAX ERROR> Cannot put string literal here, Line:\n%s\nInternal:\n%s\n", line_copy, metaproc);
				goto error;
			}else if (strprefix("#", metaproc) || strprefix("//", metaproc)){
				if(debugging)
					if(!clear_output)printf("<ASM WARNING> Inline Comment invalidates rest of line, Line:\n%s\nInternal:\n%s\n", line_copy, metaproc);
				break; /*Comment on line.*/
			} else {
				if(strlen(metaproc) > 0 && !
					(strfind(metaproc,";") == 0 ||
					strfind(metaproc,"|") == 0)){
					printf("<ASM SYNTAX ERROR> invalid statement on line %s\nStatement:%s\n", line_copy, metaproc);
					goto error;
				}
			}
			{long next_semicolon = strfind(metaproc, ";");
			long next_vbar = strfind(metaproc, "|");
			if(next_semicolon == -1) break; /*We have handled all sublines.*/
			if(next_vbar!=-1 && next_vbar < next_semicolon) break; /**/
			metaproc += next_semicolon + 1;
			if(strlen(metaproc) == 0) break; /*Convenient line break*/
			}
		} while(1);
		/*if this is a line with vertical bars, start processing the stuff after the next vertical bar. */
		if(strfind(line, "|")!=-1){
			char* line_temp = strcatalloc(line+strfind(line, "|")+1, "");
			free(line); line = line_temp;
			goto pre_pre_processing;
		}
		end:
		free(line);
		free(line_copy);
		continue;
		/*Yikes!*/
		error:
		puts("<ASM> Assembler Aborted.\n");
		return 1;
	}
	if(!clear_output)printf("<ASM> Successfully assembled %s\n", outfilename);
	if(ofile) 	fclose(ofile);
	if(infile)	fclose(infile);
	if(run_sisa16 && !quit_after_macros && !debugging){
		SEGMENT = malloc(256);
		SEGMENT_PAGES = 1;
		if(SEGMENT == NULL){
			puts("<SISA16 EMULATOR ERROR: Could not allocate segment>");
			return 1;
		}
		if(
			(sizeof(U) != 2) ||
			(sizeof(u) != 1) ||
			(sizeof(UU) != 4) ||
			(sizeof(SUU) != 4)
#ifndef NO_FP
			|| (sizeof(float) != 4)
#endif
		){
			puts("SISA16 ERROR!!! Compilation Environment conditions unmet.");
			if(sizeof(U) != 2)
				puts("U is not 2 bytes. Try using something other than unsigned short (default).");
			if(sizeof(u) != 1)
				puts("u is not 2 bytes. Try using something other than unsigned char (default).");
			if(sizeof(UU) != 4)
				puts("UU is not 4 bytes. Try toggling -DUSE_UNSIGNED_INT. the default is to use unsigned int as UU.");
			if(sizeof(SUU) != 4)
				puts("SUU is not 4 bytes. Try toggling -DUSE_UNSIGNED_INT. the default is to use int as SUU.");
#ifndef NO_FP
			if(sizeof(float) != 4){
				puts("float is not 4 bytes. Disable the floating point unit during compilation, -DNO_FP");
			}
#endif
			return 1;
		}

		
		e();

		if(R==1)puts("\n<Errfl, 16 bit div by 0>\n");
		if(R==2)puts("\n<Errfl, 16 bit mod by 0>\n");
		if(R==3)puts("\n<Errfl, 32 bit div by 0>\n");
		if(R==4)puts("\n<Errfl, 32 bit mod by 0>\n");
		if(R==5)puts("\n<Errfl, Bad Segment Page>\n");
		if(R==6)puts("\n<Errfl, Segment Cannot be Zero Pages>\n");
		if(R==7)puts("\n<Errfl, Segment Failed Allocation>\n");
#if defined(NO_FP)
		if(R==8)puts("\n<Errfl, Floating point unit disabled by compiletime options>\n");
#else
		if(R==8)puts("\n<Errfl, Internal error, reporting broken SISA16 FPU. Report this bug! https://github.com/gek169/Simple_ISA/  >\n");
#endif

		if(R==9)puts("\n<Errfl, Floating point divide by zero>\n");
#if defined(NO_SIGNED_DIV)
		if(R==10)puts("\n<Errfl, Signed 32 bit division disabled by compiletime options>\n");
#else
		if(R==10)puts("\n<Errfl, Internal error, reporting broken SISA16 signed integer division module. Report this bug! https://github.com/gek169/Simple_ISA/  >\n");
#endif
	}
	return 0;
}
