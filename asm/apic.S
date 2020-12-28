.section .rodata
msg:
	.asciz "Hello there."
fmt:
	.asciz "Message: %s\n"

.section .text
.global foobar
.type foobar, @function
foobar:
	push %rbp
	mov $fmt, %rdi
	mov $msg, %rsi
	mov $0, %rax
	call printf
	pop %rbp
	ret
