// Напишите подпрограмму alloc_mem, которая аллоцирует память на куче, 
// принимает один параметр size (32-битное беззнаковое число) — минимальный размер (в байтах) блока памяти, который нужно аллоцировать;
// возвращает адрес блока памяти не менее чем требуемого размера либо 0.
// В тестирующей программе будет определена глобальная переменная freelist, в которой хранится список свободных блоков памяти:

// В каждом блоке первое машинное слово — размер блока в байтах минус 4, второе — адрес следующего блока в списке.
// Если в списке есть блоки, у которых записанный размер не меньше size, alloc_mem должна удалить первый такой блок из списка 
// и вернуть вызывающей подпрограмме адрес тела этого блока (адрес блока, увеличенный на 4).

// Если в списке freelist подходящих блоков нет, то alloc_mem использует системный вызов brk, 
// чтобы запросить у системы 4 + actual_size байт дополнительной памяти. actual_size вычисляется как max(4, align4(size)), 
// где align4 – операция округления вверх до числа, кратного 4.


#include <sys/syscall.h>
	.global	alloc_mem
alloc_mem:
	push	%ebp
	mov	%esp, %ebp

	// размер нужного блока памяти
	movl	8(%esp), %eax

	// адрес поля next предыдущей ноды
	mov	$freelist, %edx
	// адрес current ноды
	mov	freelist, %ecx

	push	%ebx
search_freelist:
	cmp	$0, %ecx
	je	notfound
	cmp	%eax, (%ecx)
	jb	next
	add	$4, %ecx
	mov	%ecx, %eax
	mov	(%ecx), %ecx
	mov	%ecx, (%edx)
	jmp	ending

next:
	add	$4, %ecx
	mov	%ecx, %edx
	mov	(%ecx), %ecx
	jmp	search_freelist

notfound:
	mov	$3, %ecx
	// если у нас ноль байт попросили выделить,
	// то мы все равно выделяем как минимум 8
	cmp	$0, %eax
	jne	align
	mov	$4, %eax
	jmp	continue

align:
	test	%ecx, %eax
	jz	continue
	add	$1, %eax
	jc	ending
	jmp	align

continue:
	add	$4, %eax
	jnc	heap
	mov	$0, %eax
	jmp	ending
heap:
	push	%eax
	mov	$SYS_brk, %eax
	mov	$0, %ebx
	int	$0x80

	mov	%eax, %ebx
	pop	%eax
	add	%eax, %ebx
	jnc	onemorecontinue
	mov	$0, %eax
	jmp	ending

onemorecontinue:
	push	%eax
	push	%ebx
	mov	$SYS_brk, %eax
	int	$0x80

	pop	%ebx
	pop	%ecx
	cmp	%eax, %ebx
	je	good
	mov	$0, %eax
	jmp	ending

good:
	sub	%ecx, %eax
	sub	$4, %ecx
	mov	%ecx, (%eax)
	add	$4, %eax

ending:
	pop	%ebx
	mov	%ebp, %esp
	pop	%ebp
	ret
