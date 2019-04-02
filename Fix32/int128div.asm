.code

uint128divrem PROC
	; uint64_t return_addr(rcx)
	; uint64_t lo(rdx)
	; uint64_t hi(r8)
	; uint64_t dividend(r9)
	mov rax, r8
	mov r8, rdx
	xor rdx, rdx
	div r9 ; rax->quotient_hi
	mov QWORD PTR[rcx + 8], rax
	mov rax, r8 ; rcx -> quotient_hi, rax -> lo
	div r9 ; rax->quotient_lo; rdx -> remainder
	mov QWORD PTR[rcx], rax
	mov QWORD PTR[rcx+16], rdx
	mov rax, rcx
	ret
uint128divrem ENDP

int128divrem PROC
	; int64_t return_addr(rcx)
	; int64_t lo(rdx)
	; int64_t hi(r8)
	; int64_t dividend(r9)
	mov rax, r8
	mov r8, rdx
	cqo
	idiv r9 ; rax->quotient_hi
	mov QWORD PTR[rcx + 8], rax
	mov rax, r8 ; rcx -> quotient_hi, rax -> lo
	cqo
	idiv r9 ; rax->quotient_lo; rdx -> remainder
	mov QWORD PTR[rcx], rax
	mov QWORD PTR[rcx+16], rdx
	mov rax, rcx
	ret
int128divrem ENDP

end