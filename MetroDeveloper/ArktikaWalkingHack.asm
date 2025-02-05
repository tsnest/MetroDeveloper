.CODE

walking_fix_attempt PROC
    mov rax, [rsi + 600h]             ; rax = player->_physics
    test rax, rax                     ; if (player->_physics)
    jz notinitialized                 ; if null, jump to notinitialized

    xor r8d, r8d                      ; r8d = 0
    test dword ptr [rsi + 2B90h], 40h ; if (!player->estate.crouch)
    xor edx, edx                      ; edx = 0
    setne dl                          ; if crouch is not set, edx = 1

    mov rcx, [rax + 8]                ; rcx = player->_physics->_mov_control
    mov rax, [rcx]                    ; rax = _mov_control vtable
    call qword ptr [rax + 1E0h]       ; call mov_control_nx::activate_box

notinitialized:
    add rsp, 100h                     ; restore stack pointer
    pop rsi                           ; restore rsi
    pop rbx                           ; restore rbx
    pop rbp                           ; restore rbp
    ret
walking_fix_attempt ENDP

END