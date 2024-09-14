# LOOP(loop, rax=0, <, rdi=1000, 1)
xor rax, rax
mov rdi, #1000
.align 16
loop:
# LOOPEND(loop)
add rax, 1
cmp rax, rdi
jl loop

