; Предполагается, что мы используем Windows x64 и подключаем printf из C Runtime
EXTERN printf : PROC
EXTERN initHandle_Orig : QWORD
EXTERN rlog : QWORD

.DATA
    format db "!!Can't find texture: %s", 0          ; Строка формата для printf

.CODE

initHandle_hack PROC
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ;mov [rsp+08], rbx
    push rcx
    push rdi
    push rdx
    sub rsp, 28h
    ;mov rdi, rdx
    ;mov rdx, rcx

    test rcx, rcx
    ;jne rlogCallBypass
    je rlogCallBypass

    mov rdx, [rdx]
    lea rcx, format
    add rdx, 14h
    call qword ptr [rlog]

rlogCallBypass:
    ;mov rdx, rdi
    ;mov rcx, rbx
    ;mov rbx, [rsp+30h]

    add rsp, 28h
    pop rdi
    pop rcx
    pop rdx
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;

    jmp initHandle_Orig
initHandle_hack ENDP

END