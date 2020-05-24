.global _start
_start:
la sp,__stack_end
call test
call exit