*
* int sort_com(uint16_t argc, char *argv[], ...)
*
* Alle Parameter werden Åber den Stack Åbergeben!
* Die weiteren Parameter sind nur fÅr den Batch-Betrieb.
* Darf d0-d2/a0-a2 verÑndern.
*

sort_com:
 link     a6,#0
 movem.l  d4/d5/d7/a3/a4/a5,-(sp)
 lea      d+r_flag(pc),a2
 clr.w    (a2)					; clear R and C flags
* clr.b    r_flag
* clr.b    c_flag
 clr.w    sort_keypos-r_flag(a2)	; sort with column 0
 bra.b    sort_com_next_arg

* loop over all arguments

sort_com_arg_loop:
 addq.l   #4,ARGV(a6)			; advance argument pointer
 movea.l  ARGV(a6),a0			; get arguments pointer
 movea.l  (a0),a5				; a5 = argument
 lea      syntax_sorts(pc),a0
 cmpi.b   #'-',(a5)+
 bne.b    sort_com_error
* switch found
 cmpi.b   #'1',(a5)
 blt.b    sort_4
 cmpi.b   #'9',(a5)
 bgt.b    sort_4
* numerical argument, get sort column
 move.l   a5,a0
 bsr      str_toi				; get unsigned number
 subq.w   #1,d0
 blt.b    sort_4				; ignore if less than 1
 move.w   d0,sort_keypos-r_flag(a2)
sort_4:
 move.b   (a5),d0
 bsr      d0_upper
 cmpi.b   #'R',d0
 bne.b    sort_5
 st       (a2)					; clear r_flag
sort_5:
 cmpi.b   #'C',d0
 bne.b    sort_com_next_arg
 st       c_flag-r_flag(a2)

* next loop over arguments

sort_com_next_arg:
 subq.w   #1,ARGC(a6)
 bne.b    sort_com_arg_loop

* end loop over arguments: get memory

 bsr      memavail
 move.l   d0,d5
 beq.b    sort_com_mem_err
* move.l   d5,d0
 bsr      alloc_tpa                ; get memory, print error message in case of failure and set Z=1
 beq.b    sort_50				; give up
 move.l   d0,a3				; a3 = pointer to memory block
 cmpi.l   #1000,d5
 bcc.b    sort_com_mem_allocated

* no memory available or less than 1000 bytes

sort_com_mem_err:
 lea      sort_memerrs(pc),a0
sort_com_error:
 bsr      strcon
sort_50:
 bsr      inc_errlv
 bra      sort_com_tidy_up

* memory successfully allocated
* a3: memory block
* d5: its size in bytes

sort_com_mem_allocated:
* Zeigerpuffer a4 bis d4, Zeilenpuffer a5 bis d5
 movea.l  a3,a4				; a4: start of pointer buffer
* reserve 1/8 of allocated memory for pointers and the remaining for text
 move.l   d5,d4
 lsr.l    #3,d4				; d4 = blocksize / 8
 sub.l    d4,d5				; d5 = blocksize - blocksize / 8
 add.l    a4,d4				; d4 = end of pointers table
 move.l   d4,a5				; a5 = start of text buffer
 subq.l   #4,d4				; why decrease by 4?
 add.l    a5,d5				; d5 = end of text buffer
 subq.l   #4,d5				; why decrease by 4?
 clr.l    d7					; d7: number of lines read
 move.l   a5,(a4)+				; address of first line
 bra.b    sort_com_readline_next

* loop over lines to read, replace line endings with NUL

sort_com_loop_readline:
 move.l   a5,a0
 moveq    #1,d1
 moveq    #STDIN,d0				; get single byte from stdin to a5[]
 bsr      read
 bsr      fatal
 subq.l   #1,d0				; success?
 bne.b    sort_com_eof			; no, end-of-file
 cmpi.b   #$1a,(a5)				; ^Z (EOF) ?
 beq.b    sort_com_eof			; yes, end-of-file
 cmpi.b   #CR,(a5)				; skip CR
 beq.b    sort_com_readline_next
 cmpi.b   #LF,(a5)+				; skip LF
 bne.b    sort_com_readline_next
 clr.b    -1(a5)				; LF marks the end of the line, replace with NUL
 addq.l   #1,d7				; increase number of lines read
 move.l   a5,(a4)+				; address of next line

* next line to read

sort_com_readline_next:
 cmpa.l   d5,a5				; text buffer overflow?
 bcc.b    sort_com_mem_err		; yes, too much text
 cmpa.l   d4,a4				; line pointer buffer overflow
 bcc.b    sort_com_mem_err		; yes, too many lines
 bra.b    sort_com_loop_readline	; continue with next line

* End of read-line loop. File read, start sorting

sort_com_eof:
 clr.b    (a5)					; terminate last string read
 move.l   a3,-(sp)				; line pointer table
 move.l   d7,-(sp)				; number of lines
 pea      cmps(pc)				; comparison callback function
 bsr      shellsort				; sort the lines
 adda.w   #12,sp
 tst.l    d7					; was there anything to sort?
 beq.b    sort_com_tidy_up		; no

* write line loop

sort_com_writeline_loop:
 move.l   (a3)+,a0				; a0 = sorted line
 bsr      strstdout                ; print sorted line to stdout
 bsr      crlf_stdout              ; ... and add CR/LF
* d0 = 1 (OK) or 0 (volume full) or negative (fatal error)
 bsr      fatal				; d0 < 0 => fatal error, do not return
 lea      sort_volume_full_s(pc),a0
 subq.l   #1,d0				; crlf was successful?
 bne      sort_com_error			; no, volume is full
 subq.l   #1,d7
 bne.b    sort_com_writeline_loop

* free memory, if allocated, and leave

sort_com_tidy_up:
 bsr      free_tpa				; preserves d0
sort_end:
 movem.l  (sp)+,a5/a4/a3/d7/d5/d4
 unlk     a6
 rts
