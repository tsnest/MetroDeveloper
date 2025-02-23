; Предполагается, что мы используем Windows x64 и подключаем printf из C Runtime
EXTERN printf : PROC
EXTERN load_navmap_Orig : QWORD

.DATA
    format db "%s", 0          ; Строка формата для printf

.CODE

load_navmap_hack PROC
    ; RCX содержит первый аргумент (аналог [esp + 4] в x86)

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;

    push rcx

    mov rdx, rcx               ; Перемещаем аргумент в RDX (2-й аргумент для printf)
    lea rcx, format            ; Загружаем адрес строки формата в RCX (1-й аргумент)

    sub rsp, 28h               ; Выравнивание стека (16-байтовое выравнивание + "тень" для вызова)
    call printf                ; Вызов printf
    add rsp, 28h               ; Восстановление стека после вызова

    pop rcx

    mov rax, 1h
    mov [rsp+78h], rax
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;

    jmp load_navmap_Orig
load_navmap_hack ENDP

END