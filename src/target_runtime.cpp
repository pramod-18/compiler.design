#include "target_runtime.h"

std::string target_runtime_data() {
    return R"MIPS(
    .align 3
__rt_million: .double 1000000.0
__rt_half: .double 0.5
__rt_ten: .double 10.0
__rt_seed: .word 1
__rt_byte: .space 4
__rt_pending: .space 256
__rt_has_pending: .space 256
)MIPS";
}


std::string target_runtime_text() {
    return R"MIPS(
# Memory helpers: byte loops also handle character arrays and structure padding.
__rt_copy:
    move $v0, $a0
__rt_copy_loop:
    beq $a2, $zero, __rt_copy_end
    nop
    lbu $t0, 0($a1)
    sb $t0, 0($a0)
    addiu $a0, $a0, 1
    addiu $a1, $a1, 1
    addiu $a2, $a2, -1
    b __rt_copy_loop
    nop
__rt_copy_end:
    jr $ra
    nop
__rt_zero:
    move $v0, $a0
__rt_zero_loop:
    beq $a1, $zero, __rt_zero_end
    nop
    sb $zero, 0($a0)
    addiu $a0, $a0, 1
    addiu $a1, $a1, -1
    b __rt_zero_loop
    nop
__rt_zero_end:
    jr $ra
    nop

# Eight-byte allocation headers retain sizes for realloc; sbrk cannot release memory.
__rt_malloc:
    lw $a0, 0($sp)
    b __rt_allocate
    nop
__rt_allocate:
    blez $a0, __rt_allocate_fail
    nop
    li $t0, 67108864
    bgt $a0, $t0, __rt_allocate_fail
    nop
    move $t3, $a0
    addiu $a0, $a0, 15
    li $t0, -8
    and $a0, $a0, $t0
    li $v0, 9
    syscall
    bltz $v0, __rt_allocate_fail
    nop
    sw $t3, 0($v0)
    addiu $v0, $v0, 8
    jr $ra
    nop
__rt_allocate_fail:
    move $v0, $zero
    jr $ra
    nop
__rt_calloc:
    lw $t0, 0($sp)
    lw $t1, 8($sp)
    blez $t0, __rt_allocate_fail
    nop
    blez $t1, __rt_allocate_fail
    nop
    multu $t0, $t1
    mfhi $t2
    bne $t2, $zero, __rt_allocate_fail
    nop
    mflo $a0
    addiu $sp, $sp, -16
    sw $ra, 12($sp)
    sw $a0, 0($sp)
    jal __rt_allocate
    nop
    beq $v0, $zero, __rt_calloc_end
    nop
    move $a0, $v0
    lw $a1, 0($sp)
    jal __rt_zero
    nop
__rt_calloc_end:
    lw $ra, 12($sp)
    addiu $sp, $sp, 16
    jr $ra
    nop
__rt_realloc:
    lw $a0, 8($sp)
    addiu $sp, $sp, -24
    sw $ra, 20($sp)
    sw $a0, 0($sp)
    jal __rt_allocate
    nop
    sw $v0, 8($sp)
    beq $v0, $zero, __rt_realloc_end
    nop
    lw $a1, 24($sp)
    beq $a1, $zero, __rt_realloc_end
    nop
    lw $a2, -8($a1)
    lw $t0, 0($sp)
    ble $a2, $t0, __rt_realloc_copy
    nop
    move $a2, $t0
__rt_realloc_copy:
    move $a0, $v0
    jal __rt_copy
    nop
__rt_realloc_end:
    lw $v0, 8($sp)
    lw $ra, 20($sp)
    addiu $sp, $sp, 24
    jr $ra
    nop
__rt_free:
    move $v0, $zero
    jr $ra
    nop

# Strings and small standard-library operations.
__rt_strlen:
    lw $t0, 0($sp)
    move $v0, $zero
__rt_strlen_loop:
    lbu $t1, 0($t0)
    beq $t1, $zero, __rt_strlen_end
    nop
    addiu $v0, $v0, 1
    addiu $t0, $t0, 1
    b __rt_strlen_loop
    nop
__rt_strlen_end:
    jr $ra
    nop
__rt_strcpy:
    lw $t0, 0($sp)
    lw $t1, 8($sp)
    move $v0, $t0
__rt_strcpy_loop:
    lbu $t2, 0($t1)
    sb $t2, 0($t0)
    addiu $t0, $t0, 1
    addiu $t1, $t1, 1
    bne $t2, $zero, __rt_strcpy_loop
    nop
    jr $ra
    nop
__rt_strcmp:
    lw $t0, 0($sp)
    lw $t1, 8($sp)
__rt_strcmp_loop:
    lbu $t2, 0($t0)
    lbu $t3, 0($t1)
    subu $v0, $t2, $t3
    bne $v0, $zero, __rt_strcmp_end
    nop
    beq $t2, $zero, __rt_strcmp_end
    nop
    addiu $t0, $t0, 1
    addiu $t1, $t1, 1
    b __rt_strcmp_loop
    nop
__rt_strcmp_end:
    jr $ra
    nop
__rt_abs:
    lw $v0, 0($sp)
    bgez $v0, __rt_abs_end
    nop
    subu $v0, $zero, $v0
__rt_abs_end:
    jr $ra
    nop
__rt_exit:
    lw $a0, 0($sp)
    li $v0, 17
    syscall
__rt_srand:
    lw $t0, 0($sp)
    sw $t0, __rt_seed
    move $v0, $zero
    jr $ra
    nop
__rt_rand:
    lw $t0, __rt_seed
    li $t1, 1103515245
    mul $t0, $t0, $t1
    addiu $t0, $t0, 12345
    sw $t0, __rt_seed
    srl $v0, $t0, 16
    andi $v0, $v0, 32767
    jr $ra
    nop

# fopen supports POSIX SPIM flags: r=0, w=O_WRONLY|O_CREAT|O_TRUNC, a=append.
__rt_fopen:
    lw $a0, 0($sp)
    lw $t0, 8($sp)
    lbu $t1, 0($t0)
    move $a1, $zero
    li $t2, 119
    beq $t1, $t2, __rt_open_write
    nop
    li $t2, 97
    beq $t1, $t2, __rt_open_append
    nop
    b __rt_open_do
    nop
__rt_open_write:
    li $a1, 577
    b __rt_open_do
    nop
__rt_open_append:
    li $a1, 1089
__rt_open_do:
    li $a2, 420
    li $v0, 13
    syscall
    addiu $v0, $v0, 1
    blez $v0, __rt_open_end
    nop
    li $t0, 256
    bge $v0, $t0, __rt_open_end
    nop
    la $t0, __rt_has_pending
    addu $t0, $t0, $v0
    sb $zero, 0($t0)
__rt_open_end:
    jr $ra
    nop
__rt_fclose:
    lw $a0, 0($sp)
    addiu $a0, $a0, -1
    bltz $a0, __rt_close_bad
    nop
    li $v0, 16
    syscall
    move $v0, $zero
    jr $ra
    nop
__rt_close_bad:
    li $v0, -1
    jr $ra
    nop

# Unified byte output: mode 0 console, 1 FILE, 2 string; $v1 is the next string pointer.
__rt_put:
    move $v1, $a2
    li $t0, 2
    beq $a1, $t0, __rt_put_string
    nop
    beq $a1, $zero, __rt_put_console
    nop
    blez $a2, __rt_put_error
    nop
    sb $a0, __rt_byte
    addiu $a0, $a2, -1
    la $a1, __rt_byte
    li $a2, 1
    li $v0, 15
    syscall
    jr $ra
    nop
__rt_put_console:
    li $v0, 11
    syscall
    li $v0, 1
    jr $ra
    nop
__rt_put_string:
    sb $a0, 0($a2)
    addiu $v1, $a2, 1
    li $v0, 1
    jr $ra
    nop
__rt_put_error:
    li $v0, -1
    jr $ra
    nop
__rt_putchar:
    lw $a0, 0($sp)
    move $a1, $zero
    move $a2, $zero
    addiu $sp, $sp, -16
    sw $ra, 12($sp)
    sw $a0, 0($sp)
    jal __rt_put
    nop
    lw $v0, 0($sp)
    andi $v0, $v0, 255
    lw $ra, 12($sp)
    addiu $sp, $sp, 16
    jr $ra
    nop
__rt_fputc:
    lw $a0, 0($sp)
    li $a1, 1
    lw $a2, 8($sp)
    addiu $sp, $sp, -16
    sw $ra, 12($sp)
    sw $a0, 0($sp)
    jal __rt_put
    nop
    bltz $v0, __rt_fputc_end
    nop
    lw $v0, 0($sp)
    andi $v0, $v0, 255
__rt_fputc_end:
    lw $ra, 12($sp)
    addiu $sp, $sp, 16
    jr $ra
    nop
__rt_puts:
    lw $a0, 0($sp)
    move $a1, $zero
    move $a2, $zero
    li $a3, 1
    b __rt_put_string_loop_entry
    nop
__rt_fputs:
    lw $a0, 0($sp)
    li $a1, 1
    lw $a2, 8($sp)
    move $a3, $zero
__rt_put_string_loop_entry:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $s0, 24($sp)
    sw $s1, 20($sp)
    move $s0, $a0
    sw $a1, 0($sp)
    sw $a2, 4($sp)
    sw $a3, 8($sp)
    move $s1, $zero
__rt_put_string_loop:
    lbu $a0, 0($s0)
    beq $a0, $zero, __rt_put_string_done
    nop
    lw $a1, 0($sp)
    lw $a2, 4($sp)
    jal __rt_put
    nop
    bltz $v0, __rt_put_string_end
    nop
    addiu $s0, $s0, 1
    addiu $s1, $s1, 1
    b __rt_put_string_loop
    nop
__rt_put_string_done:
    lw $t0, 8($sp)
    beq $t0, $zero, __rt_put_string_return
    nop
    li $a0, 10
    move $a1, $zero
    move $a2, $zero
    jal __rt_put
    nop
    addiu $s1, $s1, 1
__rt_put_string_return:
    move $v0, $s1
__rt_put_string_end:
    lw $s1, 20($sp)
    lw $s0, 24($sp)
    lw $ra, 28($sp)
    addiu $sp, $sp, 32
    jr $ra
    nop

# Byte input keeps one lookahead per console/file, shared by fscanf, fgets and fgetc.
__rt_get:
    move $v1, $a1
    li $t0, 2
    beq $a0, $t0, __rt_get_string
    nop
    move $t4, $a0
    move $t5, $a1
    beq $a0, $zero, __rt_get_pending_console
    nop
    blez $a1, __rt_get_eof
    nop
    move $t0, $a1
    b __rt_get_pending
    nop
__rt_get_pending_console:
    move $t0, $zero
__rt_get_pending:
    li $t1, 256
    bgeu $t0, $t1, __rt_get_real
    nop
    la $t1, __rt_has_pending
    addu $t1, $t1, $t0
    lbu $t2, 0($t1)
    beq $t2, $zero, __rt_get_real
    nop
    sb $zero, 0($t1)
    la $t1, __rt_pending
    addu $t1, $t1, $t0
    lbu $v0, 0($t1)
    jr $ra
    nop
__rt_get_real:
    beq $t4, $zero, __rt_get_console
    nop
    addiu $a0, $t5, -1
    la $a1, __rt_byte
    li $a2, 1
    li $v0, 14
    syscall
    blez $v0, __rt_get_eof
    nop
    lbu $v0, __rt_byte
    jr $ra
    nop
__rt_get_console:
    li $v0, 12
    syscall
    jr $ra
    nop
__rt_get_string:
    lbu $v0, 0($a1)
    beq $v0, $zero, __rt_get_eof
    nop
    addiu $v1, $a1, 1
    jr $ra
    nop
__rt_get_eof:
    li $v0, -1
    jr $ra
    nop
__rt_unget:
    bltz $a2, __rt_unget_end
    nop
    beq $a0, $zero, __rt_unget_console
    nop
    move $t0, $a1
    b __rt_unget_store
    nop
__rt_unget_console:
    move $t0, $zero
__rt_unget_store:
    li $t1, 256
    bgeu $t0, $t1, __rt_unget_end
    nop
    la $t1, __rt_pending
    addu $t1, $t1, $t0
    sb $a2, 0($t1)
    la $t1, __rt_has_pending
    addu $t1, $t1, $t0
    li $t2, 1
    sb $t2, 0($t1)
__rt_unget_end:
    jr $ra
    nop
__rt_getchar:
    move $a0, $zero
    move $a1, $zero
    b __rt_get
    nop
__rt_fgetc:
    li $a0, 1
    lw $a1, 0($sp)
    b __rt_get
    nop
__rt_fgets:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $s0, 24($sp)
    sw $s1, 20($sp)
    sw $s2, 16($sp)
    lw $s0, 32($sp)
    lw $s1, 40($sp)
    move $s2, $zero
    blez $s1, __rt_fgets_fail
    nop
    addiu $s1, $s1, -1
__rt_fgets_loop:
    beq $s2, $s1, __rt_fgets_done
    nop
    li $a0, 1
    lw $a1, 48($sp)
    jal __rt_get
    nop
    bltz $v0, __rt_fgets_eof
    nop
    addu $t0, $s0, $s2
    sb $v0, 0($t0)
    addiu $s2, $s2, 1
    li $t0, 10
    bne $v0, $t0, __rt_fgets_loop
    nop
    b __rt_fgets_done
    nop
__rt_fgets_eof:
    beq $s2, $zero, __rt_fgets_fail
    nop
__rt_fgets_done:
    addu $t0, $s0, $s2
    sb $zero, 0($t0)
    move $v0, $s0
    b __rt_fgets_return
    nop
__rt_fgets_fail:
    move $v0, $zero
__rt_fgets_return:
    lw $s2, 16($sp)
    lw $s1, 20($sp)
    lw $s0, 24($sp)
    lw $ra, 28($sp)
    addiu $sp, $sp, 32
    jr $ra
    nop

# Formatted output: %d/%i/%u/%c/%s/%f/%%. Float output uses six fractional digits.
__rt_printf:
    move $a0, $zero
    move $a1, $zero
    lw $a2, 0($sp)
    addiu $a3, $sp, 8
    b __rt_format
    nop
__rt_fprintf:
    li $a0, 1
    lw $a1, 0($sp)
    lw $a2, 8($sp)
    addiu $a3, $sp, 16
    b __rt_format
    nop
__rt_sprintf:
    li $a0, 2
    lw $a1, 0($sp)
    lw $a2, 8($sp)
    addiu $a3, $sp, 16
__rt_format:
    addiu $sp, $sp, -40
    sw $ra, 36($sp)
    sw $s0, 32($sp)
    sw $s1, 28($sp)
    sw $s2, 24($sp)
    sw $s3, 20($sp)
    sw $s4, 16($sp)
    sw $s5, 12($sp)
    sw $s6, 8($sp)
    sw $s7, 4($sp)
    move $s0, $a0
    move $s1, $a1
    move $s2, $a2
    move $s3, $a3
    move $s4, $zero
__rt_format_loop:
    lbu $a0, 0($s2)
    addiu $s2, $s2, 1
    beq $a0, $zero, __rt_format_done
    nop
    li $t0, 37
    beq $a0, $t0, __rt_format_specifier
    nop
__rt_format_char:
    jal __rt_fmt_put
    nop
    b __rt_format_loop
    nop
__rt_format_specifier:
    lbu $s5, 0($s2)
    addiu $s2, $s2, 1
    li $t0, 37
    beq $s5, $t0, __rt_format_percent
    nop
    lw $a0, 0($s3)
    addiu $s3, $s3, 8
    li $t0, 99
    beq $s5, $t0, __rt_format_char
    nop
    li $t0, 115
    beq $s5, $t0, __rt_format_string
    nop
    li $t0, 102
    beq $s5, $t0, __rt_format_float
    nop
    li $t0, 117
    xor $a3, $s5, $t0
    sltu $a3, $zero, $a3
    jal __rt_fmt_number
    nop
    b __rt_format_loop
    nop
__rt_format_percent:
    li $a0, 37
    b __rt_format_char
    nop
__rt_format_string:
    move $s6, $a0
__rt_format_string_loop:
    lbu $a0, 0($s6)
    beq $a0, $zero, __rt_format_loop
    nop
    addiu $s6, $s6, 1
    jal __rt_fmt_put
    nop
    b __rt_format_string_loop
    nop
__rt_format_float:
    l.d $f0, -8($s3)
    jal __rt_fmt_float
    nop
    b __rt_format_loop
    nop
__rt_format_done:
    li $t0, 2
    bne $s0, $t0, __rt_format_result
    nop
    sb $zero, 0($s1)
__rt_format_result:
    move $v0, $s4
    lw $s7, 4($sp)
    lw $s6, 8($sp)
    lw $s5, 12($sp)
    lw $s4, 16($sp)
    lw $s3, 20($sp)
    lw $s2, 24($sp)
    lw $s1, 28($sp)
    lw $s0, 32($sp)
    lw $ra, 36($sp)
    addiu $sp, $sp, 40
    jr $ra
    nop
__rt_fmt_put:
    addiu $sp, $sp, -8
    sw $ra, 4($sp)
    move $a1, $s0
    move $a2, $s1
    jal __rt_put
    nop
    bltz $v0, __rt_fmt_put_failed
    nop
    bltz $s4, __rt_fmt_put_counted
    nop
    addiu $s4, $s4, 1
    b __rt_fmt_put_counted
    nop
__rt_fmt_put_failed:
    li $s4, -1
__rt_fmt_put_counted:
    li $t0, 2
    bne $s0, $t0, __rt_fmt_put_end
    nop
    move $s1, $v1
__rt_fmt_put_end:
    lw $ra, 4($sp)
    addiu $sp, $sp, 8
    jr $ra
    nop
__rt_fmt_number:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $s6, 24($sp)
    sw $s7, 20($sp)
    move $s7, $a0
    beq $a3, $zero, __rt_number_digits
    nop
    bgez $s7, __rt_number_digits
    nop
    subu $s7, $zero, $s7
    li $a0, 45
    jal __rt_fmt_put
    nop
__rt_number_digits:
    move $s6, $zero
__rt_number_collect:
    li $t0, 10
    divu $s7, $t0
    mflo $s7
    mfhi $t1
    addiu $t1, $t1, 48
    addu $t2, $sp, $s6
    sb $t1, 0($t2)
    addiu $s6, $s6, 1
    bne $s7, $zero, __rt_number_collect
    nop
__rt_number_emit:
    addiu $s6, $s6, -1
    addu $t0, $sp, $s6
    lbu $a0, 0($t0)
    jal __rt_fmt_put
    nop
    bne $s6, $zero, __rt_number_emit
    nop
    lw $s7, 20($sp)
    lw $s6, 24($sp)
    lw $ra, 28($sp)
    addiu $sp, $sp, 32
    jr $ra
    nop
__rt_fmt_float:
    addiu $sp, $sp, -40
    sw $ra, 36($sp)
    sw $s6, 32($sp)
    sw $s7, 28($sp)
    s.d $f0, 0($sp)
    mtc1 $zero, $f2
    mtc1 $zero, $f3
    c.lt.d $f0, $f2
    bc1f __rt_float_positive
    nop
    li $a0, 45
    jal __rt_fmt_put
    nop
__rt_float_positive:
    l.d $f0, 0($sp)
    abs.d $f0, $f0
    trunc.w.d $f2, $f0
    mfc1 $s7, $f2
    cvt.d.w $f2, $f2
    sub.d $f0, $f0, $f2
    l.d $f2, __rt_million
    mul.d $f0, $f0, $f2
    l.d $f2, __rt_half
    add.d $f0, $f0, $f2
    trunc.w.d $f0, $f0
    mfc1 $s6, $f0
    li $t0, 1000000
    blt $s6, $t0, __rt_float_whole
    nop
    subu $s6, $s6, $t0
    addiu $s7, $s7, 1
__rt_float_whole:
    sw $s6, 16($sp)
    move $a0, $s7
    move $a3, $zero
    jal __rt_fmt_number
    nop
    li $a0, 46
    jal __rt_fmt_put
    nop
    lw $s7, 16($sp)
    li $s6, 100000
__rt_float_fraction:
    divu $s7, $s6
    mflo $a0
    mfhi $s7
    addiu $a0, $a0, 48
    jal __rt_fmt_put
    nop
    li $t0, 10
    divu $s6, $t0
    mflo $s6
    bne $s6, $zero, __rt_float_fraction
    nop
    lw $s7, 28($sp)
    lw $s6, 32($sp)
    lw $ra, 36($sp)
    addiu $sp, $sp, 40
    jr $ra
    nop

# Formatted input: same byte scanner for console, FILE and string sources.
__rt_scanf:
    move $a0, $zero
    move $a1, $zero
    lw $a2, 0($sp)
    addiu $a3, $sp, 8
    b __rt_scan
    nop
__rt_fscanf:
    li $a0, 1
    lw $a1, 0($sp)
    lw $a2, 8($sp)
    addiu $a3, $sp, 16
    b __rt_scan
    nop
__rt_sscanf:
    li $a0, 2
    lw $a1, 0($sp)
    lw $a2, 8($sp)
    addiu $a3, $sp, 16
__rt_scan:
    addiu $sp, $sp, -64
    sw $ra, 60($sp)
    sw $s0, 56($sp)
    sw $s1, 52($sp)
    sw $s2, 48($sp)
    sw $s3, 44($sp)
    sw $s4, 40($sp)
    sw $s5, 36($sp)
    sw $s6, 32($sp)
    sw $s7, 28($sp)
    move $s0, $a0
    move $s1, $a1
    move $s2, $a2
    move $s3, $a3
    move $s4, $zero
__rt_scan_loop:
    lbu $s5, 0($s2)
    addiu $s2, $s2, 1
    beq $s5, $zero, __rt_scan_done
    nop
    li $t0, 32
    beq $s5, $t0, __rt_scan_whitespace
    nop
    li $t0, 9
    blt $s5, $t0, __rt_scan_nonspace
    nop
    li $t0, 13
    ble $s5, $t0, __rt_scan_whitespace
    nop
__rt_scan_nonspace:
    li $t0, 37
    beq $s5, $t0, __rt_scan_conversion
    nop
__rt_scan_literal:
    jal __rt_scan_get
    nop
    bltz $v0, __rt_scan_input_fail
    nop
    beq $v0, $s5, __rt_scan_loop
    nop
    jal __rt_scan_unget
    nop
    b __rt_scan_done
    nop
__rt_scan_whitespace:
    jal __rt_scan_skip
    nop
    jal __rt_scan_unget
    nop
    b __rt_scan_loop
    nop
__rt_scan_conversion:
    lbu $s5, 0($s2)
    addiu $s2, $s2, 1
    li $t0, 37
    beq $s5, $t0, __rt_scan_literal
    nop
    sw $zero, 16($sp)
    li $t0, 108
    bne $s5, $t0, __rt_scan_destination
    nop
    li $t0, 1
    sw $t0, 16($sp)
    lbu $s5, 0($s2)
    addiu $s2, $s2, 1
__rt_scan_destination:
    lw $s6, 0($s3)
    addiu $s3, $s3, 8
    li $t0, 99
    beq $s5, $t0, __rt_scan_char
    nop
    jal __rt_scan_skip
    nop
    bltz $v0, __rt_scan_input_fail
    nop
    li $t0, 115
    beq $s5, $t0, __rt_scan_string
    nop
    li $s7, 1
    li $t0, 45
    beq $v0, $t0, __rt_scan_negative
    nop
    li $t0, 43
    beq $v0, $t0, __rt_scan_sign
    nop
    b __rt_scan_number_start
    nop
__rt_scan_negative:
    li $s7, -1
__rt_scan_sign:
    jal __rt_scan_get
    nop
__rt_scan_number_start:
    sw $s7, 8($sp)
    sw $zero, 12($sp)
    move $s7, $zero
    mtc1 $zero, $f4
    mtc1 $zero, $f5
__rt_scan_digits:
    li $t0, 48
    blt $v0, $t0, __rt_scan_digits_done
    nop
    li $t0, 57
    bgt $v0, $t0, __rt_scan_digits_done
    nop
    addiu $t0, $v0, -48
    li $t1, 10
    mul $s7, $s7, $t1
    addu $s7, $s7, $t0
    mtc1 $t0, $f6
    cvt.d.w $f6, $f6
    l.d $f8, __rt_ten
    mul.d $f4, $f4, $f8
    add.d $f4, $f4, $f6
    li $t0, 1
    sw $t0, 12($sp)
    jal __rt_scan_get
    nop
    b __rt_scan_digits
    nop
__rt_scan_digits_done:
    li $t0, 102
    beq $s5, $t0, __rt_scan_real
    nop
    lw $t0, 12($sp)
    beq $t0, $zero, __rt_scan_match_fail
    nop
    lw $t0, 8($sp)
    mul $s7, $s7, $t0
    sw $s7, 0($s6)
    b __rt_scan_number_done
    nop
__rt_scan_real:
    li $t0, 46
    bne $v0, $t0, __rt_scan_exponent
    nop
    l.d $f10, __rt_ten
    jal __rt_scan_get
    nop
__rt_scan_fraction:
    li $t0, 48
    blt $v0, $t0, __rt_scan_exponent
    nop
    li $t0, 57
    bgt $v0, $t0, __rt_scan_exponent
    nop
    addiu $t0, $v0, -48
    mtc1 $t0, $f6
    cvt.d.w $f6, $f6
    div.d $f6, $f6, $f10
    add.d $f4, $f4, $f6
    l.d $f8, __rt_ten
    mul.d $f10, $f10, $f8
    li $t0, 1
    sw $t0, 12($sp)
    jal __rt_scan_get
    nop
    b __rt_scan_fraction
    nop
__rt_scan_exponent:
    lw $t0, 12($sp)
    beq $t0, $zero, __rt_scan_match_fail
    nop
    li $t0, 101
    beq $v0, $t0, __rt_scan_exp_start
    nop
    li $t0, 69
    bne $v0, $t0, __rt_scan_real_store
    nop
__rt_scan_exp_start:
    move $s7, $zero
    li $t0, 1
    sw $t0, 20($sp)
    jal __rt_scan_get
    nop
    li $t0, 45
    bne $v0, $t0, __rt_scan_exp_plus
    nop
    li $t0, -1
    sw $t0, 20($sp)
    jal __rt_scan_get
    nop
    b __rt_scan_exp_digits
    nop
__rt_scan_exp_plus:
    li $t0, 43
    bne $v0, $t0, __rt_scan_exp_digits
    nop
    jal __rt_scan_get
    nop
__rt_scan_exp_digits:
    li $t0, 48
    blt $v0, $t0, __rt_scan_exp_apply
    nop
    li $t0, 57
    bgt $v0, $t0, __rt_scan_exp_apply
    nop
    li $t0, 10
    mul $s7, $s7, $t0
    addiu $t0, $v0, -48
    addu $s7, $s7, $t0
    li $t0, 308
    bgt $s7, $t0, __rt_scan_match_fail
    nop
    jal __rt_scan_get
    nop
    b __rt_scan_exp_digits
    nop
__rt_scan_exp_apply:
    l.d $f8, __rt_ten
    lw $t0, 20($sp)
__rt_scan_exp_loop:
    beq $s7, $zero, __rt_scan_real_store
    nop
    bltz $t0, __rt_scan_exp_divide
    nop
    mul.d $f4, $f4, $f8
    b __rt_scan_exp_next
    nop
__rt_scan_exp_divide:
    div.d $f4, $f4, $f8
__rt_scan_exp_next:
    addiu $s7, $s7, -1
    b __rt_scan_exp_loop
    nop
__rt_scan_real_store:
    lw $t0, 8($sp)
    bgez $t0, __rt_scan_real_positive
    nop
    neg.d $f4, $f4
__rt_scan_real_positive:
    lw $t0, 16($sp)
    beq $t0, $zero, __rt_scan_real_single
    nop
    s.d $f4, 0($s6)
    b __rt_scan_number_done
    nop
__rt_scan_real_single:
    cvt.s.d $f4, $f4
    s.s $f4, 0($s6)
__rt_scan_number_done:
    jal __rt_scan_unget
    nop
    addiu $s4, $s4, 1
    b __rt_scan_loop
    nop
__rt_scan_string:
    sb $v0, 0($s6)
    addiu $s6, $s6, 1
    jal __rt_scan_get
    nop
    bltz $v0, __rt_scan_string_done
    nop
    li $t0, 32
    beq $v0, $t0, __rt_scan_string_done
    nop
    li $t0, 9
    blt $v0, $t0, __rt_scan_string
    nop
    li $t0, 13
    bgt $v0, $t0, __rt_scan_string
    nop
__rt_scan_string_done:
    sb $zero, 0($s6)
    b __rt_scan_number_done
    nop
__rt_scan_char:
    jal __rt_scan_get
    nop
    bltz $v0, __rt_scan_input_fail
    nop
    sb $v0, 0($s6)
    addiu $s4, $s4, 1
    b __rt_scan_loop
    nop
__rt_scan_match_fail:
    jal __rt_scan_unget
    nop
    b __rt_scan_done
    nop
__rt_scan_input_fail:
    bne $s4, $zero, __rt_scan_done
    nop
    li $s4, -1
__rt_scan_done:
    move $v0, $s4
    lw $s7, 28($sp)
    lw $s6, 32($sp)
    lw $s5, 36($sp)
    lw $s4, 40($sp)
    lw $s3, 44($sp)
    lw $s2, 48($sp)
    lw $s1, 52($sp)
    lw $s0, 56($sp)
    lw $ra, 60($sp)
    addiu $sp, $sp, 64
    jr $ra
    nop
__rt_scan_get:
    addiu $sp, $sp, -8
    sw $ra, 4($sp)
    move $a0, $s0
    move $a1, $s1
    jal __rt_get
    nop
    li $t0, 2
    bne $s0, $t0, __rt_scan_get_done
    nop
    move $s1, $v1
__rt_scan_get_done:
    lw $ra, 4($sp)
    addiu $sp, $sp, 8
    jr $ra
    nop
__rt_scan_unget:
    bltz $v0, __rt_scan_unget_end
    nop
    li $t0, 2
    bne $s0, $t0, __rt_scan_unget_file
    nop
    addiu $s1, $s1, -1
    jr $ra
    nop
__rt_scan_unget_file:
    move $a0, $s0
    move $a1, $s1
    move $a2, $v0
    b __rt_unget
    nop
__rt_scan_unget_end:
    jr $ra
    nop
__rt_scan_skip:
    addiu $sp, $sp, -8
    sw $ra, 4($sp)
__rt_scan_skip_loop:
    jal __rt_scan_get
    nop
    li $t0, 32
    beq $v0, $t0, __rt_scan_skip_loop
    nop
    li $t0, 9
    blt $v0, $t0, __rt_scan_skip_end
    nop
    li $t0, 13
    ble $v0, $t0, __rt_scan_skip_loop
    nop
__rt_scan_skip_end:
    lw $ra, 4($sp)
    addiu $sp, $sp, 8
    jr $ra
    nop
__rt_atoi:
    lw $t0, 0($sp)
    li $t1, 1
    move $v0, $zero
__rt_atoi_space:
    lbu $t2, 0($t0)
    li $t3, 32
    beq $t2, $t3, __rt_atoi_skip
    nop
    li $t3, 9
    blt $t2, $t3, __rt_atoi_sign
    nop
    li $t3, 13
    bgt $t2, $t3, __rt_atoi_sign
    nop
__rt_atoi_skip:
    addiu $t0, $t0, 1
    b __rt_atoi_space
    nop
__rt_atoi_sign:
    li $t3, 45
    bne $t2, $t3, __rt_atoi_plus
    nop
    li $t1, -1
    addiu $t0, $t0, 1
    b __rt_atoi_digits
    nop
__rt_atoi_plus:
    li $t3, 43
    bne $t2, $t3, __rt_atoi_digits
    nop
    addiu $t0, $t0, 1
__rt_atoi_digits:
    lbu $t2, 0($t0)
    li $t3, 48
    blt $t2, $t3, __rt_atoi_done
    nop
    li $t3, 57
    bgt $t2, $t3, __rt_atoi_done
    nop
    li $t3, 10
    mul $v0, $v0, $t3
    addiu $t2, $t2, -48
    addu $v0, $v0, $t2
    addiu $t0, $t0, 1
    b __rt_atoi_digits
    nop
__rt_atoi_done:
    mul $v0, $v0, $t1
    jr $ra
    nop
)MIPS";
}
