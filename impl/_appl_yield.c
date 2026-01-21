
void _appl_yield(void)
{
	__asm__ volatile (
		"move.l	#201, %%d0\n\t"
		"trap	#2\n\t"
		:
		:
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc"
	);
}
