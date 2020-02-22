.code

;SIGNATURE:
;
;struct retval {
;	uint64_t lo, hi, rem;
;};
;extern "C" retval uint128div(uint64_t lo, uint64_t hi, uint64_t div);
;
uint128div PROC
	; retval* return_addr(rcx)
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

;SIGNATURE:
;
;struct retval {
;	uint64_t lo, hi;
;};
;extern "C" retval uint128mul(uint64_t a, uint64_t b);
;
uint128mul PROC
	; retval* return_addr(rcx)
	; uint64_t a(rdx)
	; uint64_t b(r8)
	mov rax, rdx
	mul r8
	mov QWORD PTR[rcx], rax
	mov QWORD PTR[rcx+8], rdx
	mov rax, rcx
	ret
uint128mul ENDP

;SIGNATURE:
;
;struct retval {
;	uint64_t lo
;	int64_t hi;
;};
;extern "C" retval int128mul(int64_t a, int64_t b);
;
int128mul PROC
	; retval* return_addr(rcx)
	; int64_t a(rdx)
	; int64_t b(r8)
	mov rax, rdx
	imul r8
	mov QWORD PTR[rcx], rax
	mov QWORD PTR[rcx+8], rdx
	mov rax, rcx
	ret
int128mul ENDP

end