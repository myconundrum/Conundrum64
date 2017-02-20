;
; basic asm
;

* = $A000
.DB $94 $E3 ; BASIC cold entry point
.DB $7B $E3 ; BASIC warm entry point
.DB $43 $42 $4D $42 $41 $53 $49 $43  ; 'cbmbasic'

;action addresses for primary commands
;
;                                these are called by pushing the address onto the stack and doing an RTS so the
;                                actual address -1 needs to be pushed
.DB $30 $A8                    ;perform END     $80
.DB $41 $A7                    ;perform FOR     $81
.DB $1D $AD                    ;perform NEXT    $82
.DB $F7 $A8                    ;perform DATA    $83
.DB $A4 $AB                    ;perform INPUT#  $84
.DB $BE $AB                    ;perform INPUT   $85
.DB $80 $B0                    ;perform DIM     $86
.DB $05 $AC                    ;perform READ    $87
.DB $A4 $A9                    ;perform LET     $88
.DB $9F $A8                    ;perform GOTO    $89
.DB $70 $A8                    ;perform RUN     $8A
.DB $27 $A9                    ;perform IF      $8B
.DB $1C $A8                    ;perform RESTORE $8C
.DB $82 $A8                    ;perform GOSUB   $8D
.DB $D1 $A8                    ;perform RETURN  $8E
.DB $3A $A9                    ;perform REM     $8F
.DB $2E $A8                    ;perform STOP    $90
.DB $4A $A9                    ;perform ON      $91
.DB $2C $B8                    ;perform WAIT    $92
.DB $67 $E1                    ;perform LOAD    $93
.DB $55 $E1                    ;perform SAVE    $94
.DB $64 $E1                    ;perform VERIFY  $95
.DB $B2 $B3                    ;perform DEF     $96
.DB $23 $B8                    ;perform POKE    $97
.DB $7F $AA                    ;perform PRINT#  $98
.DB $9F $AA                    ;perform PRINT   $99
.DB $56 $A8                    ;perform CONT    $9A
.DB $9B $A6                    ;perform LIST    $9B
.DB $5D $A6                    ;perform CLR     $9C
.DB $85 $AA                    ;perform CMD     $9D
.DB $29 $E1                    ;perform SYS     $9E
.DB $BD $E1                    ;perform OPEN    $9F
.DB $C6 $E1                    ;perform CLOSE   $A0
.DB $7A $AB                    ;perform GET     $A1
.DB $41 4A6                    ;perform NEW     $A2

; action addresses for functions

.DB $39 $BC                    ;perform SGN     $B4
.DB $CC $BC                    ;perform INT     $B5
.DB $58 $BC                    ;perform ABS     $B6
.DB $10 $03                    ;perform USR     $B7
.DB $7D $B3                    ;perform FRE     $B8
.DB $9E $B3                    ;perform POS     $B9
.DB $71 $BF                    ;perform SQR     $BA
.DB $97 $E0                    ;perform RND     $BB
.DB $EA $B9                    ;perform LOG     $BC
.DB $ED $BF                    ;perform EXP     $BD
.DB $64 $E2                    ;perform COS     $BE
.DB $6B $E2                    ;perform SIN     $BF
.DB $B4 $E2                    ;perform TAN     $C0
.DB $0E $E3                    ;perform ATN     $C1
.DB $0D $B8                    ;perform PEEK    $C2
.DB $7C $B7                    ;perform LEN     $C3
.DB $65 $B4                    ;perform STR$    $C4
.DB $AD $B7                    ;perform VAL     $C5
.DB $8B $B7                    ;perform ASC     $C6
.DB $EC $B6                    ;perform CHR$    $C7
.DB $00 $B7                    ;perform LEFT$   $C8
.DB $2C $B7                    ;perform RIGHT$  $C9
.DB $37 $B7                    ;perform MID$    $CA

; precedence byte and action addresses for operators
;
;                                like the primary commands these are called by pushing the address onto the
;  								stack and doing an RTS, so again the actual address -1 needs to be pushe
.DB $79 $69 $B8                 ;+
.DB $79 $52 $B8                 ;-
.DB $7B $2A $BA                 ;*
.DB $7B $11 $BB                 ;/
.DB $7F $7A $BF                 ;^
.DB $50 $E8 $AF                 ;AND
.DB $46 $E5 $AF                 ;OR
.DB $7D $B3 $BF                 ;>
.DB $5A $D3 $AE                 ;=
.DB $64 $15 $B0                 ;<

;BASIC keywords
;
;                                each word has b7 set in it's last character as an end marker, even
;                                the one character keywords such as "<" or "="
;                                first are the primary command keywords, only these can start a statement

.DB $45 $4E                     	;end
.DB $C4 $46 $4F $D2 $4E $45 $58 $D4  ;for next
.DB $44 $41 $54 $C1 $49 $4E $50 $55  ;data input#
.DB $54 $A3 $49 $4E $50 $55 $D4 $44  ;input dim
.DB $49 $CD $52 $45 $41 $C4 $4C $45  ;read let
.DB $D4 $47 $4F $54 $CF $52 $55 $CE  ;goto run
.DB $49 $C6 $52 $45 $53 $54 $4F $52  ;if restore
.DB $C5 $47 $4F $53 $55 $C2 $52 $45  ;gosub return
.DB $54 $55 $52 $CE $52 $45 $CD $53  ;rem stop
.DB $54 $4F $D0 $4F $CE $57 $41 $49  ;on wait
.DB $D4 $4C $4F $41 $C4 $53 $41 $56  ;load save
.DB $C5 $56 $45 $52 $49 $46 $D9 $44  ;verify def
.DB $45 $C6 $50 $4F $4B $C5 $50 $52  ;poke print#
.DB $49 $4E $54 $A3 $50 $52 $49 $4E  ;print
.DB $D4 $43 $4F $4E $D4 $4C $49 $53  ;cont list
.DB $D4 $43 $4C $D2 $43 $4D $C4 $53  ;clr cmd sys
.DB $59 $D3 $4F $50 $45 $CE $43 $4C  ;open close
.DB $4F $53 $C5 $47 $45 $D4 $4E $45  ;get new

                                ; next are the secondary command keywords, these can not start a statement
.DB $D7 $54 $41 $42 $A8 $54 $CF $46  ;tab( to
.DB $CE $53 $50 $43 $A8 $54 $48 $45  ;spc( then
.DB $CE $4E $4F $D4 $53 $54 $45 $D0  ;not stop
                                ; next are the operators
.DB $AB $AD $AA $AF $DE $41 $4E $C4  ;+ - * / ' and
.DB $4F $D2 $BE $BD $BC           ;or <=>
.DB $53 $47 $CE  					;sgn

 ;                               and finally the functions
.DB $49 $4E $D4 $41 $42 $D3 $55 $53  ;int abs usr
.DB $D2 $46 $52 $C5 $50 $4F $D3 $53  ;fre pos sqr
.DB $51 $D2 $52 $4E $C4 $4C $4F $C7  ;rnd log
.DB $45 $58 $D0 $43 $4F $D3 $53 $49  ;exp cos sin
.DB $CE $54 $41 $CE $41 $54 $CE $50  ;tan atn peek
.DB $45 $45 $CB $4C $45 $CE $53 $54  ;len str$
.DB $52 $A4 $56 $41 $CC $41 $53 $C3  ;val asc
.DB $43 $48 $52 $A4 $4C $45 $46 $54  ;chr$ left$
.DB $A4 $52 $49 $47 $48 $54 $A4 $4D  ;right$ mid$

                              ;  lastly is GO, this is an add on so that GO TO, as well as GOTO, will work
.DB $49 $44 $A4 $47 $CF           ;go
.DB $00                       ;end marker

; BASIC error messages

.DB $54 4F                    			;1 too many files
.DB $4F $20 $4D $41 $4E $59 $20 $46
.DB $49 $4C $45 $D3 $46 $49 $4C $45  ;2 file open
.DB $20 $4F $50 $45 $CE $46 $49 $4C  ;3 file not open
.DB $45 $20 $4E $4F $54 $20 $4F $50
.DB $45 $CE $46 $49 $4C $45 $20 $4E  ;4 file not found
.DB $4F $54 $20 $46 $4F $55 $4E $C4  ;5 device not present
.DB $44 $45 $56 $49 $43 $45 $20 $4E
.DB $4F $54 $20 $50 $52 $45 $53 $45
.DB $4E $D4 $4E $4F $54 $20 $49 $4E  ;6 not input file
.DB $50 $55 $54 $20 $46 $49 $4C $C5
.DB $4E $4F $54 $20 $4F $55 $54 $50  ;7 not output file
.DB $55 $54 $20 $46 $49 $4C $C5 $4D
.DB $49 $53 $53 $49 $4E $47 $20 $46  ;8 missing filename
.DB $49 $4C $45 $20 $4E $41 $4D $C5
.DB $49 $4C $4C $45 $47 $41 $4C $20  ;9 illegal device number
.DB $44 $45 $56 $49 $43 $45 $20 $4E
.DB $55 $4D $42 $45 $D2 $4E $45 $58  ;10 next without for
.DB $54 $20 $57 $49 $54 $48 $4F $55
.DB $54 $20 $46 $4F $D2 $53 $59 $4E  ;11 syntax
.DB $54 $41 $D8 $52 $45 $54 $55 $52  ;12 return without gosub
.DB $4E $20 $57 $49 $54 $48 $4F $55
.DB $54 $20 $47 $4F $53 $55 $C2 $4F  ;13 out of data
.DB $55 $54 $20 $4F $46 $20 $44 $41
.DB $54 $C1 $49 $4C $4C $45 $47 $41  ;14 illegal quantity
.DB $4C $20 $51 $55 $41 $4E $54 $49
.DB $54 $D9 $4F $56 $45 $52 $46 $4C  ;15 overflow
.DB $4F $D7 $4F $55 $54 $20 $4F $46  ;16 out of memory
.DB $20 $4D $45 $4D $4F $52 $D9 $55  ;17 undef'd statement
.DB $4E $44 $45 $46 $27 $44 $20 $53
.DB $54 $41 $54 $45 $4D $45 $4E $D4
.DB $42 $41 $44 $20 $53 $55 $42 $53  ;18 bad subscript
.DB $43 $52 $49 $50 $D4 $52 $45 $44  ;19 redim'd array
.DB $49 $4D $27 $44 $20 $41 $52 $52
.DB $41 $D9 $44 $49 $56 $49 $53 $49  ;20 division by zero
.DB $4F $4E $20 $42 $59 $20 $5A $45
.DB $52 $CF $49 $4C $4C $45 $47 $41  ;21 illegal direct
.DB $4C $20 $44 $49 $52 $45 $43 $D4
.DB $54 $59 $50 $45 $20 $4D $49 $53  ;22 type mismatch
.DB $4D $41 $54 $43 $C8 $53 $54 $52  ;23 string too long
.DB $49 $4E $47 $20 $54 $4F $4F $20
.DB $4C $4F $4E $C7 $46 $49 $4C $45  ;24 file data
.DB $20 $44 $41 $54 $C1 $46 $4F $52  ;25 formula too complex
.DB $4D $55 $4C $41 $20 $54 $4F $4F
.DB $20 $43 $4F $4D $50 $4C $45 $D8
.DB $43 $41 $4E $27 $54 $20 $43 $4F  ;26 can't continue
.DB $4E $54 $49 $4E $55 $C5 $55 $4E  ;27 undef'd function
.DB $44 $45 $46 $27 $44 $20 $46 $55
.DB $4E $43 $54 $49 $4F $CE $56 $45  ;28 verify
.DB $52 $49 $46 $D9 $4C $4F $41 $C4  ;29 load

; error message pointer table

.DB $9E $A1 $AC $A1 $B5 $A1 $C2 $A1
.DB $D0 $A1 $E2 $A1 $F0 $A1 $FF $A1
.DB $10 $A2 $25 $A2 $35 $A2 $3B $A2
.DB $4F $A2 $5A $A2 $6A $A2 $72 $A2
.DB $7F $A2 $90 $A2 $9D $A2 $AA $A2
.DB $BA $A2 $C8 $A2 $D5 $A2 $E4 $A2
.DB $ED $A2 $00 $A3 $0E $A3 $1E $A3
.DB $24 $A3 $83 $A3

; BASIC messages

.DB $0D $4F $4B $0D              ;OK
.DB $00 $20 $20 $45 $52 $52 $4F $52  ;ERROR
.DB $00 $20 $49 $4E $20 $00 $0D $0A  ;IN
.DB $52 $45 $41 $44 $59 $2E $0D $0A  ;READY.
.DB $00 $0D $0A $42 $52 $45 $41 $4B  ;BREAK
.DB $00

; spare byte, not referenced

.DB $A0                       ; unused


; BASIC warm start entry point
WMST = $E37B
* = WMST
JSR $FFCC       ;close input and output channels
LDA #$00        ;clear A
STA $13         ;set current I/O channel, flag default
JSR $A67A       ;flush BASIC stack and clear continue pointer
CLI             ;enable the interrupts
LDX #$80        ;set -ve error, just do warm start
JMP ($0300)     ;go handle error message, normally $E38B ; MRW BUGBUG START HERE
TXA             ;copy the error number
BMI $E391       ;if -ve go do warm start
JMP $A43A       ;else do error #X then warm start
JMP $A474       ;do warm start



; BASIC COLD ENTRY POINT

BST = $E394

* = BST
JSR $E453       ;initialise the BASIC vector table
JSR $E3BF       ;initialise the BASIC RAM locations
JSR $E422       ;print the start up message and initialise the memory
                                ;pointers
                                ;not ok ??
LDX #$FB        ;value for start stack
TXS             ;set stack pointer
BNE $E386       ;do "READY." warm start, branch always


* = $E422 ;print the start up message and initialise the memory pointers

LDA $2B         ;get the start of memory low byte
LDY $2C         ;get the start of memory high byte
JSR $A408       ;check available memory, do out of memory error if no room
LDA #$73        ;set "**** COMMODORE 64 BASIC V2 ****" pointer low byte
LDY #$E4        ;set "**** COMMODORE 64 BASIC V2 ****" pointer high byte
JSR $AB1E       ;print a null terminated string
LDA $37         ;get the end of memory low byte
SEC             ;set carry for subtract
SBC $2B         ;subtract the start of memory low byte
TAX             ;copy the result to X
LDA $38         ;get the end of memory high byte
SBC $2C         ;subtract the start of memory high byte
JSR $BDCD       ;print XA as unsigned integer
LDA #$60        ;set " BYTES FREE" pointer low byte
LDY #$E4        ;set " BYTES FREE" pointer high byte
JSR $AB1E       ;print a null terminated string
JMP $A644       ;do NEW, CLEAR, RESTORE and return



* = $E447 ; BASIC vectors, these are copied to RAM from $0300 onwards

.DB $8B $E3                    ;error message          $0300
.DB $83 $A4                    ;BASIC warm start       $0302
.DB $7C $A5                    ;crunch BASIC tokens    $0304
.DB $1A $A7                    ;uncrunch BASIC tokens  $0306
.DB $E4 $A7                    ;start new BASIC code   $0308
.DB $86 $AE                    ;get arithmetic element $030A

BVT = $E453
* = BVT ; initialise the BASIC vectors

LDX #$0B        ;set byte count
LDA $E447,X     ;get byte from table
STA $0300,X     ;save byte to RAM
DEX             ;decrement index
BPL $E455       ;loop if more to do
RTS

* = $E45F ; BASIC startup messages

.DB $00 $20 $42 $41 $53 $49 $43 $20  ;basic bytes free
.DB $42 $59 $54 $45 $53 $20 $46 $52
.DB $45 $45 $0D $00 $93 $0D $20 $20
.DB $93 $0D $20 $20 $20 $20 $2A $2A  ;(clr) **** commodore 64 basic v2 ****
.DB $2A $2A $20 $43 $4F $4D $4D $4F  ;(cr) (cr) 64k ram system
.DB $44 $4F $52 $45 $20 $36 $34 $20
.DB $42 $41 $53 $49 $43 $20 $56 $32
.DB $20 $2A $2A $2A $2A $0D $0D $20
.DB $36 $34 $4B $20 $52 $41 $4D $20
.DB $53 $59 $53 $54 $45 $4D $20 $20
.DB $00

; unused

.DB $5C

BRAM = $E3BF
* = BRAM;initialise BASIC RAM locations

LDA #$4C        ;opcode for JMP
STA $54         ;save for functions vector jump
STA $0310       ;save for USR() vector jump
                ;                set USR() vector to illegal quantity error
LDA #$48        ;set USR() vector low byte
LDY #$B2        ;set USR() vector high byte
STA $0311       ;save USR() vector low byte
STY $0312       ;save USR() vector high byte
LDA #$91        ;set fixed to float vector low byte
LDY #$B3        ;set fixed to float vector high byte
STA $05         ;save fixed to float vector low byte
STY $06         ;save fixed to float vector high byte
LDA #$AA        ;set float to fixed vector low byte
LDY #$B1        ;set float to fixed vector high byte
STA $03         ;save float to fixed vector low byte
STY $04         ;save float to fixed vector high byte
                ;                copy the character get subroutine from $E3A2 to $0074
LDX #$1C        ;set the byte count
LDA $E3A2,X     ;get a byte from the table
STA $73,X       ;save the byte in page zero
DEX             ;decrement the count
BPL $E3E2       ;loop if not all done
                ;                clear descriptors, strings, program area and mamory pointers
LDA #$03        ;set the step size, collecting descriptors
STA $53         ;save the garbage collection step size
LDA #$00        ;clear A
STA $68         ;clear FAC1 overflow byte
STA $13         ;clear the current I/O channel, flag default
STA $18         ;clear the current descriptor stack item pointer high byte
LDX #$01        ;set X
STX $01FD      ; set the chain link pointer low byte
STX $01FC      ; set the chain link pointer high byte
LDX #$19        ;initial the value for descriptor stack
STX $16         ;set descriptor stack pointer
SEC             ;set Cb = 1 to read the bottom of memory
JSR $FF9C       ;read/set the bottom of memory
STX $2B         ;save the start of memory low byte
STY $2C         ;save the start of memory high byte
SEC            ; set Cb = 1 to read the top of memory
JSR $FF99       ;read/set the top of memory
STX $37         ;save the end of memory low byte
STY $38         ;save the end of memory high byte
STX $33         ;set the bottom of string space low byte
STY $34         ;set the bottom of string space high byte
LDY #$00        ;clear the index
TYA            ; clear the A
STA ($2B),Y     ;clear the the first byte of memory
INC $2B         ;increment the start of memory low byte
BNE $E421       ;if no rollover skip the high byte increment
INC $2C         ;increment start of memory high byte
RTS


* = $A408 ; check available memory, do out of memory error if no room
CPY $34         ;compare with bottom of string space high byte
BCC $A434       ;if less then exit (is ok)
BNE $A412       ;skip next test if greater (tested <)
                ;                high byte was =, now do low byte
CMP $33         ;compare with bottom of string space low byte
BCC $A434       ;if less then exit (is ok)
                ;                address is > string storage ptr (oops!)
PHA             ;push address low byte
LDX #$09        ;set index to save $57 to $60 inclusive
TYA             ;copy address high byte (to push on stack)
                ;                save misc numeric work area
PHA             ;push byte
LDA $57,X       ;get byte from $57 to $60
DEX             ;decrement index
BPL $A416       ;loop until all done
;
; MRW BUGBUG: Need to do garbage colleciton routine.
;
;
;JSR $B526       ;do garbage collection routine
                ;                restore misc numeric work area
LDX #$F7        ;set index to restore bytes
PLA             ;pop byte
STA $61,X       ;save byte to $57 to $60
INX             ;increment index
BMI $A421       ;loop while -ve
PLA             ;pop address high byte
TAY             ;copy back to Y
PLA             ;pop address low byte
CPY $34         ;compare with bottom of string space high byte
BCC $A434       ;if less then exit (is ok)
BNE $A435       ;if greater do out of memory error then warm start
                ;                high byte was =, now do low byte
CMP $33         ;compare with bottom of string space low byte
BCS $A435       ;if >= do out of memory error then warm start
                ;ok exit, carry clear
RTS
LDX #$10        ;error code $10, out of memory error
				;do error #X then warm start
JMP ($0300)     ;do error message




* = $AB1E ; print null terminated string

JSR $B487       ;print " terminated string to utility pointer



* = $B487

                                ;print " terminated string to utility pointer
LDX #$22        ;set terminator to "
STX $07         ;set search character, terminator 1
STX $08         ;set terminator 2
                ;                print search or alternate terminated string to utility pointer
                ;                source is AY
STA $6F         ;store string start low byte
STY $70         ;store string start high byte
STA $62         ;save string pointer low byte
STY $63         ;save string pointer high byte
LDY #$FF        ;set length to -1
INY             ;increment length
LDA ($6F),Y     ;get byte from string
BEQ $B4A8       ;exit loop if null byte [EOS]
CMP $07         ;compare with search character, terminator 1
BEQ $B4A4       ;branch if terminator
CMP $08         ;compare with terminator 2
BNE $B497       ;loop if not terminator 2
CMP #$22        ;compare with "
BEQ $B4A9       ;branch if " (carry set if = !)
CLC             ;clear carry for add (only if [EOL] terminated string)
STY $61         ;save length in FAC1 exponent
TYA             ;copy length to A
ADC $6F         ;add string start low byte
STA $71         ;save string end low byte
LDX $70         ;get string start high byte
BCC $B4B5       ;branch if no low byte overflow
INX             ;else increment high byte
STX $72         ;save string end high byte
LDA $70         ;get string start high byte
BEQ $B4BF       ;branch if in utility area
CMP #$02        ;compare with input buffer memory high byte
BNE $B4CA       ;branch if not in input buffer memory
                ;                string in input buffer or utility area, move to string
                ;                memory
TYA             ;copy length to A
JSR $B475       ;copy descriptor pointer and make string space A bytes long
LDX $6F         ;get string start low byte
LDY $70         ;get string start high byte
JSR $B688       ;store string A bytes long from XY to utility pointer
                ;                check for space on descriptor stack then ...
                ;                put string address and length on descriptor stack and update stack pointers
LDX $16         ;get the descriptor stack pointer
CPX #$22        ;compare it with the maximum + 1
BNE $B4D5       ;if there is space on the string stack continue
                ;                else do string too complex error
LDX #$19        ;error $19, string too complex error
JMP $A437       ;do error #X then warm start 
				;put string address and length on descriptor stack and update stack pointers
LDA $61         ;get the string length
STA $00,X       ;put it on the string stack
LDA $62         ;get the string pointer low byte
STA $01,X       ;put it on the string stack
LDA $63         ;get the string pointer high byte
STA $02,X       ;put it on the string stack
LDY #$00        ;clear Y
STX $64         ;save the string descriptor pointer low byte
STY $65         ;save the string descriptor pointer high byte, always $00
STY $70         ;clear FAC1 rounding byte
DEY             ;Y = $FF
STY $0D         ;save the data type flag, $FF = string
STX $17         ;save the current descriptor stack item pointer low byte
INX             ;update the stack pointer
INX             ;update the stack pointer
INX             ;update the stack pointer
STX $16         ;save the new descriptor stack pointer
RTS

* = $B475 ;do string vector

                                ;copy descriptor pointer and make string space A bytes long
LDX $64         ;get descriptor pointer low byte
LDY $65         ;get descriptor pointer high byte
STX $50         ;save descriptor pointer low byte
STY $51         ;save descriptor pointer high byte

;make string space A bytes long

JSR $B4F4       ;make space in string memory for string A long
STX $62         ;save string pointer low byte
STY $63         ;save string pointer high byte
STA $61         ;save length
RTS


* = $B4F4 ; make space in string memory for string A long

                ;                return X = pointer low byte, Y = pointer high byte
LSR $0F         ;clear garbage collected flag (b7)
                ;                make space for string A long
PHA             ;save string length
EOR #$FF        ;complement it
SEC             ;set carry for subtract, two's complement add
ADC $33         ;add bottom of string space low byte, subtract length
LDY $34         ;get bottom of string space high byte
BCS $B501       ;skip decrement if no underflow
DEY             ;decrement bottom of string space high byte
CPY $32         ;compare with end of arrays high byte
BCC $B516       ;do out of memory error if less
BNE $B50B       ;if not = skip next test
CMP $31         ;compare with end of arrays low byte
BCC $B516       ;do out of memory error if less
STA $33         ;save bottom of string space low byte
STY $34         ;save bottom of string space high byte
STA $35         ;save string utility ptr low byte
STY $36         ;save string utility ptr high byte
TAX             ;copy low byte to X
PLA             ;get string length back
RTS
LDX #$10        ;error code $10, out of memory error
LDA $0F         ;get garbage collected flag
BMI $B4D2       ;if set then do error code X
JSR $B526       ;else go do garbage collection
LDA #$80        ;flag for garbage collected
STA $0F         ;set garbage collected flag
PLA             ;pull length
BNE $B4F6       ;go try again (loop always, length should never be = $00)


* = $B67A ; copy string from descriptor to utility pointer

LDY #$00        ;clear index
LDA ($6F),Y     ;get string length
PHA             ;save it
INY             ;increment index
LDA ($6F),Y     ;get string pointer low byte
TAX             ;copy to X
INY             ;increment index
LDA ($6F),Y     ;get string pointer high byte
TAY             ;copy to Y
PLA             ;get length back
STX $22         ;save string pointer low byte
STY $23         ;save string pointer high byte
                ;                store string from pointer to utility pointer
TAY             ;copy length as index
BEQ $B699       ;branch if null string
PHA             ;save length
DEY             ;decrement length/index
LDA ($22),Y     ;get byte from string
STA ($35),Y     ;save byte to destination
TYA             ;copy length/index
BNE $B690       ;loop if not all done yet
PLA             ;restore length
CLC             ;clear carry for add
ADC $35         ;add string utility ptr low byte
STA $35         ;save string utility ptr low byte
BCC $B6A2       ;branch if no rollover
INC $36         ;increment string utility ptr high byte
RTS

; do error #X then warm start, the error message vector is initialised to point here
* = $A43A

TXA             ;copy error number
ASL             ;*2
TAX             ;copy to index
LDA $A326,X     ;get error message pointer low byte
STA $22         ;save it
LDA $A327,X     ;get error message pointer high byte
STA $23         ;save it
JSR $FFCC       ;close input and output channels
LDA #$00        ;clear A
STA $13         ;clear current I/O channel, flag default
JSR $AAD7       ;print CR/LF
JSR $AB45       ;print "?"
LDY #$00        ;clear index
LDA ($22),Y     ;get byte from message
PHA             ;save status
AND #$7F        ;mask 0xxx xxxx, clear b7
JSR $AB47       ;output character
INY             ;increment index
PLA             ;restore status
BPL $A456       ;loop if character was not end marker
JSR $A67A       ;flush BASIC stack and clear continue pointer
LDA #$69        ;set " ERROR" pointer low byte
LDY #$A3        ;set " ERROR" pointer high byte


