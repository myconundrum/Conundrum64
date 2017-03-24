;put Hello World using screen address
		;rather than jsr $e716
			
		processor 6502
		org $1000
		
		jsr $E544	;clear the screen
	
		;set the screen color memory
		lda #0		;black in A
		sta $d800	;black in d800 color memory
		sta $d801	;black in colour memory+1
		sta $d802	;black in colour memory+2
		sta $d803	;black in colour memory+3
		sta $d804	;black in colour memory+4
		sta $d806	;black in colour memory+6
		sta $d807	;black in colour memory+7
		sta $d808	;black in colour memory+8
		sta $d809	;black in colour memory+9
		sta $d80A	;black in colour memory+A
		
		;enter HELLO WORLD in screen memory
		lda #8		;set2 H
		sta $400	;H in 400 screen memory
				
		lda #5		;set2 E
		sta $401	;E in 400+1 screen memory
		

		lda #12		;set2 L
		sta $402	;E in 400+2 screen memory
		
		
		lda #12		;set2 L
		sta $403	;E in 400+3 screen memory
		

		lda #15		;set2 O
		sta $404	;E in 400+4 screen memory
		
		lda #23		;W
		sta $406
		
		lda #15		;O
		sta $407
		
		lda #18		;R
		sta $408
		
		lda #12		;L
		sta $409
		
		lda #4		;D
		sta $40A
		
		rts