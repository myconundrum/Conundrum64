;
; MRW File
; test whether check cart is working correctly on startup. 
;

MSG 		= $8200
VIDMEM 		= $0400


* = $4200
LDX MSG 	 		; load base string
LDY #$01    		; start at next offset.
LOOP: LDA MSG,X
STA VIDMEM,X		; move string to video memory
INY
DEX  
BNE LOOP

* = $8000
.DB $00 $42
.DB $00 $00 $C3 $C2 $CD $38 $30 


* = MSG
.STRING "Cartridge Start!"
