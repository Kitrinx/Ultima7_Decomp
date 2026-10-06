; Serpent Isle INTRO.EXE, one module of resident segment 70 (file offsets 0x01281e to 0x0134cd, 3247 bytes).
; Turbo Assembler 2.51 /m /w+ /ml rebuilds it byte for byte.

; Third-party source: Miles Design AIL V2.00 of 24-Sep-91, kept as its author wrote it;
; comment boxes converted from code page 437 to UTF-8.
;████████████████████████████████████████████████████████████████████████████
;██                                                                        ██
;██   AIL.ASM                                                              ██
;██                                                                        ██
;██   IBM Audio Interface Library -- Application Program Interface module  ██
;██                                                                        ██
;██   Version 2.00 of 24-Sep-91: Initial V2.X version (derived from V1.02) ██
;██                                                                        ██
;██   8086 ASM source compatible with Turbo Assembler v2.0 or later        ██
;██   C function prototypes in AIL.H                                       ██
;██   Author: John Miles                                                   ██
;██                                                                        ██
;████████████████████████████████████████████████████████████████████████████
;██                                                                        ██
;██   Copyright (C) 1991, 1992 Miles Design, Inc.                          ██
;██                                                                        ██
;██   Miles Design, Inc.                                                   ██
;██   10926 Jollyville #308                                                ██
;██   Austin, TX 78759                                                     ██
;██   (512) 345-2642 / FAX (512) 338-9630 / BBS (512) 454-9990             ██
;██                                                                        ██
;████████████████████████████████████████████████████████████████████████████

                MODEL MEDIUM,C          ;Procedures far, data near by default
                LOCALS __               ;Enable local labels with __ prefix
                JUMPS                   ;Enable auto jump sizing

                ;
                ;Macros, internal equates
                ;

                INCLUDE ail.inc         ;define driver procedure call numbers
                INCLUDE ail.mac         ;general-use macros

                .CODE

                ;
                ;Process services
                ;                

                PUBLIC AIL_startup
                PUBLIC AIL_shutdown
                PUBLIC AIL_register_timer
                PUBLIC AIL_set_timer_period
                PUBLIC AIL_set_timer_frequency
                PUBLIC AIL_set_timer_divisor
                PUBLIC AIL_interrupt_divisor
                PUBLIC AIL_start_timer
                PUBLIC AIL_start_all_timers
                PUBLIC AIL_stop_timer
                PUBLIC AIL_stop_all_timers
                PUBLIC AIL_release_timer_handle
                PUBLIC AIL_release_all_timers

                ;
                ;Installation services
                ;

                PUBLIC AIL_register_driver
                PUBLIC AIL_release_driver_handle
                PUBLIC AIL_describe_driver
                PUBLIC AIL_detect_device
                PUBLIC AIL_init_driver
                PUBLIC AIL_shutdown_driver
                
                ;
                ;Extended MIDI (XMIDI) performance services
                ;

                PUBLIC AIL_state_table_size
                PUBLIC AIL_register_sequence
                PUBLIC AIL_release_sequence_handle

                PUBLIC AIL_default_timbre_cache_size
                PUBLIC AIL_define_timbre_cache
                PUBLIC AIL_timbre_request
                PUBLIC AIL_install_timbre
                PUBLIC AIL_protect_timbre
                PUBLIC AIL_unprotect_timbre
                PUBLIC AIL_timbre_status

                PUBLIC AIL_start_sequence
                PUBLIC AIL_stop_sequence
                PUBLIC AIL_resume_sequence
                PUBLIC AIL_sequence_status
                PUBLIC AIL_relative_volume
                PUBLIC AIL_relative_tempo
                PUBLIC AIL_set_relative_volume
                PUBLIC AIL_set_relative_tempo
                PUBLIC AIL_beat_count
                PUBLIC AIL_measure_count
                PUBLIC AIL_branch_index

                PUBLIC AIL_controller_value
                PUBLIC AIL_set_controller_value
                PUBLIC AIL_channel_notes
                PUBLIC AIL_send_channel_voice_message
                PUBLIC AIL_send_sysex_message
                PUBLIC AIL_write_display
                PUBLIC AIL_install_callback
                PUBLIC AIL_cancel_callback

                PUBLIC AIL_lock_channel
                PUBLIC AIL_map_sequence_channel
                PUBLIC AIL_true_sequence_channel
                PUBLIC AIL_release_channel

                ;
                ;Digital performance services
                ;

                PUBLIC AIL_index_VOC_block
                PUBLIC AIL_register_sound_buffer
                PUBLIC AIL_sound_buffer_status
                PUBLIC AIL_play_VOC_file
                PUBLIC AIL_VOC_playback_status
                PUBLIC AIL_start_digital_playback
                PUBLIC AIL_stop_digital_playback
                PUBLIC AIL_pause_digital_playback
                PUBLIC AIL_resume_digital_playback
                PUBLIC AIL_set_digital_playback_volume
                PUBLIC AIL_digital_playback_volume
                PUBLIC AIL_set_digital_playback_panpot
                PUBLIC AIL_digital_playback_panpot

                ;
                ;Local data
                ;
                
BIOS_H          equ 16                  ;Handle to BIOS default timer

active_timers   dw ?                    ;# of timers currently registered
timer_busy      dw 0                    ;Reentry flag for INT 8 handler

timer_callback  dd 17 dup (?)           ;Callback function addrs for timers
callback_ds     dw 17 dup (?)           ;Default data segments for callbacks
timer_status    dw 17 dup (?)           ;Status of timers (0=free 1=off 2=on)
timer_elapsed   dd 17 dup (?)           ;Modified DDA error counts for timers
timer_value     dd 17 dup (?)           ;Modified DDA limit values for timers
timer_period    dd ?                    ;Modified DDA increment for timers

bios_callback   dd ?
current_timer   dw ?                    
temp_period     dd ?
PIT_divisor     dw ?

index_base      dd 16 dup (?)           ;Driver table base addresses
assigned_timer  dw 16 dup (?)           ;Timers assigned to drivers
driver_active   dw 16 dup (?)

drvproc         dd ?
cur_drvr        dw ?
rtn_off         dw ?
rtn_seg         dw ?
timer_handle    dw ?

drvr_desc       STRUC
min_API_version dw ?
drvr_type       dw ?
data_suffix     db 4 dup (?)
dev_names       dd ?
def_IO          dw ?
def_IRQ         dw ?
def_DMA         dw ?
def_DRQ         dw ?
svc_rate        dw ?
dsp_size        dw ?
                ENDS

                ALIGN 2
stack_check     db 'Test'              ;(Used for stack overflow checking)
                dw 256 dup (?)         ;512-byte interrupt stack used by
intstack        LABEL WORD             ;default -- can be increased if needed

old_ss          dw ?
old_sp          dw ?

current_rev     dw 200

;*****************************************************************************
;*                                                                           *
;* Internal procedures                                                       *
;*                                                                           *
;*****************************************************************************

find_proc       PROC                    ;Return addr of function AX in driver
                                        ;BX (Note: reentrant!)
                cmp bx,16
                jae __bad_handle        ;exit sanely if handle invalid
                shl bx,1                ;(a legitimate action for apps)
                shl bx,1
                les bx,index_base[bx]   ;ES:BX -> driver procedure table
                mov cx,es
                or cx,bx
                jz __bad_handle         ;handle -> unreg'd driver, exit

__find_proc:    mov cx,es:[bx]          ;search for requested function in 
                cmp cx,ax               ;driver procedure table
                je __found
                add bx,4
                cmp cx,-1               
                jne __find_proc

                mov ax,0                ;return 0: function not available
                mov dx,0
                ret

__bad_handle:   mov ax,0                ;return 0: invalid driver handle
                mov dx,0
                ret                                                     

__found:        mov ax,es:[bx+2]        ;get offset from start of driver
                mov dx,es               ;get segment of driver (org = 0)
                ret

                ENDP

;*****************************************************************************
call_driver     PROC                    ;Call function AX in specified driver
                                        ;(Warning: re-entrant procedure!)
                mov bx,sp
                mov bx,ss:[bx+4]        ;get handle

                call find_proc

                cmp ax,0
                jne __do_call
                cmp dx,0
                je __invalid_call

__do_call:      push dx                 ;call driver function via stack 
                push ax
                retf                    

__invalid_call: ret                     ;return DX:AX = 0 if call failed

                ENDP

;*****************************************************************************
API_timer       PROC                    ;API INT 8 dispatcher

                cmp timer_busy,0
                jne __exit
                mov timer_busy,1

                cld
                push ax
                push bx
                push cx
                push dx
                push si
                push di
                push bp
                push es
                push ds

                mov old_ss,ss
                mov old_sp,sp

                mov ax,cs               ;switch to internal stack if INT 8
                mov ss,ax               ;in use
                lea sp,intstack

                mov current_timer,0
__for_timer:    mov si,current_timer    ;for timer = 0 to 16
                shl si,1
                cmp timer_status[si],2  ;is timer "running"?
                jne __next_timer        ;no, go on to the next one
                mov ds,callback_ds[si]  ;else load callback's data segment...
                shl si,1
                
                mov ax,WORD PTR timer_elapsed[si]
                mov dx,WORD PTR timer_elapsed[si]+2

                add ax,WORD PTR timer_period
                adc dx,WORD PTR timer_period+2

                cmp dx,WORD PTR timer_value[si]+2
                jb __dec_timer
                ja __timer_tick
                cmp ax,WORD PTR timer_value[si]
                jae __timer_tick

__dec_timer:    mov WORD PTR timer_elapsed[si],ax
                mov WORD PTR timer_elapsed[si]+2,dx
                jmp __next_timer

__timer_tick:   sub ax,WORD PTR timer_value[si]
                sbb dx,WORD PTR timer_value[si]+2

                mov WORD PTR timer_elapsed[si],ax
                mov WORD PTR timer_elapsed[si]+2,dx

                call timer_callback[si] ;DDA timer expired, call timer proc

__next_timer:   inc current_timer       ;(may be externally set to -1 to 
                cmp current_timer,16    ; cancel further callbacks)
                jbe __for_timer

                mov ss,old_ss
                mov sp,old_sp

                pop ds
                pop es
                pop bp
                pop di
                pop si
                pop dx
                pop cx
                pop bx
                pop ax

                mov timer_busy,0

__exit:         push ax
                mov al,20h       
                out 20h,al
                pop ax

                cmp WORD PTR stack_check,'eT'
                jne __stack_fault
                cmp WORD PTR stack_check+2,'ts'
                jne __stack_fault

                iret             

__stack_fault:  sti                     ;(Increase size of internal stack if
                int 3                   ;application breaks or "hangs" here)
                jmp __stack_fault       

                ENDP

;*****************************************************************************
init_DDA_arrays PROC                    ;Initialize timer DDA counters
                USES ds,si,di

                pushf
                cli

                cld
                mov WORD PTR timer_period,-1
                mov WORD PTR timer_period+2,-1
                
                push cs
                pop es
                mov di,OFFSET timer_status
                mov cx,17
                mov ax,0
                rep stosw               ;mark all timer handles "free"

                mov di,OFFSET timer_elapsed
                mov cx,17*2
                rep stosw
                                                              
                mov di,OFFSET timer_value
                mov cx,17*2
                rep stosw

                POP_F
                ret
                ENDP

;*****************************************************************************
bios_caller     PROC                    ;Call old INT8 handler to maintain RTC

                pushf
                call DWORD PTR bios_callback

                ret
                ENDP

;*****************************************************************************
hook_timer_process PROC                 ;Take over default BIOS INT 8 handler
                USES ds,si,di

                pushf
                cli
                
                mov ax,3508h            ;get current INT 8 vector and save it
                int 21h                 ;as reserved timer function (stopped)

                mov WORD PTR bios_callback,bx
                mov WORD PTR bios_callback+2,es

                mov bx,OFFSET bios_caller
                mov WORD PTR timer_callback[BIOS_H*4],bx
                mov WORD PTR timer_callback[BIOS_H*4]+2,cs
                mov timer_status[BIOS_H*2],1

                mov ax,cs
                mov ds,ax
                mov dx,OFFSET API_timer

                mov ax,2508h            ;replace default handler with API task
                int 21h                 ;manager

                POP_F
                ret
                ENDP

;*****************************************************************************
unhook_timer_process PROC               ;Restore default BIOS INT 8 handler
                USES ds,si,di

                pushf
                cli

                mov current_timer,-1    ;disallow any further callbacks

                mov dx,WORD PTR bios_callback
                mov ds,WORD PTR bios_callback+2

                mov ax,2508h
                int 21h

                POP_F
                ret
                ENDP

;*****************************************************************************
set_PIT_divisor PROC Divisor            ;Set 8253 Programmable Interval Timer
                USES ds,si,di           ;to desired IRQ 0 (INT 8) interval

                pushf
                cli

                mov al,36h
                out 43h,al
                mov ax,[Divisor]        ;PIT granularity = 1/1193181 sec.
                mov PIT_divisor,ax
                jmp $+2
                out 40h,al
                mov al,ah
                jmp $+2
                out 40h,al

                POP_F
                ret
                ENDP

;*****************************************************************************
set_PIT_period  PROC Period             ;Set 8253 Programmable Interval Timer
                USES ds,si,di           ;to desired period in microseconds
                   
                mov ax,0                ;special case: no rounding error
                cmp [Period],54925      ;if period=55 msec. BIOS default value
                jae __set_PIT

                mov ax,[Period]
                mov bx,8381             ;PIT granularity = .83809532 uS
                mov cx,10000
                mul cx
                div bx

__set_PIT:      call set_PIT_divisor C,ax

                ret
                ENDP

;*****************************************************************************
ul_divide       PROC Num:DWORD,Den:DWORD
                USES ds,si,di

                mov ax,WORD PTR [Num]
                mov dx,WORD PTR [Num]+2
                mov bx,WORD PTR [Den]
                mov cx,WORD PTR [Den]+2

                or cx,cx
                jne __long_div
                or dx,dx
                je __short_div
                or bx,bx
                je __short_div

__long_div:     mov bp,cx
                mov cx,20h
                xor di,di
                xor si,si
                
__div_loop:     shl ax,1
                rcl dx,1
                rcl si,1
                rcl di,1
                cmp di,bp
                jb __cont_loop
                ja __bump_quot
                cmp si,bx
                jb __cont_loop
__bump_quot:    sub si,bx
                sbb di,bp
                inc ax
__cont_loop:    loop __div_loop
                jmp __end_div

__short_div:    div bx
                xor dx,dx

__end_div:      ret

                ENDP

;*****************************************************************************
program_timers  PROC                    ;Establish timer interrupt rates
                USES ds,si,di           ;based on fastest active timer

                pushf
                cli                     ;non-reentrant, don't interrupt

                cld
                mov WORD PTR temp_period,-1
                mov WORD PTR temp_period+2,-1

                mov si,0
__for_timer:    mov bx,si               ;find fastest active timer....
                shl bx,1
                cmp timer_status[bx],0  ;timer active (registered)?
                je __next_timer         ;no, skip it
                shl bx,1
                mov ax,WORD PTR timer_value[bx]
                mov dx,WORD PTR timer_value[bx]+2

                cmp dx,WORD PTR temp_period+2
                jb __set_temp
                ja __next_timer
                cmp ax,WORD PTR temp_period
                jae __next_timer

__set_temp:     mov WORD PTR temp_period,ax
                mov WORD PTR temp_period+2,dx

__next_timer:   inc si
                cmp si,16               ;(include BIOS reserved timer)
                jbe __for_timer        

                mov ax,WORD PTR temp_period
                mov dx,WORD PTR temp_period+2

                cmp ax,WORD PTR timer_period
                jne __must_reset
                cmp dx,WORD PTR timer_period+2
                je __no_change          ;current rate hasn't changed, exit

__must_reset:   mov current_timer,-1    ;else set new base timer rate
                                        ;(slowest possible base = 54 msec!)
                mov WORD PTR timer_period,ax
                mov WORD PTR timer_period+2,dx  

                call set_PIT_period C,ax

                push cs
                pop es
                mov di,OFFSET timer_elapsed
                mov cx,17*2
                mov ax,0                ;reset ALL elapsed counters to 0 uS
                rep stosw

__no_change:    
                POP_F
                ret
                ENDP

;*****************************************************************************
;*                                                                           *
;* Process services                                                          *
;*                                                                           *
;*****************************************************************************

AIL_startup     PROC                    ;Initialize AIL API
                USES ds,si,di

                pushf
                cli

                mov active_timers,0     ;# of registered timers
                mov timer_busy,0        ;timer re-entrancy protection

                cld
                mov ax,cs
                mov es,ax
                lea di,index_base
                mov cx,16*2
                mov ax,0
                rep stosw
                lea di,assigned_timer
                mov cx,16
                mov ax,-1
                rep stosw
                lea di,driver_active
                mov cx,16
                mov ax,0
                rep stosw

                POP_F    
                ret
                ENDP

;*****************************************************************************
AIL_shutdown    PROC SignOff:FAR PTR    ;Quick shutdown of all AIL resources
                USES ds,si,di

                pushf
                cli

                mov cur_drvr,0
__for_slot:     mov si,cur_drvr
                shl si,1
                mov dx,assigned_timer[si]
                shl si,1 
                mov ax,WORD PTR index_base[si]   
                or ax,WORD PTR index_base[si+2]
                jz __next_slot          ;no driver installed, skip slot

                cmp dx,-1
                je __shut_down          ;no timer assigned, continue
                call AIL_release_timer_handle C,dx

__shut_down:    call AIL_shutdown_driver C,cur_drvr,[SignOff]

__next_slot:    inc cur_drvr
                cmp cur_drvr,16
                jne __for_slot

                call AIL_release_all_timers

                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_register_timer PROC Callback:FAR PTR        
                USES ds,si,di

                pushf
                cli

                mov cx,ds               ;save application module's DS segment

                mov bx,0                ;look for a free timer handle....
__find_free:    cmp timer_status[bx],0
                je __found              ;found one
                add bx,2
                cmp bx,32
                jb __find_free
                mov ax,-1
                jmp __return            ;no free timers, return -1

__found:        mov ax,bx               ;yes, set up to return handle
                shr ax,1
                mov timer_status[bx],1  ;turn the new timer "off" (stopped)
                mov callback_ds[bx],cx  ;register timer proc's DS segment
                shl bx,1
                lds si,[Callback]       ;register the timer proc
                mov WORD PTR timer_callback[bx],si
                mov WORD PTR timer_callback+2[bx],ds
                inc active_timers
                cmp active_timers,1     ;is this the first timer registered?
                jne __return            ;no, just return

                push ax                 ;yes, set up our own interrupt handler

                call init_DDA_arrays    ;init timer countdown values
                call hook_timer_process ;seize interrupt and register BIOS handler

                call AIL_set_timer_period C,BIOS_H,54925
                call AIL_start_timer C,BIOS_H

                pop ax

                mov bx,ax
                shl bx,1
                mov timer_status[bx],1  ;(cleared by init_DDA_arrays)

__return:                               
                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_release_timer_handle PROC Timer
                USES ds,si,di

                pushf
                cli

                mov bx,[Timer]
                shl bx,1
                cmp timer_status[bx],0  ;is the specified timer active?
                je __return             ;no, exit

                mov timer_status[bx],0  ;release the timer's handle
                
                dec active_timers       ;any active timers left?
                jnz __return            ;if not, put the default handler back

                call set_PIT_divisor C,0

                call unhook_timer_process             
__return:       
                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_release_all_timers PROC
                USES ds,si,di

                pushf
                cli

                mov si,15               ;free all external timer handles
__release_it:   call AIL_release_timer_handle C,si
                dec si
                jge __release_it

                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_start_timer PROC Timer
                USES ds,si,di

                pushf
                cli

                mov bx,[Timer]
                shl bx,1
                cmp timer_status[bx],1  ;is the specified timer stopped?
                jne __return
                mov timer_status[bx],2  ;yes, start it
__return:       
                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_start_all_timers PROC
                USES ds,si,di

                pushf 
                cli

                mov si,15               ;start all stopped timers
__start_it:     call AIL_start_timer C,si
                dec si
                jge __start_it

                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_stop_timer  PROC Timer
                USES ds,si,di

                pushf
                cli

                mov bx,[Timer]
                shl bx,1
                cmp timer_status[bx],2  ;is the specified timer running?
                jne __return
                mov timer_status[bx],1  ;yes, stop it
__return:       
                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_stop_all_timers PROC
                USES ds,si,di

                pushf
                cli

                mov si,15               ;stop all running timers
__stop_it:      call AIL_stop_timer C,si
                dec si
                jge __stop_it

                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_set_timer_period PROC Timer,uS:DWORD
                USES ds,si,di           ;accepts timer period in microseconds

                pushf
                cli

                mov bx,[Timer]
                shl bx,1
                mov ax,timer_status[bx] ;save timer's status
                push ax
                mov timer_status[bx],1  ;stop timer while calculating...

                shl bx,1
                mov ax,WORD PTR [uS]
                mov dx,WORD PTR [uS]+2
                mov WORD PTR timer_value[bx],ax
                mov WORD PTR timer_value[bx]+2,dx

                mov WORD PTR timer_elapsed[bx],0
                mov WORD PTR timer_elapsed[bx]+2,0

                call program_timers     ;reset base interrupt rate if needed

                pop ax
                mov bx,[Timer]
                shl bx,1
                mov timer_status[bx],ax ;restore timer's former status

                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_set_timer_frequency PROC Timer,Hz:DWORD
                USES ds,si,di           ;accepts timer frequency in Hertz

                pushf
                cli

                call ul_divide C,4240h,000fh,[Hz]
                call AIL_set_timer_period C,[Timer],ax,dx

                POP_F
                ret
                ENDP                                               

;*****************************************************************************
AIL_set_timer_divisor PROC Timer,PIT
                USES ds,si,di           ;accepts PIT register values directly

                pushf
                cli

                cmp [PIT],0             ;special case: 0 wraps to 65536
                jne __nonzero
                mov ax,54925      
                mov dx,0
                jmp __set_AXDX

__nonzero:      mov ax,10000            ;convert to microseconds
                mov bx,11932
                mul [PIT]
                div bx                  ;(accurate to ±.01%)
                mov dx,0                ;(fixes bug in v1.00 release)

__set_AXDX:     call AIL_set_timer_period C,[Timer],ax,dx

                POP_F
                ret
                ENDP

;*****************************************************************************
AIL_interrupt_divisor PROC Timer        ;Get value last used by the API to 
                USES ds,si,di           ;program the PIT chip

                pushf
                cli

                mov ax,PIT_divisor

                POP_F
                ret
                ENDP

;*****************************************************************************
;*                                                                           *
;* Installation services                                                     *
;*                                                                           *
;*****************************************************************************

AIL_register_driver PROC Addr:FAR PTR
                USES ds,si,di

                pushf
                cli
                
                mov cur_drvr,0
__find_handle:  mov si,cur_drvr
                shl si,1
                shl si,1
                mov ax,WORD PTR index_base[si]
                or ax,WORD PTR index_base[si+2]
                je __found
                inc cur_drvr
                cmp cur_drvr,16
                jne __find_handle
                mov ax,-1               ;return -1 if no free handles
                jmp __return

__found:        les di,[Addr]           ;get driver base address

                mov ax,-1               ;check for copyright string to
                cmp es:[di+2],'oC'      ;avoid calling non-AIL drivers
                jne __return
                cmp es:[di+4],'yp'
                jne __return

                add di,es:[di]          ;skip copyright message text
                mov WORD PTR index_base[si],di
                mov WORD PTR index_base[si+2],es

                call AIL_describe_driver C,cur_drvr

                mov es,dx               ;check API version compatibility
                mov di,ax
                or dx,ax
                mov ax,-1
                je __return             ;return -1 if description call failed

                mov dx,es:[di].min_API_version
                cmp dx,current_rev
                ja __return             ;return -1 if API out of date

__valid_handle: mov ax,cur_drvr         ;else return AX=new driver handle

__return:       POP_F
                ret

                ENDP

;*****************************************************************************
AIL_release_driver_handle PROC H
                USES ds,si,di

                pushf
                cli

                mov bx,[H]
                cmp bx,16
                jae __exit              ;exit cleanly if invalid handle passed
                shl bx,1
                shl bx,1
                mov WORD PTR index_base[bx],0
                mov WORD PTR index_base[bx+2],0

__exit:         POP_F
                ret
                ENDP

;*****************************************************************************
AIL_describe_driver PROC HDrvr

                push SEG AIL_interrupt_divisor
                push OFFSET AIL_interrupt_divisor
                push [HDrvr]
                mov ax,AIL_DESC_DRVR
                call call_driver
                add sp,6
                ret
                ENDP

;*****************************************************************************
AIL_detect_device PROC

                mov ax,AIL_DET_DEV
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_init_driver PROC HDrvr,IO,IRQ,DMA,DRQ
                USES ds,si,di

                pushf
                cli

                cmp [HDrvr],16
                jae __return            ;exit cleanly if invalid handle passed

                mov timer_handle,-1

                call AIL_describe_driver C,[HDrvr]

                mov es,dx
                mov di,ax
                mov si,es:[di].svc_rate ;get desired service rate
                cmp si,-1
                je __do_init            ;(no timer service requested)

                mov ax,AIL_SERVE_DRVR 
                mov bx,[HDrvr]
                call find_proc

                mov es,dx
                mov bx,ax               ;ES:BX = serve_driver() address

                call AIL_register_timer C,bx,es
                mov bx,[HDrvr]
                shl bx,1           
                mov assigned_timer[bx],ax
                mov timer_handle,ax

                call AIL_set_timer_frequency C,ax,si,0

__do_init:      push DRQ
                push DMA
                push IRQ
                push IO
                push HDrvr
                mov ax,AIL_INIT_DRVR
                call call_driver
                add sp,10

                mov bx,[HDrvr]
                shl bx,1
                mov driver_active[bx],1

                cmp timer_handle,-1
                je __return
                call AIL_start_timer C,timer_handle

__return:       POP_F
                ret

                ENDP

;*****************************************************************************
AIL_shutdown_driver PROC

                mov bx,sp
                mov bx,ss:[bx+4]        ;get handle
                cmp bx,16
                jae __exit              ;exit cleanly if invalid handle passed

                shl bx,1
                mov dx,0
                xchg driver_active[bx],dx
                cmp dx,0
                je __exit               ;driver never initialized, exit
                mov dx,assigned_timer[bx]
                cmp dx,-1
                je __shut_down          ;no timer assigned, continue
                call AIL_release_timer_handle C,dx

__shut_down:    mov ax,AIL_SHUTDOWN_DRVR
                jmp call_driver

__exit:         ret
                ENDP

;*****************************************************************************
;*                                                                           *
;* Performance services                                                      *
;*                                                                           *
;*****************************************************************************

AIL_index_VOC_block PROC

                mov ax,AIL_INDEX_VOC_BLK  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_register_sound_buffer PROC

                mov ax,AIL_REG_SND_BUFF  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_sound_buffer_status PROC

                mov ax,AIL_SND_BUFF_STAT 
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_play_VOC_file  PROC

                mov ax,AIL_P_VOC_FILE 
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_VOC_playback_status PROC

                mov ax,AIL_VOC_PB_STAT   
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_start_digital_playback PROC

                mov ax,AIL_START_D_PB    
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_stop_digital_playback PROC

                mov ax,AIL_STOP_D_PB     
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_pause_digital_playback PROC

                mov ax,AIL_PAUSE_D_PB    
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_resume_digital_playback PROC

                mov ax,AIL_RESUME_D_PB   
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_set_digital_playback_volume PROC

                mov ax,AIL_SET_D_PB_VOL  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_digital_playback_volume PROC

                mov ax,AIL_D_PB_VOL      
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_set_digital_playback_panpot PROC

                mov ax,AIL_SET_D_PB_PAN  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_digital_playback_panpot PROC

                mov ax,AIL_D_PB_PAN      
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_state_table_size PROC

                mov ax,AIL_STATE_TAB_SIZE
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_register_sequence PROC

                mov ax,AIL_REG_SEQ       
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_release_sequence_handle PROC

                mov ax,AIL_REL_SEQ_HND   
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_default_timbre_cache_size PROC

                mov ax,AIL_T_CACHE_SIZE  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_define_timbre_cache PROC

                mov ax,AIL_DEFINE_T_CACHE
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_timbre_request PROC

                mov ax,AIL_T_REQ         
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_install_timbre PROC

                mov ax,AIL_INSTALL_T     
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_protect_timbre PROC

                mov ax,AIL_PROTECT_T     
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_unprotect_timbre PROC

                mov ax,AIL_UNPROTECT_T   
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_timbre_status  PROC

                mov ax,AIL_T_STATUS
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_start_sequence PROC

                mov ax,AIL_START_SEQ     
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_stop_sequence  PROC

                mov ax,AIL_STOP_SEQ      
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_resume_sequence PROC

                mov ax,AIL_RESUME_SEQ    
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_sequence_status PROC

                mov ax,AIL_SEQ_STAT      
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_relative_volume PROC

                mov ax,AIL_REL_VOL       
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_relative_tempo PROC

                mov ax,AIL_REL_TEMPO
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_set_relative_volume PROC

                mov ax,AIL_SET_REL_VOL       
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_set_relative_tempo PROC

                mov ax,AIL_SET_REL_TEMPO  
                jmp call_driver
                        
                ENDP

;*****************************************************************************
AIL_beat_count     PROC

                mov ax,AIL_BEAT_CNT      
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_measure_count  PROC

                mov ax,AIL_BAR_CNT       
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_branch_index   PROC

                mov ax,AIL_BRA_INDEX  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_controller_value PROC

                mov ax,AIL_CON_VAL       
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_set_controller_value PROC

                mov ax,AIL_SET_CON_VAL   
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_channel_notes  PROC

                mov ax,AIL_CHAN_NOTES    
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_send_channel_voice_message PROC

                mov ax,AIL_SEND_CV_MSG   
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_send_sysex_message PROC

                mov ax,AIL_SEND_SYSEX_MSG
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_write_display  PROC

                mov ax,AIL_WRITE_DISP    
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_install_callback PROC

                mov ax,AIL_INSTALL_CB    
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_cancel_callback PROC

                mov ax,AIL_CANCEL_CB     
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_lock_channel   PROC

                mov ax,AIL_LOCK_CHAN     
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_map_sequence_channel PROC

                mov ax,AIL_MAP_SEQ_CHAN  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_release_channel PROC

                mov ax,AIL_RELEASE_CHAN  
                jmp call_driver

                ENDP

;*****************************************************************************
AIL_true_sequence_channel PROC

                mov ax,AIL_TRUE_SEQ_CHAN  
                jmp call_driver

                ENDP

;*****************************************************************************
                END

