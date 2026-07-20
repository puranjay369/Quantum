.intel_syntax noprefix
.global _start
.bss
printBuf: .skip 32
.text
_start:
    push rbp
    mov rbp, rsp
    sub rsp, 256
    mov rax, 7
    mov qword ptr [rbp-8], rax
    mov rax, qword ptr [rbp-8]
    lea r8, [printBuf + 31]
    mov byte ptr [r8], 10
    dec r8
    xor r9, r9
    cmp rax, 0
    jge .Lnonneg0
    neg rax
    mov r9, 1
.Lnonneg0:
    cmp rax, 0
    jne .Lloop0
    mov byte ptr [r8], 48
    dec r8
    jmp .Lsign0
.Lloop0:
    cmp rax, 0
    je .Lsign0
    xor rdx, rdx
    mov rbx, 10
    div rbx
    add dl, 48
    mov [r8], dl
    dec r8
    jmp .Lloop0
.Lsign0:
    cmp r9, 0
    je .Ldone0
    mov byte ptr [r8], 45
    dec r8
.Ldone0:
    inc r8
    lea rax, [printBuf + 32]
    sub rax, r8
    mov rdx, rax
    mov rsi, r8
    mov rdi, 1
    mov rax, 1
    syscall
    mov rax, 14
    mov qword ptr [rbp-16], rax
    mov rax, qword ptr [rbp-16]
    lea r8, [printBuf + 31]
    mov byte ptr [r8], 10
    dec r8
    xor r9, r9
    cmp rax, 0
    jge .Lnonneg1
    neg rax
    mov r9, 1
.Lnonneg1:
    cmp rax, 0
    jne .Lloop1
    mov byte ptr [r8], 48
    dec r8
    jmp .Lsign1
.Lloop1:
    cmp rax, 0
    je .Lsign1
    xor rdx, rdx
    mov rbx, 10
    div rbx
    add dl, 48
    mov [r8], dl
    dec r8
    jmp .Lloop1
.Lsign1:
    cmp r9, 0
    je .Ldone1
    mov byte ptr [r8], 45
    dec r8
.Ldone1:
    inc r8
    lea rax, [printBuf + 32]
    sub rax, r8
    mov rdx, rax
    mov rsi, r8
    mov rdi, 1
    mov rax, 1
    syscall
    mov rax, qword ptr [rbp-16]
    push rax
    mov rax, qword ptr [rbp-8]
    mov rbx, rax
    pop rax
    cmp rbx, 0
    jne .Ldivsafe2
    mov rax, 1
    mov rdi, 1
    lea rsi, divzero_msg
    mov rdx, 17
    syscall
    mov rax, 60
    mov rdi, 1
    syscall
.Ldivsafe2:
    cqo
    idiv rbx
    mov qword ptr [rbp-24], rax
    mov rax, qword ptr [rbp-24]
    lea r8, [printBuf + 31]
    mov byte ptr [r8], 10
    dec r8
    xor r9, r9
    cmp rax, 0
    jge .Lnonneg3
    neg rax
    mov r9, 1
.Lnonneg3:
    cmp rax, 0
    jne .Lloop3
    mov byte ptr [r8], 48
    dec r8
    jmp .Lsign3
.Lloop3:
    cmp rax, 0
    je .Lsign3
    xor rdx, rdx
    mov rbx, 10
    div rbx
    add dl, 48
    mov [r8], dl
    dec r8
    jmp .Lloop3
.Lsign3:
    cmp r9, 0
    je .Ldone3
    mov byte ptr [r8], 45
    dec r8
.Ldone3:
    inc r8
    lea rax, [printBuf + 32]
    sub rax, r8
    mov rdx, rax
    mov rsi, r8
    mov rdi, 1
    mov rax, 1
    syscall
    mov rax, 60
    mov rdi, 0
    syscall
.data
divzero_msg: .ascii "Division by zero\n"
