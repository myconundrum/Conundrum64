; MOS 6510 System Reset routine[3]
; Reset vector (Kernal address $FFFC) points here.
; 
; If cartridge is detected then cartridge cold start routine is activated.
; If no cartridge is detected then I/O and memory are initialised and BASIC cold start routine is activated.

;FCE2   A2 FF      LDX #$FF        ; 
;FCE4   78         SEI             ; set interrupt disable
;FCE5   9A         TXS             ; transfer .X to stack
;FCE6   D8         CLD             ; clear direction flag
;FCE7   20 02 FD   JSR $FD02       ; check for cart
;FCEA   D0 03      BNE $FCEF       ; .Z=0? then no cart detected
;FCEC   6C 00 80   JMP ($8000)     ; direct to cartridge cold start via vector
;FCEF   8E 16 D0   STX $D016       ; sets bit 5 (MCM) off, bit 3 (38 cols) off
;FCF2   20 A3 FD   JSR $FDA3       ; initialise I/O
;FCF5   20 50 FD   JSR $FD50       ; initialise memory
;FCF8   20 15 FD   JSR $FD15       ; set I/O vectors ($0314..$0333) to kernal defaults
;FCFB   20 5B FF   JSR $FF5B       ; more initialising... mostly set system IRQ to correct value and start
;FCFE   58         CLI             ; clear interrupt flag
;FCFF   6C 00 A0   JMP ($A000)     ; direct to BASIC cold start via vector


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




* = $FCE2		; COLD START RESET
LDX #$FF        ; 
SEI             ; set interrupt disable
TXS             ; transfer .X to stack
CLD             ; clear direction flag
JSR $FD02       ; check for cart
BNE $FCEF       ; .Z=0? then no cart detected
JMP ($8000)     ; direct to cartridge cold start via vector
STX $D016       ; sets bit 5 (MCM) off, bit 3 (38 cols) off
;JSR $FDA3       ; initialise I/O
;JSR $FD50       ; initialise memory
;JSR $FD15       ; set I/O vectors ($0314..$0333) to kernal defaults
;JSR $FF5B       ; more initialising... mostly set system IRQ to correct value and start
CLI             ; clear interrupt flag
;JMP ($A000)     ; direct to BASIC cold start via vector
JSR $FC00		; MRW REady message
JSR $FB00		; MRW Set Brk.


* = $FD02			; CHECK CARTRIDGE ROUTINE
LDX #$05        	; five characters to test
CARTL: LDA $FD0F,X  ; get test character
CMP $8003,X     	; compare wiith byte in ROM space
BNE $FD0F       	; exit if no match
DEX             	; decrement index
BNE CARTL       	; loop if not all done
RTS					; return



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
.DB $C3 $C2 $CD $38 $30 





