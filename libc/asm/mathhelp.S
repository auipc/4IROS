.global round_to_nearest
round_to_nearest:
	// FIXME Does this clobber registers? 2 lazy to read output asm
	roundsd $0x0, %xmm0, %xmm1
	movsd %xmm0, %xmm1
	ret
