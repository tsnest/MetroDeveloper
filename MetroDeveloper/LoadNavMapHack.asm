; ��������������, ��� �� ���������� Windows x64 � ���������� printf �� C Runtime
EXTERN printf : PROC
EXTERN load_navmap_Orig : QWORD

.DATA
    format db "%s", 0          ; ������ ������� ��� printf

.CODE

load_navmap_hack PROC
    ; RCX �������� ������ �������� (������ [esp + 4] � x86)

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;

    push rcx

    mov rdx, rcx               ; ���������� �������� � RDX (2-� �������� ��� printf)
    lea rcx, format            ; ��������� ����� ������ ������� � RCX (1-� ��������)

    sub rsp, 28h               ; ������������ ����� (16-�������� ������������ + "����" ��� ������)
    call printf                ; ����� printf
    add rsp, 28h               ; �������������� ����� ����� ������

    pop rcx

    mov rax, 1h
    mov [rsp+78h], rax
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;

    jmp load_navmap_Orig
load_navmap_hack ENDP

END