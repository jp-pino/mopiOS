.section .text, "ax"
.balign 4
.thumb
.syntax unified

.thumb_func
.global ResetISR_asm
ResetISR_asm:
	ldr     r0, =.bss
	ldr     r1, =.ebss
	mov     r2, #0
zero_loop:
  cmp     r0, r1
  it      lt
  strlt   r2, [r0], #4
  blt     zero_loop
