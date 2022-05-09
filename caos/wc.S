// На стандартный вход подаётся текст в кодировке ASCII.

// Напечатайте на стандартный выход количество строк, слов и символов (байт) во входном тексте (как это сделала бы утилита wc). 
// Словами считаются последовательности непробельных символов, разделённые любым количеством пробельных символов. 
// Пробельными символами считаются пробел (код 32) и перевод строки (код 10).



#include <sys/syscall.h>

	.text
	.global main
main:
	// будем считать количество непробельных символов подряд
	xor	%edi,  %edi
loop:
	// считываем по одному байтику
	mov	$SYS_read, %eax
	mov	$0, %ebx
	mov	$buffer, %ecx
	mov	$1, %edx
	int	$0x80

	// если ничего не считали, то конец
	cmp	$0, %eax
	jle	ending

	incl	symbols

	cmp	$10, buffer
	je	newline
	cmp	$32, buffer
	je	space
	incl	%edi
	jmp	loop

newline:
	incl	lines

space:
	cmp	$0, %edi
	jle	xor
	incl	words
xor:
	xor	%edi, %edi
	jmp	loop

ending:
	mov	lines, %eax
	call	writei32
	mov	words, %eax
	call	writei32
	mov	symbols, %eax
	call	writei32
	call	finish


	.data
lines:
	.int 	0
words:
	.int 	0
symbols:
	.int 	0
buffer:
	.skip	4
