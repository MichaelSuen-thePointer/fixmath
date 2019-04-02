.code

uint128div PROC
	; uint64_t return_addr(rcx)
	; uint64_t lo(rdx)
	; uint64_t hi(r8)
	; uint64_t dividend(r9)
	mov rax, r8
	mov r8, rdx
	xor rdx, rdx
	div r9 ; rax->quotient_hi
	mov QWORD PTR[rcx+8], rax
	mov rax, r8 ; rcx -> quotient_hi, rax -> lo
	div r9 ; rax->quotient_lo; rdx -> remainder
	mov QWORD PTR[rcx], rax
	mov QWORD PTR[rcx+16], rdx
	mov rax, rcx
	ret
uint128div ENDP

uint128mul PROC
	; uint64_t return_addr(rcx)
	; uint64_t a(rdx)
	; uint64_t b(r8)
	mov rax, rdx
	mul r8
	mov QWORD PTR[rcx], rax
	mov QWORD PTR[rcx+8], rdx
uint128mul ENDP

int128mul PROC
	; uint64_t return_addr(rcx)
	; int64_t a(rdx)
	; int64_t b(r8)
	mov rax, rdx
	imul r8
	mov QWORD PTR[rcx], rax
	mov QWORD PTR[rcx+8], rdx
int128mul ENDP

end