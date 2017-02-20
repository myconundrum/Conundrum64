; Welcome Message

VIDMEM = $0400
WELSTR = $0900

LDY WELSTR  ; load base string
LDX #$01    ; start at next offset.
LOOP: LDA WELSTR,X
STA VIDMEM,X
INX
DEY
BNE LOOP
RTS

* = $0900
.STRING "Welcome. Ready"






