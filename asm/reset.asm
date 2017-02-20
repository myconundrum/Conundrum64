; MOS 6510 System Reset routine[3]
; Reset vector (Kernal address $FFFC) points here.
; 
; If cartridge is detected then cartridge cold start routine is activated.
; If no cartridge is detected then I/O and memory are initialised and BASIC cold start routine is activated.


;
; MRW Routine
;
SETBRK = $FB00
BRKVECL = $FFFE
BRKVECH = $FFFF
RDYMSG 		= $FC00
VIDMEM 		= $0400
WELSTR 		= $F000
ROMCART 	= $8000

;kernal vectors
* = $FD30
.DB $31 $EA                    ;$0314 IRQ vector
.DB $66 $FE                    ;$0316 BRK vector
.DB $47 $FE                    ;$0318 NMI vector
.DB $4A $F3                    ;$031A open a logical file
.DB $91 $F2                    ;$031C close a specified logical file
.DB $0E $F2                    ;$031E open channel for input
.DB $50 $F2                    ;$0320 open channel for output
.DB $33 $F3                    ;$0322 close input and output channels
.DB $57 $F1                    ;$0324 input character from channel
.DB $CA $F1                    ;$0326 output character to channel
.DB $ED $F6                    ;$0328 scan stop key
.DB $3E $F1                    ;$032A get character from the input device
.DB $2F $F3                    ;$032C close all channels and files
.DB $66 $FE                    ;$032E user function
                             ;   Vector to user defined command, currently points to BRK.
                             ;   This appears to be a holdover from PET days, when the built-in machine ;language monitor
                             ;   would jump through the $032E vector when it encountered a command that it did ;not
                             ;   understand, allowing the user to add new commands to the monitor.
                             ;   Although this vector is initialized to point to the routine called by ;STOP/;RESTORE and
                             ;   the BRK interrupt, and is updated by the kernal vector routine at $FD57, it no ;longer
                             ;   has any function.
.DB $A5 $F4                    ;$0330 load
.DB $ED $F5                    ;$0332 save


CSR = $FCE2
* = CSR			; COLD START RESET
LDX #$FF        	; 
SEI             	; set interrupt disable
TXS             	; transfer .X to stack
CLD             	; clear direction flag
JSR $FD02       	; check for cart
BNE $FCEF       	; .Z=0? then no cart detected
JMP ($8000)     	; direct to cartridge cold start via vector
STX $D016       	; sets bit 5 (MCM) off, bit 3 (38 cols) off
JSR $FDA3       	; initialise I/O
JSR $FD50       	; initialise memory
JSR $FD15       	; set I/O vectors ($0314..$0333) to kernal defaults
JSR $FF5B       	; more initialising... mostly set system IRQ to correct value and start
CLI             	; clear interrupt flag
JMP ($A000)     	; direct to BASIC cold start via vector
;JSR $FC00			; MRW REady message


* = $FD02			; CHECK CARTRIDGE ROUTINE
LDX #$05        	; five characters to test
CARTL: LDA $FD0F,X  ; get test character
CMP $8003,X     	; compare wiith byte in ROM space
BNE $FD0F       	; exit if no match
DEX             	; decrement index
BNE CARTL       	; loop if not all done
RTS					; return



* = $FDA3			; INITIALIZE I/O AFTER RESET

LDA #$7F        ;disable all interrupts
STA $DC0D       ;save VIA 1 ICR
STA $DD0D       ;save VIA 2 ICR
STA $DC00       ;save VIA 1 DRA, keyboard column drive
LDA #$08        ;set timer single shot
STA $DC0E       ;save VIA 1 CRA
STA $DD0E       ;save VIA 2 CRA
STA $DC0F       ;save VIA 1 CRB
STA $DD0F       ;save VIA 2 CRB
LDX #$00        ;set all inputs
STX $DC03       ;save VIA 1 DDRB, keyboard row
STX $DD03       ;save VIA 2 DDRB, RS232 port
STX $D418       ;clear the volume and filter select register
DEX             ;set X = $FF
STX $DC02       ;save VIA 1 DDRA, keyboard column
LDA #$07        ;DATA out high, CLK out high, ATN out high, RE232 Tx DATA
                ;high, video address 15 = 1, video address 14 = 1
STA $DD00       ;save VIA 2 DRA, serial port and video address
LDA #$3F        ;set serial DATA input, serial CLK input
STA $DD02       ;save VIA 2 DDRA, serial port and video address
LDA #$E7        ;set 1110 0111, motor off, enable I/O, enable KERNAL,
		        ;enable BASIC
STA $01         ;save the 6510 I/O port
LDA #$2F        ;set 0010 1111, 0 = input, 1 = output
STA $00         ;save the 6510 I/O port direction register
LDA $02A6       ;get the PAL/NTSC flag
BEQ $FDEC       ;if NTSC go set NTSC timing else set PAL timing
LDA #$25
STA $DC04       ;save VIA 1 timer A low byte
LDA #$40
JMP $FDF3
LDA #$95
STA $DC04       ;save VIA 1 timer A low byte
LDA #$42
STA $DC05       ;save VIA 1 timer A high byte
JMP $FF6E


* = $FF6E 		; CONTINUED INITIALIZATION
LDA #$81        ;enable timer A interrupt
STA $DC0D       ;save VIA 1 ICR
LDA $DC0E       ;read VIA 1 CRA
AND #$80        ;mask x000 0000, TOD clock
ORA #$11        ;mask xxx1 xxx1, load timer A, start timer A
STA $DC0E       ;save VIA 1 CRA
JMP $EE8E       ;set the serial clock out low and return


* = $EE8E		; SET SERIAL CLOCK LOW
LDA $DD00       ; read VIA 2 DRA, serial port and video address
ORA #$10        ; mask xxx1 xxxx, set serial clock out low
STA $DD00       ; save VIA 2 DRA, serial port and video address
RTS


* = $FD50    	; TEST/INITIALIZE MEMORY
LDA #$00        ; clear A
TAY             ; clear index
STA $0002,Y     ; clear page 0, don't do $0000 or $0001
STA $0200,Y     ; clear page 2
STA $0300,Y     ; clear page 3
INY             ; increment index
BNE $FD53       ; loop if more to do
LDX #$3C        ; set cassette buffer pointer low byte
LDY #$03        ; set cassette buffer pointer high byte
STX $B2         ; save tape buffer start pointer low byte
STY $B3         ; save tape buffer start pointer high byte
TAY             ; clear Y
LDA #$03        ; set RAM test pointer high byte
STA $C2         ; save RAM test pointer high byte
INC $C2         ; increment RAM test pointer high byte
LDA ($C1),Y
TAX
LDA #$55
STA ($C1),Y
CMP ($C1),Y
BNE $FD88
ROL  ; test
STA ($C1),Y
CMP ($C1),Y
BNE $FD88
TXA
STA ($C1),Y
INY
BNE $FD6E
BEQ $FD6C
TYA
TAX
LDY $C2
CLC
JSR $FE2D       ;set the top of memory
LDA #$08
STA $0282       ;save the OS start of memory high byte
LDA #$04
STA $0288       ;save the screen memory page
RTS

* = $FE2D 		; SET TOP OF MEMORY
STX $0283       ; set memory top low byte
STY $0284       ; set memory top high byte
RTS



* = $FD15			; RESET DEFAULT I/O VECTORS
LDX #$30        	; pointer to vector table low byte
LDY #$FD        	; pointer to vector table high byte
CLC             	; flag set vectors

* = $FD1A   	; set/read vectored I/O from (XY), Cb = 1 to read, Cb = 0 to set
STX $C3         ; save pointer low byte
STY $C4         ; save pointer high byte
LDY #$1F        ; set byte count
LDA $0314,Y     ; read vector byte from vectors
BCS $FD27       ; branch if read vectors
LDA ($C3),Y     ; read vector byte from (XY)
STA ($C3),Y     ; save byte to (XY)
STA $0314,Y     ; save byte to vector
DEY             ; decrement index
BPL $FD20       ; loop if more to do
RTS

*=$FF5B ; INITIALIZE VIC AND SCREEN EDITOR

JSR $E518       ; initialise the screen and keyboard
LDA $D012       ; read the raster compare register
BNE $FF5E       ; loop if not raster line $00
LDA $D019       ; read the vic interrupt flag register
AND #$01        ; mask the raster compare flag
STA $02A6       ; save the PAL/NTSC flag
JMP $FDDD
LDA #$81        ; enable timer A interrupt
STA $DC0D       ; save VIA 1 ICR
LDA $DC0E       ; read VIA 1 CRA
AND #$80        ; mask x000 0000, TOD clock
ORA #$11        ; mask xxx1 xxx1, load timer A, start timer A
STA $DC0E       ; save VIA 1 CRA
JMP $EE8E       ; set the serial clock out low and return



* = $FDDD ; INITIALIZE TAL1/TAH1 fpr 1/60 of a second
LDA $02A6
BEQ $FDEC
LDA #$25
STA $DC04
LDA #$40
JMP $FDF3
LDA #$95
STA $DC04
LDA #$42
STA $DC05
JMP $FF6E

; initalise file name parameters

STA $B7
STX $BB
STY $BC
RTS

* = $FF81	
JMP $FF5B 		; initalize vic and screen editor

* = $E518		; INITIALIZE SCREEN AND KEYBOARD
JSR $E5A0       ; initialise the vic chip
LDA #$00        ; clear A
STA $0291       ; clear the shift mode switch
STA $CF         ; clear the cursor blink phase
LDA #$48        ; get the keyboard decode logic pointer low byte
STA $028F       ; save the keyboard decode logic pointer low byte
LDA #$EB        ; get the keyboard decode logic pointer high byte
STA $0290       ; save the keyboard decode logic pointer high byte
LDA #$0A        ; set the maximum size of the keyboard buffer
STA $0289       ; save the maximum size of the keyboard buffer
STA $028C       ; save the repeat delay counter
LDA #$0E        ; set light blue
STA $0286       ; save the current colour code
LDA #$04        ; speed 4
STA $028B       ; save the repeat speed counter
LDA #$0C        ; set the cursor flash timing
STA $CD         ; save the cursor timing countdown
STA $CC         ; save the cursor enable, $00 = flash cursor

* = $E544; CLEAR THE SCREEN
LDA $0288       ; get the screen memory page
ORA #$80        ; set the high bit, flag every line is a logical line start
TAY             ; copy to Y
LDA #$00        ; clear the line start low byte
TAX             ; clear the index
STY $D9,X       ; save the start of line X pointer high byte
CLC             ; clear carry for add
ADC #$28        ; add the line length to the low byte
BCC $E555       ; if no rollover skip the high byte increment
INY             ; else increment the high byte
INX             ; increment the line index
CPX #$1A        ; compare it with the number of lines + 1
BNE $E54D       ; loop if not all done
LDA #$FF        ; set the end of table marker
STA $D9,X       ; mark the end of the table
LDX #$18        ; set the line count, 25 lines to do, 0 to 24
JSR $E9FF       ; clear screen line X
DEX             ; decrement the count
BPL $E560       ; loop if more to do

* = $E566 		; home the cursor

LDY #$00        ; clear Y
STY $D3         ; clear the cursor column
STY $D6         ; clear the cursor row

* = $E56C 		;set screen pointers for cursor row, column

LDX $D6         ; get the cursor row
LDA $D3         ; get the cursor column
LDY $D9,X       ; get start of line X pointer high byte
BMI $E57C       ; if it is the logical line start continue
CLC             ; else clear carry for add
ADC #$28        ; add one line length
STA $D3         ; save the cursor column
DEX             ; decrement the cursor row
BPL $E570       ; loop, branch always
JSR $E9F0       ; fetch a screen address
LDA #$27        ; set the line length
INX             ; increment the cursor row
LDY $D9,X       ; get the start of line X pointer high byte
BMI $E58C       ; if logical line start exit
CLC             ; else clear carry for add
ADC #$28        ; add one line length to the current line length
INX             ; increment the cursor row
BPL $E582       ; loop, branch always
STA $D5         ; save current screen line length
JMP $EA24       ; calculate the pointer to colour RAM and return
CPX $C9         ; compare it with the input cursor row
BEQ $E598       ; if there just exit
JMP $E6ED       ; else go ??
RTS


* = $E9FF ; clear one screen line (MRW made here)

LDY #$27
JSR $E9F0
JSR $EA24
JSR $E4DA
LDA #$20
STA ($D1),Y
DEY
BPL $EA07
RTS
NOP


* = $E9F0 ; fetch screen addresses

LDA $ECF0,X
STA $D1
LDA $D9,X
AND #$03
ORA $0288
STA $D2
RTS

* = $EA24 ; set colour memory adress parallel to screen

LDA $D1
STA $F3
LDA $D2
AND #$03
ORA #$D8
STA $F4
RTS

* = $E4DA ; clear byte in color ram

LDA $0286
STA ($F3),Y
RTS



* = $E5A0 ; initialise vic chip
LDA #$03
STA $9A
LDA #$00
STA $99
LDX #$2F
LDA $ECB8,X
STA $CFFF,X
DEX
BNE $E5AA
RTS


* = $EA24 ; set colour memory adress parallel to screen

LDA $D1
STA $F3
LDA $D2
AND #$03
ORA #$D8
STA $F4
RTS


;
; MRW Routine. Setup Break routine.
;
* = $FB00
LDA #$10
STA BRKVECL
LDA #$FB
STA BRKVECH
RTS
;
; Just cycle on brk. 
;
* = $FB10
JMP $FB10

;
; Just a ready message. 
;
* = $FC00
LDX WELSTR  		; load base string
LDY #$01    		; start at next offset.
LOOP: LDA WELSTR,X
STA VIDMEM,X		; move string to video memory
INY
DEX  
BNE LOOP
JSR $FB00
RTS


* = $FFCF

							; input character from channel

                            ; this routine will get a byte of data from the channel already set up as the input
                        	;channel by the CHKIN routine, $FFC6.
                            ;If CHKIN, $FFC6, has not been used to define another input channel the data is
                            ;expected to be from the keyboard. the data byte is returned in the accumulator. the
                            ;channel remains open after the call.

                            ;input from the keyboard is handled in a special way. first, the cursor is turned on
                            ;and it will blink until a carriage return is typed on the keyboard. all characters
                            ;on the logical line, up to 80 characters, will be stored in the BASIC input buffer.
                            ;then the characters can be returned one at a time by calling this routine once for
                            ;each character. when the carriage return is returned the entire line has been
                            ;processed. the next time this routine is called the whole process begins again.
JMP ($0324)     ; do input character from channel




* = $FF9C ;read/set the bottom of memory

                                ;this routine is used to read and set the bottom of RAM. When this routine is
                                ;called with the carry bit set the pointer to the bottom of RAM will be loaded
                                ;into XY. When this routine is called with the carry bit clear XY will be saved ;as
                                ;the bottom of memory pointer changing the bottom of memory.
JMP $FE34       ;read/set the bottom of memory

* = $FE34			;read/set the bottom of memory, Cb = 1 to read, Cb = 0 to set

BCC $FE3C       ;if Cb clear go set the bottom of memory
LDX $0281       ;get the OS start of memory low byte
LDY $0282       ;get the OS start of memory high byte
STX $0281       ;save the OS start of memory low byte
STY $0282       ;save the OS start of memory high byte
RTS



* = $FF99 ;read/set the top of memory

                                ;this routine is used to read and set the top of RAM. When this routine is ;called
                                ;with the carry bit set the pointer to the top of RAM will be loaded into XY. ;When
                                ;this routine is called with the carry bit clear XY will be saved as the top of
                                ;memory pointer changing the top of memory.
JMP $FE25       				;read/set the top of memory



* = $FE25 						;read/set the top of memory, Cb = 1 to read, Cb = 0 to set

BCC $FE2D       ;if Cb clear go set the top of memory

;read the top of memory

LDX $0283       ;get memory top low byte
LDY $0284       ;get memory top high byte

;set the top of memory

STX $0283       ;set memory top low byte
STY $0284       ;set memory top high byte
RTS




;
; mrw - just a test message.
;

* = $F000
.STRING "Welcome. Ready"

;
; bytes used to test cartridge signature (mrw added for rom image) 
;
* = $FD10
.DB $C3 $C2 $CD $38 $30s





