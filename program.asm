#Compile this program using asm. ./asm -i program.asm -o program.bin
section 0
fill 0xFFFF,0

section 0xF000
!hello world!!! You should see this print.
bytes 0xA, 0xA;
!Another string.
bytes 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA
bytes 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA
bytes 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA
bytes 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA
bytes 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA
bytes 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA
bytes 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA

section 0
#loop control
lda 1,0xe8
lb 1
sc 0,0
add
sta 1,0xe8
#grab our character from our array of characters

llb 0xEF,0xFE
add
illdaa;
llb 0,255;and;
putchar



#perform the loop check
lda 1,0xe8
lb 0x41
cmp;lb 0;cmp;jmpifeq


#Load the value at 1e8 into register A
lda 1,0xe8
#Print it
putchar
#Halt execution
#alternatively, halt by dividing by zero
#la 1
#lb 0
#mod
halt
