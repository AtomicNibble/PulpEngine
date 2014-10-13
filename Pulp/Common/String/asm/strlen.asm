
ifndef X_64

.686p
.XMM
.model flat, C

.code

; 32bit version
x_strlen PROC

        mov      eax,  [esp+4]         ; get pointer to string
        mov      ecx,  eax             ; copy pointer
        pxor     xmm0, xmm0            ; set to zero
        and      ecx,  0FH             ; lower 4 bits indicate misalignment
        and      eax,  -10H            ; align pointer by 16
        movdqa   xmm1, [eax]           ; read from nearest preceding boundary
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        shr      edx,  cl              ; shift out false bits
        shl      edx,  cl              ; shift back again
        bsf      edx,  edx             ; find first 1-bit
        jnz      A200                  ; found
        
        ; Main loop, search 16 bytes at a time
A100:   add      eax,  10H             ; increment pointer by 16
        movdqa   xmm1, [eax]           ; read 16 bytes aligned
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        bsf      edx,  edx             ; find first 1-bit
        ; (moving the bsf out of the loop and using test here would be faster for long strings on old processors,
        ;  but we are assuming that most strings are short, and newer processors have higher priority)
        jz       A100                  ; loop if not found
        
A200:   ; Zero-byte found. Compute string length        
        sub      eax,  [esp+4]         ; subtract start address
        add      eax,  edx             ; add byte index
        ret


x_strlen ENDP

else ; X_64 ------------------------------------------------------------------------------

.code

; 64bit version
x_strlen PROC
        mov      rax,  rcx             ; get pointer to string from rcx
        mov      r8,   rcx             ; copy pointer
        
        ; rax = s, ecx = 32 bits of s
        pxor     xmm0, xmm0            ; set to zero
        and      ecx,  0FH             ; lower 4 bits indicate misalignment
        and      rax,  -10H            ; align pointer by 16
        movdqa   xmm1, [rax]           ; read from nearest preceding boundary
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        shr      edx,  cl              ; shift out false bits
        shl      edx,  cl              ; shift back again
        bsf      edx,  edx             ; find first 1-bit
        jnz      L2                    ; found
        
        ; Main loop, search 16 bytes at a time
L1:     add      rax,  10H             ; increment pointer by 16
        movdqa   xmm1, [rax]           ; read 16 bytes aligned
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        bsf      edx,  edx             ; find first 1-bit
        ; (moving the bsf out of the loop and using test here would be faster for long strings on old processors,
        ;  but we are assuming that most strings are short, and newer processors have higher priority)
        jz       L1                    ; loop if not found
        
L2:     ; Zero-byte found. Compute string length        
        sub      rax,  r8          ; subtract start address
        add      rax,  rdx             ; add byte index
        ret

x_strlen ENDP


endif

END









