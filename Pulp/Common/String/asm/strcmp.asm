
ifndef X_64

.686p
.XMM
.model flat, C

.code

; 32bit version
x_strcmp PROC


align 16
_strcmpSSE42:
		mov     eax, [esp+4]           ; string 1
		mov     edx, [esp+8]           ; string 2
		push    ebx
		mov     ebx, -16               ; offset counter

compareloop:
        add     ebx, 16                ; increment offset
        movdqu  xmm1, [eax+ebx]        ; read 16 bytes of string 1
        pcmpistri xmm1, [edx+ebx], 00011000B ; unsigned bytes, equal each, invert. returns index in ecx
        jnbe    compareloop            ; jump if not carry flag and not zero flag
        
        jnc     equal
notequal:                              ; strings are not equal
        ; strings are not equal
        add     ecx, ebx               ; offset to first differing byte
        movzx   eax, byte ptr [eax+ecx]    ; compare bytes
        movzx   edx, byte ptr [edx+ecx]
		sub     eax, edx
		pop     ebx
		ret
        
equal:
        xor     eax, eax               ; strings are equal
		pop     ebx
		ret

;_strcmpSSE42: endp


align 16
_strcmpGeneric:  ; generic version
; This is a very simple solution. There is not much gained by using SSE2 or anything complicated
		mov     ecx, [esp+4]          ; string 1
		mov     edx, [esp+8]          ; string 2
		
_compareloop:
        mov     al, [ecx]
        cmp     al, [edx]
        jne     _notequal
        test    al, al
        jz      _equal
        inc     ecx
        inc     edx
        jmp     _compareloop        
        
_equal: xor     eax, eax               ; strings are equal
		ret

_notequal:                             ; strings are not equal
        movzx   eax, byte ptr [ecx]        ; compare first differing byte
        movzx   edx, byte ptr [edx]
		sub     eax, edx
		ret
		
;_strcmpGeneric end



x_strcmp ENDP

else ; X_64 ------------------------------------------------------------------------------

FUNCPROTO       TYPEDEF PROTO 
FUNCPTR         TYPEDEF PTR FUNCPROTO

.data?

InstructionSet  FUNCPTR     ?    ; Instruction set for CPU dispatcher
strcmpDispatch	DQ			?

.data



.code

; 64bit version
x_strcmp PROC

        jmp     near ptr [strcmpDispatch] ; Go to appropriate version, depending on instruction set

align 16
strcmpSSE42:
        push    rdi
        mov     rdi, rcx
        mov     rax, -16               ; offset counter
compareloop:
        add     rax, 16                ; increment offset
        movdqu  xmm1, [rdi+rax]        ; read 16 bytes of string 1
        pcmpistri xmm1, [rdx+rax], 00011000B ; unsigned bytes, equal each, invert. returns index in ecx
        jnbe    compareloop            ; jump if not carry flag and not zero flag
        
        jnc     equal
notequal:
        ; strings are not equal
        add     rcx, rax               ; offset to first differing byte
        movzx   eax, byte ptr[rdi+rcx]    ; compare first differing byte
        movzx   edx, byte ptr[rdx+rcx]
		sub     rax, rdx
        pop     rdi
		ret

equal:
        xor     eax, eax               ; strings are equal
        pop     rdi
		ret

;strcmpSSE42: endp


align 16
strcmpGeneric:  ; generic version

_compareloop:
        mov     al, [rcx]
        cmp     al, [rdx]
        jne     _notequal
        test    al, al
        jz      _equal
        inc     rcx
        inc     rdx
        jmp     _compareloop        
        
_equal: xor     eax, eax               ; strings are equal
		ret

_notequal:                             ; strings are not equal
        movzx   eax, byte ptr [rcx]        ; compare first differing byte
        movzx   edx, byte ptr [rdx]
		sub     eax, edx
		ret
		
;strcmpGeneric end


; CPU dispatching for strcmp. This is executed only once
strcmpCPUDispatch:
        ; get supported instruction set
        push    rcx
        push    rdx
        call    InstructionSet
        pop     rdx
        pop     rcx
        ; Point to generic version of strcmp
        lea     r9, [strcmpGeneric]
        cmp     eax, 10                ; check SSE4.2
        jb      Q100
        ; SSE4.2 supported
        ; Point to SSE4.2 version of strcmp
        lea     r9, [strcmpSSE42]
Q100:   mov     [strcmpDispatch], r9
        ; Continue in appropriate version of strcmp
        jmp     r9


x_strcmp ENDP



endif

END


