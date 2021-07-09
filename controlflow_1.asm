#!/usr/local/bin/sisa16_asm -run
//Example program from the manual to demonstrate basic control flow.

..main(1):
getchar; 
farstla %&0x20000%; 
putchar;
lb '8'; cmp; lb 0; cmp; 
sc %Lbl_val_is_lt%; jmpifeq;
sc %Lbl_val_is_gte%; jmp;

:Lbl_val_is_lt:
	halt;
:Lbl_val_is_gte:
	farllda %&0x20000%;
	cpc; 
	putchar; 
	jmp;
