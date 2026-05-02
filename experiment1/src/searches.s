	.intel_syntax noprefix
	.section .note.GNU-stack,"",@progbits
	.text

	.globl lower_bound1
	.type lower_bound1, @function
# lower_bound1(const int64_t *numbers, size_t n, int64_t query)
# r8  = numbers (base pointer)
# rcx = right   (exclusive upper bound index)
# rdx = left    (inclusive lower bound index)
# rsi = query
# rax = scratch (right-left, then mid)
# r11 = scratch (mid + 1)
lower_bound1:
	endbr64
	mov	r8, rdi         # r8  = numbers
	mov	rcx, rsi        # rcx = right = n
	mov	rsi, rdx        # rsi = query
	xor	rdx, rdx        # rdx = left = 0
.Lcond1:                        # while (left < right)
	mov	rax, rcx
	sub	rax, rdx        # rax = right - left
	cmp	rax, 0
	jne	.Lloop1
	mov	rax, rdx        # return left (lower bound index)
	ret
.Lloop1:
	shr	rax, 1          # rax = (right - left) / 2
	add	rax, rdx        # rax = mid = left + (right - left) / 2
	lea	r11, [rax+1]    # r11 = mid + 1
	cmp	QWORD PTR [r8+rax*8], rsi  # numbers[mid] vs query
	cmovge	rcx, rax        # numbers[mid] >= query → right = mid
	cmovl	rdx, r11        # numbers[mid] <  query → left  = mid + 1
	jmp	.Lcond1
	.size lower_bound1, .-lower_bound1

	.globl lower_bound2
	.type lower_bound2, @function
# lower_bound2(const int64_t *numbers, size_t n, int64_t query)
# r8  = numbers (base pointer)
# rcx = right   (exclusive upper bound index)
# rdx = left    (inclusive lower bound index)
# rsi = query
# rax = scratch (right-left, then mid)
# r11 = scratch (mid + 1)
lower_bound2:
	endbr64
	mov	r8, rdi         # r8  = numbers
	mov	rcx, rsi        # rcx = right = n
	mov	rsi, rdx        # rsi = query
	xor	rdx, rdx        # rdx = left = 0
.Lcond2:                        # while (left < right)
	mov	rax, rcx
	sub	rax, rdx        # rax = right - left
	cmp	rax, 0
	jne	.Lloop2
	mov	rax, rdx        # return left (lower bound index)
	ret
.Lloop2:
	shr	rax, 1          # rax = (right - left) / 2
	add	rax, rdx        # rax = mid = left + (right - left) / 2
	lea	r11, [rax+1]    # r11 = mid + 1
	cmp	QWORD PTR [r8+rax*8], rsi  # numbers[mid] vs query
	jl	.Lleft2         # numbers[mid] < query → left = mid + 1
	mov	rcx, rax        # numbers[mid] >= query → right = mid
	jmp	.Lcond2
.Lleft2:
	mov	rdx, r11        # numbers[mid] < query → left = mid + 1
	jmp	.Lcond2
	.size lower_bound2, .-lower_bound2
