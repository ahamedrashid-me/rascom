global _start

extern sc_mutex_create, sc_mutex_lock, sc_mutex_unlock, sc_mutex_trylock, sc_mutex_destroy
extern sc_semaphore_create, sc_semaphore_wait, sc_semaphore_signal
extern sc_cond_create, sc_cond_wait, sc_cond_signal, sc_cond_broadcast
extern sc_atomic_cmp_swap, sc_atomic_increment, sc_atomic_decrement
extern sc_channel_create, sc_channel_send, sc_channel_recv, sc_channel_close, sc_channel_empty, sc_channel_full
extern sc_pool_create, sc_pool_submit, sc_pool_wait, sc_pool_destroy
extern sc_socket, sc_connect, sc_bind, sc_listen, sc_accept, sc_send, sc_recv, sc_close
extern sc_fopen, sc_fread, sc_fwrite, sc_fseek, sc_fclose
extern sc_mmap, sc_munmap, sc_mprotect
extern sc_port_in, sc_port_out, sc_ioread, sc_iowrite
extern sc_syscall, sc_irq_enable, sc_irq_disable, sc_verify
extern sc_split, sc_join, sc_trim, sc_upper, sc_lower, sc_index
extern sc_replace, sc_startswith, sc_endswith, sc_reverse, sc_repeat, sc_pad
extern sc_type, sc_isqrt, sc_pow, sc_abs, sc_min, sc_max
extern sc_clz, sc_ctz, sc_popcount, sc_gcd, sc_lcm, sc_isprime
extern sc_modpow, sc_sqrt, sc_floor, sc_ceil
extern sc_error, sc_get_error_code, sc_get_error_msg, sc_clear_error
extern sc_assert, sc_check_alloc, sc_try_syscall, sc_try_fopen, sc_panic_error, sc_log_error, sc_recover
extern sc_fork, sc_wait, sc_wait_any, sc_getpid, sc_getppid
extern sc_chdir, sc_getcwd, sc_getenv, sc_setenv, sc_unsetenv
extern sc_getenv_int, sc_setenv_int, sc_exec, sc_system, sc_getrlimit, sc_setrlimit, sc_thread_count
extern sc_rwlock_create, sc_rwlock_read, sc_rwlock_read_unlock, sc_rwlock_write, sc_rwlock_write_unlock
extern sc_barrier_create, sc_barrier_wait, sc_event_create, sc_event_signal, sc_event_wait, sc_event_reset
extern sc_srand, sc_rand, sc_rand_range, sc_rand_between, sc_rand_new
extern sc_time, sc_time_ms, sc_time_us, sc_year_from_time, sc_month_from_time
extern sc_day_from_time, sc_hour_from_time, sc_minute_from_time, sc_second_from_time
extern sc_strftime, sc_strptime, sc_day_of_week, sc_day_of_year, sc_is_leap_year, sc_days_in_month
extern sc_qsort, sc_bsearch, sc_search, sc_shuffle, sc_bubble_sort
extern sc_selection_sort, sc_insertion_sort, sc_find_min, sc_find_max
extern sc_find_min_idx, sc_find_max_idx, sc_count_val, sc_sum, sc_average
extern safe_alloc_versioned, safe_free_versioned, validate_array_access, memory_safety_init

section .data
newline: db 10
bounds_msg: db 'Array bounds error!', 10
type_error_msg: db 'Type error!', 10
unimported_func_msg: db 'Unimplemented function!', 10
stack_limit_msg: db 'Recursion depth limit exceeded', 10
stack_warn_msg: db 'Warning: Approaching the recursion limit now', 10
empty_string: db '', 0       ; empty string for uninitialized str variables
g_recursion_depth: dq 0       ; global recursion depth counter (SECURITY)
heap_start: dq 0            ; stores initial heap brk for @heap_size

section .text

print_int:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov rax, rdi
    lea rsi, [rbp - 1]
    mov byte [rsi], 10     ; newline
    mov rbx, 10
    test rax, rax
    jns .convert
    neg rax
.convert:
    xor rcx, rcx
.loop:
    xor rdx, rdx
    div rbx
    add dl, '0'
    dec rsi
    mov [rsi], dl
    inc rcx
    test rax, rax
    jnz .loop
    cmp rdi, 0
    jge .print
    dec rsi
    mov byte [rsi], '-'
    inc rcx
.print:
    mov rax, 1             ; sys_write
    mov rdi, 1             ; stdout
    lea rdx, [rcx + 1]     ; length
    syscall
    leave
    ret

print_int_no_newline:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov rax, rdi
    lea rsi, [rbp - 1]
    mov rbx, 10
    test rax, rax
    jns .convert_nn
    neg rax
.convert_nn:
    xor rcx, rcx
.loop_nn:
    xor rdx, rdx
    div rbx
    add dl, '0'
    dec rsi
    mov [rsi], dl
    inc rcx
    test rax, rax
    jnz .loop_nn
    cmp rdi, 0
    jge .print_nn
    dec rsi
    mov byte [rsi], '-'
    inc rcx
.print_nn:
    mov rax, 1             ; sys_write
    mov rdi, 1             ; stdout
    mov rdx, rcx           ; length (no newline)
    syscall
    leave
    ret

print_float:
    push rbp
    mov rbp, rsp
    sub rsp, 64             ; space for buffer and saving xmm0
    movsd [rbp-64], xmm0    ; save original value
    ; Check for negative
    xorpd xmm1, xmm1
    ucomisd xmm0, xmm1
    jae .pf_positive
    ; Print minus sign
    mov byte [rbp-1], '-'
    mov rdi, 1
    lea rsi, [rbp-1]
    mov rdx, 1
    mov rax, 1              ; sys_write
    syscall
    ; Make positive
    movsd xmm1, xmm0
    xorpd xmm0, xmm0
    subsd xmm0, xmm1
    movsd [rbp-64], xmm0    ; save positive value
.pf_positive:
    ; Extract integer part
    cvttsd2si rdi, xmm0     ; convert to int
    call print_int_no_newline
    ; Print decimal point
    mov byte [rbp-1], '.'
    mov rdi, 1
    lea rsi, [rbp-1]
    mov rdx, 1
    mov rax, 1
    syscall
    ; Extract fractional part
    movsd xmm0, [rbp-64]    ; reload positive value
    cvttsd2si rax, xmm0     ; get integer part
    cvtsi2sd xmm1, rax      ; convert back to double
    subsd xmm0, xmm1        ; xmm0 = fractional part
    ; Multiply by 100 for 2 decimal places
    mov rax, 100
    cvtsi2sd xmm1, rax
    mulsd xmm0, xmm1
    cvttsd2si rdi, xmm0     ; convert to int
    ; Pad with zero if needed
    cmp rdi, 10
    jge .pf_print_frac
    ; Print leading zero
    mov byte [rbp-1], '0'
    mov rax, 1
    mov rdx, 1
    push rdi                ; save fraction
    mov rdi, 1
    lea rsi, [rbp-1]
    syscall
    pop rdi                 ; restore fraction
.pf_print_frac:
    call print_int_no_newline
    leave
    ret

strlen:
    push rbp
    mov rbp, rsp
    xor rax, rax            ; counter
.strlen_loop:
    cmp byte [rdi+rax], 0
    je .strlen_done
    inc rax
    jmp .strlen_loop
.strlen_done:
    leave
    ret

strcpy:
    push rbp
    mov rbp, rsp
    mov rax, rdi            ; save dest
.strcpy_loop:
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .strcpy_done
    inc rdi
    inc rsi
    jmp .strcpy_loop
.strcpy_done:
    leave
    ret

strcmp:
    push rbp
    mov rbp, rsp
.strcmp_loop:
    mov al, [rdi]
    mov cl, [rsi]
    cmp al, cl
    jne .strcmp_diff
    test al, al
    jz .strcmp_equal
    inc rdi
    inc rsi
    jmp .strcmp_loop
.strcmp_equal:
    xor rax, rax
    leave
    ret
.strcmp_diff:
    movzx rax, al
    movzx rcx, cl
    sub rax, rcx
    leave
    ret

malloc:
    push rbp
    mov rbp, rsp
    ; Simple bump allocator using brk syscall
    mov rsi, rdi            ; size to allocate
    mov rax, 12             ; sys_brk
    xor rdi, rdi            ; get current brk
    syscall
    mov rbx, rax            ; save current brk
    add rax, rsi            ; new brk = current + size
    mov rdi, rax
    mov rax, 12             ; sys_brk
    syscall
    mov rax, rbx            ; return old brk
    leave
    ret

runtime_type_error:
    mov rdi, 1
    lea rsi, [rel type_error_msg]
    mov rdx, 12
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1
    mov rax, 60         ; sys_exit
    syscall

runtime_unimplemented_func:
    mov rdi, 1
    lea rsi, [rel unimported_func_msg]
    mov rdx, 24
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1
    mov rax, 60         ; sys_exit
    syscall

; Package imports
; import: test



test_system:
    push rbp
    mov rbp, rsp
    inc qword [rel g_recursion_depth]  ; increment depth counter
    mov rax, [rel g_recursion_depth]
    cmp rax, 100000                       ; MAX_RECURSION_DEPTH
    jle .recursion_ok_0
    dec qword [rel g_recursion_depth]  ; undo increment on error
    mov rdi, 1          ; stderr
    lea rsi, [rel stack_limit_msg]
    mov rdx, 31         ; message length
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1          ; exit code
    mov rax, 60         ; sys_exit
    syscall
.recursion_ok_0:
    cmp rax, 90000         ; STACK_WARN_THRESHOLD
    jle .stack_ok_1
    mov rdi, 2          ; stderr
    lea rsi, [rel stack_warn_msg]
    mov rdx, 45         ; message length
    mov rax, 1          ; sys_write
    syscall
.stack_ok_1:
    lea rax, [rel .STR2]
section .data
.STR2: db 0x0a, 0x3d, 0x3d, 0x3d, 0x20, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x20, 0x43, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x54, 0x65, 0x73, 0x74, 0x73, 0x20, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L3:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L5
    cmp byte [rsi], 0
    je .L4
    inc rdx
    inc rsi
    jmp .L3
.L5:
    mov rdx, 0          ; truncate to 0 on overflow
.L4:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @clock
    sub rsp, 16
    mov rdi, 1          ; CLOCK_MONOTONIC
    lea rsi, [rsp]
    mov rax, 228        ; sys_clock_gettime
    syscall
    mov rax, [rsp]      ; rax = tv_sec
    imul rax, 1000      ; rax = tv_sec * 1000 ms
    mov rcx, [rsp+8]    ; rcx = tv_nsec
    mov rbx, 1000000
    mov rax, rcx
    xor rdx, rdx
    div rbx             ; rax = tv_nsec / 1000000 ms
    mov rcx, [rsp]      ; rcx = tv_sec
    imul rcx, 1000      ; rcx = tv_sec * 1000 ms
    add rax, rcx        ; rax = total ms
    add rsp, 16
    push rax            ; var start
    lea rax, [rel .STR6]
section .data
.STR6: db 0x40, 0x63, 0x6c, 0x6f, 0x63, 0x6b, 0x5b, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L7:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L9
    cmp byte [rsi], 0
    je .L8
    inc rdx
    inc rsi
    jmp .L7
.L9:
    mov rdx, 0          ; truncate to 0 on overflow
.L8:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 8]  ; load start
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR10]
section .data
.STR10: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L11:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L13
    cmp byte [rsi], 0
    je .L12
    inc rdx
    inc rsi
    jmp .L11
.L13:
    mov rdx, 0          ; truncate to 0 on overflow
.L12:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    lea rax, [rel .STR14]
section .data
.STR14: db 0x40, 0x73, 0x6c, 0x65, 0x65, 0x70, 0x5b, 0x31, 0x30, 0x30, 0x5d, 0x20, 0x2d, 0x20, 0x73, 0x6c, 0x65, 0x65, 0x70, 0x69, 0x6e, 0x67, 0x20, 0x31, 0x30, 0x30, 0x6d, 0x73, 0x2e, 0x2e, 0x2e, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L15:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L17
    cmp byte [rsi], 0
    je .L16
    inc rdx
    inc rsi
    jmp .L15
.L17:
    mov rdx, 0          ; truncate to 0 on overflow
.L16:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @sleep
    mov rax, 100
    ; Convert milliseconds to nanoseconds
    mov rbx, 1000000
    imul rax, rbx       ; rax = ms * 1000000 = nanoseconds
    ; Create timespec struct on stack: tv_sec=0, tv_nsec=nanoseconds
    sub rsp, 32         ; Allocate space for two timespec structs
    mov qword [rsp], 0  ; tv_sec = 0
    mov qword [rsp+8], rax ; tv_nsec = nanoseconds
    lea rdi, [rsp]      ; rdi = ptr to req timespec
    lea rsi, [rsp+16]   ; rsi = ptr to rem timespec
    mov rax, 35         ; sys_nanosleep
    syscall
    add rsp, 32         ; Clean up stack
    ; Builtin function @clock
    sub rsp, 16
    mov rdi, 1          ; CLOCK_MONOTONIC
    lea rsi, [rsp]
    mov rax, 228        ; sys_clock_gettime
    syscall
    mov rax, [rsp]      ; rax = tv_sec
    imul rax, 1000      ; rax = tv_sec * 1000 ms
    mov rcx, [rsp+8]    ; rcx = tv_nsec
    mov rbx, 1000000
    mov rax, rcx
    xor rdx, rdx
    div rbx             ; rax = tv_nsec / 1000000 ms
    mov rcx, [rsp]      ; rcx = tv_sec
    imul rcx, 1000      ; rcx = tv_sec * 1000 ms
    add rax, rcx        ; rax = total ms
    add rsp, 16
    push rax            ; var end
    mov rax, [rbp - 16]  ; load end
    push rax
    mov rax, [rbp - 8]  ; load start
    mov rbx, rax
    pop rax
    sub rax, rbx
    jo .overflow_18       ; jump if overflow flag set
    jmp .overflow_done_19
.overflow_18:
    mov rax, -1            ; return -1 to signal overflow
.overflow_done_19:
    push rax            ; var elapsed
    lea rax, [rel .STR20]
section .data
.STR20: db 0x45, 0x6c, 0x61, 0x70, 0x73, 0x65, 0x64, 0x3a, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L21:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L23
    cmp byte [rsi], 0
    je .L22
    inc rdx
    inc rsi
    jmp .L21
.L23:
    mov rdx, 0          ; truncate to 0 on overflow
.L22:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 24]  ; load elapsed
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR24]
section .data
.STR24: db 0x20, 0x6d, 0x73, 0x20, 0x28, 0x73, 0x68, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x62, 0x65, 0x20, 0x7e, 0x31, 0x30, 0x30, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L25:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L27
    cmp byte [rsi], 0
    je .L26
    inc rdx
    inc rsi
    jmp .L25
.L27:
    mov rdx, 0          ; truncate to 0 on overflow
.L26:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @compiler_ver
    lea rax, [rel .compiler_ver_28]
section .data
.compiler_ver_28: db "rascode b-0.0.1", 0
section .text
    push rax            ; var ver
    lea rax, [rel .STR29]
section .data
.STR29: db 0x40, 0x63, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72, 0x5f, 0x76, 0x65, 0x72, 0x5b, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L30:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L32
    cmp byte [rsi], 0
    je .L31
    inc rdx
    inc rsi
    jmp .L30
.L32:
    mov rdx, 0          ; truncate to 0 on overflow
.L31:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 32]  ; load ver
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR33]
section .data
.STR33: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L34:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L36
    cmp byte [rsi], 0
    je .L35
    inc rdx
    inc rsi
    jmp .L34
.L36:
    mov rdx, 0          ; truncate to 0 on overflow
.L35:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, 0
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 32         ; pop 4 block-local variables
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

test_memory:
    push rbp
    mov rbp, rsp
    inc qword [rel g_recursion_depth]  ; increment depth counter
    mov rax, [rel g_recursion_depth]
    cmp rax, 100000                       ; MAX_RECURSION_DEPTH
    jle .recursion_ok_37
    dec qword [rel g_recursion_depth]  ; undo increment on error
    mov rdi, 1          ; stderr
    lea rsi, [rel stack_limit_msg]
    mov rdx, 31         ; message length
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1          ; exit code
    mov rax, 60         ; sys_exit
    syscall
.recursion_ok_37:
    cmp rax, 90000         ; STACK_WARN_THRESHOLD
    jle .stack_ok_38
    mov rdi, 2          ; stderr
    lea rsi, [rel stack_warn_msg]
    mov rdx, 45         ; message length
    mov rax, 1          ; sys_write
    syscall
.stack_ok_38:
    lea rax, [rel .STR39]
section .data
.STR39: db 0x0a, 0x3d, 0x3d, 0x3d, 0x20, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x20, 0x4f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x20, 0x54, 0x65, 0x73, 0x74, 0x73, 0x20, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L40:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L42
    cmp byte [rsi], 0
    je .L41
    inc rdx
    inc rsi
    jmp .L40
.L42:
    mov rdx, 0          ; truncate to 0 on overflow
.L41:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @alloc
    mov rax, 64
    mov rdi, rax        ; size to allocate
    add rdi, 8          ; add 8 bytes for size metadata
    push rdi            ; save total size
    xor rdi, rdi        ; get current brk
    mov rax, 12         ; sys_brk
    syscall
    mov rbx, rax        ; save current brk (metadata location)
    pop rdi             ; restore total size
    add rdi, rbx        ; new brk = old + size
    mov rax, 12         ; sys_brk
    syscall
    cmp rax, rdi        ; check if succeeded
    jne .alloc_fail_43
    mov [rbx], rdi      ; store size metadata
    add rbx, 8          ; skip metadata for user data
    mov rax, rbx        ; return data ptr (after metadata)
    jmp .alloc_done_43
.alloc_fail_43:
    xor rax, rax        ; return 0 on failure
.alloc_done_43:
    push rax            ; var ptr
    lea rax, [rel .STR44]
section .data
.STR44: db 0x40, 0x61, 0x6c, 0x6c, 0x6f, 0x63, 0x5b, 0x36, 0x34, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L45:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L47
    cmp byte [rsi], 0
    je .L46
    inc rdx
    inc rsi
    jmp .L45
.L47:
    mov rdx, 0          ; truncate to 0 on overflow
.L46:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 8]  ; load ptr
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR48]
section .data
.STR48: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L49:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L51
    cmp byte [rsi], 0
    je .L50
    inc rdx
    inc rsi
    jmp .L49
.L51:
    mov rdx, 0          ; truncate to 0 on overflow
.L50:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, [rbp - 8]  ; load ptr
    push rax
    mov rax, 0
    mov rbx, rax
    pop rax
    cmp rax, rbx
    sete al
    movzx rax, al
    cmp rax, 0
    je .L52
    lea rax, [rel .STR54]
section .data
.STR54: db 0x45, 0x52, 0x52, 0x4f, 0x52, 0x3a, 0x20, 0x41, 0x6c, 0x6c, 0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x66, 0x61, 0x69, 0x6c, 0x65, 0x64, 0x21, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L55:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L57
    cmp byte [rsi], 0
    je .L56
    inc rdx
    inc rsi
    jmp .L55
.L57:
    mov rdx, 0          ; truncate to 0 on overflow
.L56:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, 1
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    jmp .L53
.L52:
.L53:
    ; Builtin function @poke
    mov rax, [rbp - 8]  ; load ptr
    mov rcx, rax        ; address to write
    mov rax, 42
    mov [rcx], al       ; write byte
    ; Builtin function @peek
    mov rax, [rbp - 8]  ; load ptr
    mov rcx, rax        ; address to read
    mov al, [rcx]       ; read byte
    movzx rax, al       ; zero extend to 64-bit
    push rax            ; var val
    lea rax, [rel .STR58]
section .data
.STR58: db 0x40, 0x70, 0x6f, 0x6b, 0x65, 0x5b, 0x70, 0x74, 0x72, 0x2c, 0x20, 0x34, 0x32, 0x5d, 0x20, 0x2f, 0x20, 0x40, 0x70, 0x65, 0x65, 0x6b, 0x5b, 0x70, 0x74, 0x72, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L59:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L61
    cmp byte [rsi], 0
    je .L60
    inc rdx
    inc rsi
    jmp .L59
.L61:
    mov rdx, 0          ; truncate to 0 on overflow
.L60:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 16]  ; load val
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR62]
section .data
.STR62: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x34, 0x32, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L63:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L65
    cmp byte [rsi], 0
    je .L64
    inc rdx
    inc rsi
    jmp .L63
.L65:
    mov rdx, 0          ; truncate to 0 on overflow
.L64:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @poke
    mov rax, [rbp - 8]  ; load ptr
    push rax
    mov rax, 8
    mov rbx, rax
    pop rax
    add rax, rbx
    jo .overflow_66       ; jump if overflow flag set
    jmp .overflow_done_67
.overflow_66:
    mov rax, -1            ; return -1 to signal overflow
.overflow_done_67:
    mov rcx, rax        ; address to write
    mov rax, 100
    mov [rcx], al       ; write byte
    ; Builtin function @peek
    mov rax, [rbp - 8]  ; load ptr
    push rax
    mov rax, 8
    mov rbx, rax
    pop rax
    add rax, rbx
    jo .overflow_68       ; jump if overflow flag set
    jmp .overflow_done_69
.overflow_68:
    mov rax, -1            ; return -1 to signal overflow
.overflow_done_69:
    mov rcx, rax        ; address to read
    mov al, [rcx]       ; read byte
    movzx rax, al       ; zero extend to 64-bit
    push rax            ; var val2
    lea rax, [rel .STR70]
section .data
.STR70: db 0x40, 0x70, 0x65, 0x65, 0x6b, 0x5b, 0x70, 0x74, 0x72, 0x20, 0x2b, 0x20, 0x38, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L71:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L73
    cmp byte [rsi], 0
    je .L72
    inc rdx
    inc rsi
    jmp .L71
.L73:
    mov rdx, 0          ; truncate to 0 on overflow
.L72:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 24]  ; load val2
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR74]
section .data
.STR74: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x31, 0x30, 0x30, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L75:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L77
    cmp byte [rsi], 0
    je .L76
    inc rdx
    inc rsi
    jmp .L75
.L77:
    mov rdx, 0          ; truncate to 0 on overflow
.L76:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @memclr
    mov rax, [rbp - 8]  ; load ptr
    push rax
    mov rax, 16
    mov rcx, rax
    pop rdi
    xor al, al
    rep stosb
    ; Builtin function @peek
    mov rax, [rbp - 8]  ; load ptr
    mov rcx, rax        ; address to read
    mov al, [rcx]       ; read byte
    movzx rax, al       ; zero extend to 64-bit
    push rax            ; var cleared
    lea rax, [rel .STR78]
section .data
.STR78: db 0x40, 0x6d, 0x65, 0x6d, 0x63, 0x6c, 0x72, 0x5b, 0x70, 0x74, 0x72, 0x2c, 0x20, 0x31, 0x36, 0x5d, 0x20, 0x2f, 0x20, 0x40, 0x70, 0x65, 0x65, 0x6b, 0x5b, 0x70, 0x74, 0x72, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L79:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L81
    cmp byte [rsi], 0
    je .L80
    inc rdx
    inc rsi
    jmp .L79
.L81:
    mov rdx, 0          ; truncate to 0 on overflow
.L80:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 32]  ; load cleared
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR82]
section .data
.STR82: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x30, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L83:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L85
    cmp byte [rsi], 0
    je .L84
    inc rdx
    inc rsi
    jmp .L83
.L85:
    mov rdx, 0          ; truncate to 0 on overflow
.L84:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @memset
    mov rax, [rbp - 8]  ; load ptr
    push rax            ; save ptr
    mov rax, 255
    push rax            ; save value
    mov rax, 8
    mov rcx, rax        ; count
    pop rax             ; restore value
    pop rdi             ; restore ptr
    rep stosb           ; fill memory
    ; Builtin function @peek
    mov rax, [rbp - 8]  ; load ptr
    mov rcx, rax        ; address to read
    mov al, [rcx]       ; read byte
    movzx rax, al       ; zero extend to 64-bit
    push rax            ; var filled
    lea rax, [rel .STR86]
section .data
.STR86: db 0x40, 0x6d, 0x65, 0x6d, 0x73, 0x65, 0x74, 0x5b, 0x70, 0x74, 0x72, 0x2c, 0x20, 0x30, 0x78, 0x46, 0x46, 0x2c, 0x20, 0x38, 0x5d, 0x20, 0x2f, 0x20, 0x40, 0x70, 0x65, 0x65, 0x6b, 0x5b, 0x70, 0x74, 0x72, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L87:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L89
    cmp byte [rsi], 0
    je .L88
    inc rdx
    inc rsi
    jmp .L87
.L89:
    mov rdx, 0          ; truncate to 0 on overflow
.L88:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 40]  ; load filled
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR90]
section .data
.STR90: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L91:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L93
    cmp byte [rsi], 0
    je .L92
    inc rdx
    inc rsi
    jmp .L91
.L93:
    mov rdx, 0          ; truncate to 0 on overflow
.L92:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @realloc
    mov rax, [rbp - 8]  ; load ptr
    mov rbx, rax        ; old_ptr in rbx
    mov rax, 128
    mov rcx, rax        ; new_size in rcx
    mov rax, [rbx - 8]  ; read old size from metadata
    sub rax, 8          ; adjust for metadata size
    mov r9, rax         ; old_size in r9
    push rbx            ; save old_ptr
    push rcx            ; save new_size
    push r9             ; save old_size
    mov rdi, rcx        ; new_size
    add rdi, 8          ; add metadata overhead
    xor rax, 0          ; get current brk
    mov rax, 12         ; sys_brk
    syscall
    mov r8, rax         ; new_ptr (metadata location) in r8
    pop r9              ; restore old_size
    pop rcx             ; restore new_size
    add rax, rcx        ; new brk = current + new_size + 8
    add rax, 8
    mov rdi, rax
    mov rax, 12         ; sys_brk
    syscall
    cmp rax, rdi        ; check success
    jne .realloc_fail_94
    mov rax, rcx        ; new_size
    add rax, 8          ; with metadata
    mov [r8], rax       ; store total size in metadata
    add r8, 8           ; adjust to user ptr
    pop rbx             ; old_ptr
    mov rdi, r8         ; dest = new_ptr
    mov rsi, rbx        ; src = old_ptr
    mov rcx, r9         ; old_size
    cmp rcx, [rsp]      ; compare old_size with new_size
    jle .realloc_copy_94
    mov rcx, [rsp]      ; use new_size if smaller
.realloc_copy_94:
    rep movsb           ; copy data
    add rsp, 8          ; clean stack
    mov rax, r8         ; return new_ptr
    jmp .realloc_done_94
.realloc_fail_94:
    add rsp, 16         ; clean up stack
    xor rax, rax        ; return 0 on failure
.realloc_done_94:
    mov [rbp - 8], rax  ; assign ptr
    lea rax, [rel .STR95]
section .data
.STR95: db 0x40, 0x72, 0x65, 0x61, 0x6c, 0x6c, 0x6f, 0x63, 0x5b, 0x70, 0x74, 0x72, 0x2c, 0x20, 0x31, 0x32, 0x38, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L96:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L98
    cmp byte [rsi], 0
    je .L97
    inc rdx
    inc rsi
    jmp .L96
.L98:
    mov rdx, 0          ; truncate to 0 on overflow
.L97:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 8]  ; load ptr
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR99]
section .data
.STR99: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L100:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L102
    cmp byte [rsi], 0
    je .L101
    inc rdx
    inc rsi
    jmp .L100
.L102:
    mov rdx, 0          ; truncate to 0 on overflow
.L101:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @alloc
    mov rax, 64
    mov rdi, rax        ; size to allocate
    add rdi, 8          ; add 8 bytes for size metadata
    push rdi            ; save total size
    xor rdi, rdi        ; get current brk
    mov rax, 12         ; sys_brk
    syscall
    mov rbx, rax        ; save current brk (metadata location)
    pop rdi             ; restore total size
    add rdi, rbx        ; new brk = old + size
    mov rax, 12         ; sys_brk
    syscall
    cmp rax, rdi        ; check if succeeded
    jne .alloc_fail_103
    mov [rbx], rdi      ; store size metadata
    add rbx, 8          ; skip metadata for user data
    mov rax, rbx        ; return data ptr (after metadata)
    jmp .alloc_done_103
.alloc_fail_103:
    xor rax, rax        ; return 0 on failure
.alloc_done_103:
    push rax            ; var dest
    ; Builtin function @poke
    mov rax, [rbp - 8]  ; load ptr
    mov rcx, rax        ; address to write
    mov rax, 12345
    mov [rcx], al       ; write byte
    ; Builtin function @memcpy
    mov rax, [rbp - 8]  ; load ptr
    mov rsi, rax        ; src for rep movsb
    mov rax, [rbp - 48]  ; load dest
    mov rdi, rax        ; dest for rep movsb
    mov rax, 8
    mov rcx, rax        ; count
    rep movsb           ; copy bytes
    mov rax, 0          ; return 0 on success
    ; Builtin function @peek
    mov rax, [rbp - 48]  ; load dest
    mov rcx, rax        ; address to read
    mov al, [rcx]       ; read byte
    movzx rax, al       ; zero extend to 64-bit
    push rax            ; var copied
    lea rax, [rel .STR104]
section .data
.STR104: db 0x40, 0x6d, 0x65, 0x6d, 0x63, 0x70, 0x79, 0x20, 0x2f, 0x20, 0x40, 0x70, 0x65, 0x65, 0x6b, 0x5b, 0x64, 0x65, 0x73, 0x74, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L105:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L107
    cmp byte [rsi], 0
    je .L106
    inc rdx
    inc rsi
    jmp .L105
.L107:
    mov rdx, 0          ; truncate to 0 on overflow
.L106:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 56]  ; load copied
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR108]
section .data
.STR108: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x31, 0x32, 0x33, 0x34, 0x35, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L109:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L111
    cmp byte [rsi], 0
    je .L110
    inc rdx
    inc rsi
    jmp .L109
.L111:
    mov rdx, 0          ; truncate to 0 on overflow
.L110:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @memcmp
    mov rax, [rbp - 8]  ; load ptr
    push rax
    mov rax, [rbp - 48]  ; load dest
    push rax
    mov rax, 8
    mov rcx, rax
    pop rsi
    pop rdi
    repe cmpsb          ; compare bytes
    setz al             ; 1 if equal, 0 if not
    movzx rax, al
    push rax            ; var cmp
    lea rax, [rel .STR112]
section .data
.STR112: db 0x40, 0x6d, 0x65, 0x6d, 0x63, 0x6d, 0x70, 0x5b, 0x70, 0x74, 0x72, 0x2c, 0x20, 0x64, 0x65, 0x73, 0x74, 0x2c, 0x20, 0x38, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L113:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L115
    cmp byte [rsi], 0
    je .L114
    inc rdx
    inc rsi
    jmp .L113
.L115:
    mov rdx, 0          ; truncate to 0 on overflow
.L114:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 64]  ; load cmp
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR116]
section .data
.STR116: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x30, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L117:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L119
    cmp byte [rsi], 0
    je .L118
    inc rdx
    inc rsi
    jmp .L117
.L119:
    mov rdx, 0          ; truncate to 0 on overflow
.L118:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @page_size
    mov rax, 4096
    push rax            ; var pgsz
    lea rax, [rel .STR120]
section .data
.STR120: db 0x40, 0x70, 0x61, 0x67, 0x65, 0x5f, 0x73, 0x69, 0x7a, 0x65, 0x5b, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L121:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L123
    cmp byte [rsi], 0
    je .L122
    inc rdx
    inc rsi
    jmp .L121
.L123:
    mov rdx, 0          ; truncate to 0 on overflow
.L122:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 72]  ; load pgsz
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR124]
section .data
.STR124: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L125:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L127
    cmp byte [rsi], 0
    je .L126
    inc rdx
    inc rsi
    jmp .L125
.L127:
    mov rdx, 0          ; truncate to 0 on overflow
.L126:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @heap_start
    xor rdi, rdi
    mov rax, 12         ; sys_brk
    syscall             ; returns current brk
    push rax            ; var hs
    ; Builtin function @heap_end
    xor rdi, rdi
    mov rax, 12         ; sys_brk
    syscall
    push rax            ; var he
    ; Builtin function @heap_size
    xor rdi, rdi        ; get current brk
    mov rax, 12         ; sys_brk
    syscall
    mov rbx, rax        ; current heap end
    lea rax, [rel heap_start]
    mov r8, [rax]       ; load stored heap start
    cmp r8, 0           ; check if initialized
    jne .heap_size_calc_128
    mov [rax], rbx      ; store initial heap start
    xor rax, rax        ; first call returns 0
    jmp .heap_size_done_128
.heap_size_calc_128:
    mov rax, rbx        ; current end
    sub rax, r8         ; subtract start
.heap_size_done_128:
    push rax            ; var hsz
    lea rax, [rel .STR129]
section .data
.STR129: db 0x40, 0x68, 0x65, 0x61, 0x70, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x5b, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L130:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L132
    cmp byte [rsi], 0
    je .L131
    inc rdx
    inc rsi
    jmp .L130
.L132:
    mov rdx, 0          ; truncate to 0 on overflow
.L131:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 80]  ; load hs
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR133]
section .data
.STR133: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L134:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L136
    cmp byte [rsi], 0
    je .L135
    inc rdx
    inc rsi
    jmp .L134
.L136:
    mov rdx, 0          ; truncate to 0 on overflow
.L135:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    lea rax, [rel .STR137]
section .data
.STR137: db 0x40, 0x68, 0x65, 0x61, 0x70, 0x5f, 0x65, 0x6e, 0x64, 0x5b, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L138:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L140
    cmp byte [rsi], 0
    je .L139
    inc rdx
    inc rsi
    jmp .L138
.L140:
    mov rdx, 0          ; truncate to 0 on overflow
.L139:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 88]  ; load he
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR141]
section .data
.STR141: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L142:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L144
    cmp byte [rsi], 0
    je .L143
    inc rdx
    inc rsi
    jmp .L142
.L144:
    mov rdx, 0          ; truncate to 0 on overflow
.L143:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    lea rax, [rel .STR145]
section .data
.STR145: db 0x40, 0x68, 0x65, 0x61, 0x70, 0x5f, 0x73, 0x69, 0x7a, 0x65, 0x5b, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L146:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L148
    cmp byte [rsi], 0
    je .L147
    inc rdx
    inc rsi
    jmp .L146
.L148:
    mov rdx, 0          ; truncate to 0 on overflow
.L147:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 96]  ; load hsz
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR149]
section .data
.STR149: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L150:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L152
    cmp byte [rsi], 0
    je .L151
    inc rdx
    inc rsi
    jmp .L150
.L152:
    mov rdx, 0          ; truncate to 0 on overflow
.L151:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @align
    xor rax, rax        ; unsupported
    push rax            ; var aligned
    lea rax, [rel .STR153]
section .data
.STR153: db 0x40, 0x61, 0x6c, 0x69, 0x67, 0x6e, 0x5b, 0x70, 0x74, 0x72, 0x2c, 0x20, 0x31, 0x36, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L154:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L156
    cmp byte [rsi], 0
    je .L155
    inc rdx
    inc rsi
    jmp .L154
.L156:
    mov rdx, 0          ; truncate to 0 on overflow
.L155:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 104]  ; load aligned
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR157]
section .data
.STR157: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L158:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L160
    cmp byte [rsi], 0
    je .L159
    inc rdx
    inc rsi
    jmp .L158
.L160:
    mov rdx, 0          ; truncate to 0 on overflow
.L159:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @free
    mov rax, [rbp - 8]  ; load ptr
    mov rdi, rax        ; user ptr
    sub rdi, 8          ; get metadata ptr
    mov qword [rdi], 0  ; mark as freed
    xor rax, rax        ; return 0
    ; Builtin function @free
    mov rax, [rbp - 48]  ; load dest
    mov rdi, rax        ; user ptr
    sub rdi, 8          ; get metadata ptr
    mov qword [rdi], 0  ; mark as freed
    xor rax, rax        ; return 0
    lea rax, [rel .STR161]
section .data
.STR161: db 0x40, 0x66, 0x72, 0x65, 0x65, 0x20, 0x64, 0x6f, 0x6e, 0x65, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L162:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L164
    cmp byte [rsi], 0
    je .L163
    inc rdx
    inc rsi
    jmp .L162
.L164:
    mov rdx, 0          ; truncate to 0 on overflow
.L163:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, 0
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 104         ; pop 13 block-local variables
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

test_type_conversion:
    push rbp
    mov rbp, rsp
    inc qword [rel g_recursion_depth]  ; increment depth counter
    mov rax, [rel g_recursion_depth]
    cmp rax, 100000                       ; MAX_RECURSION_DEPTH
    jle .recursion_ok_165
    dec qword [rel g_recursion_depth]  ; undo increment on error
    mov rdi, 1          ; stderr
    lea rsi, [rel stack_limit_msg]
    mov rdx, 31         ; message length
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1          ; exit code
    mov rax, 60         ; sys_exit
    syscall
.recursion_ok_165:
    cmp rax, 90000         ; STACK_WARN_THRESHOLD
    jle .stack_ok_166
    mov rdi, 2          ; stderr
    lea rsi, [rel stack_warn_msg]
    mov rdx, 45         ; message length
    mov rax, 1          ; sys_write
    syscall
.stack_ok_166:
    lea rax, [rel .STR167]
section .data
.STR167: db 0x0a, 0x3d, 0x3d, 0x3d, 0x20, 0x54, 0x79, 0x70, 0x65, 0x20, 0x43, 0x6f, 0x6e, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x54, 0x65, 0x73, 0x74, 0x73, 0x20, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L168:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L170
    cmp byte [rsi], 0
    je .L169
    inc rdx
    inc rsi
    jmp .L168
.L170:
    mov rdx, 0          ; truncate to 0 on overflow
.L169:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @len
    lea rax, [rel .STR171]
section .data
.STR171: db 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00
section .text
    mov rdi, rax
    xor rcx, rcx
.len_loop_172:
    cmp byte [rdi+rcx], 0
    je .len_done_172
    inc rcx
    jmp .len_loop_172
.len_done_172:
    mov rax, rcx
    push rax            ; var len1
    lea rax, [rel .STR173]
section .data
.STR173: db 0x40, 0x6c, 0x65, 0x6e, 0x5b, 0x22, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x22, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L174:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L176
    cmp byte [rsi], 0
    je .L175
    inc rdx
    inc rsi
    jmp .L174
.L176:
    mov rdx, 0          ; truncate to 0 on overflow
.L175:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 8]  ; load len1
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR177]
section .data
.STR177: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x35, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L178:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L180
    cmp byte [rsi], 0
    je .L179
    inc rdx
    inc rsi
    jmp .L178
.L180:
    mov rdx, 0          ; truncate to 0 on overflow
.L179:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @ord
    mov rax, 65
    movzx rax, al       ; zero extend byte
    push rax            ; var code
    lea rax, [rel .STR181]
section .data
.STR181: db 0x40, 0x6f, 0x72, 0x64, 0x5b, 0x27, 0x41, 0x27, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L182:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L184
    cmp byte [rsi], 0
    je .L183
    inc rdx
    inc rsi
    jmp .L182
.L184:
    mov rdx, 0          ; truncate to 0 on overflow
.L183:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 16]  ; load code
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR185]
section .data
.STR185: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x36, 0x35, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L186:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L188
    cmp byte [rsi], 0
    je .L187
    inc rdx
    inc rsi
    jmp .L186
.L188:
    mov rdx, 0          ; truncate to 0 on overflow
.L187:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @chr
    mov rax, 65
    and rax, 0xFF       ; keep only byte
    push rax            ; var ch
    lea rax, [rel .STR189]
section .data
.STR189: db 0x40, 0x63, 0x68, 0x72, 0x5b, 0x36, 0x35, 0x5d, 0x20, 0x3d, 0x20, 0x41, 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x41, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L190:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L192
    cmp byte [rsi], 0
    je .L191
    inc rdx
    inc rsi
    jmp .L190
.L192:
    mov rdx, 0          ; truncate to 0 on overflow
.L191:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, 0
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 24         ; pop 3 block-local variables
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

test_strings:
    push rbp
    mov rbp, rsp
    inc qword [rel g_recursion_depth]  ; increment depth counter
    mov rax, [rel g_recursion_depth]
    cmp rax, 100000                       ; MAX_RECURSION_DEPTH
    jle .recursion_ok_193
    dec qword [rel g_recursion_depth]  ; undo increment on error
    mov rdi, 1          ; stderr
    lea rsi, [rel stack_limit_msg]
    mov rdx, 31         ; message length
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1          ; exit code
    mov rax, 60         ; sys_exit
    syscall
.recursion_ok_193:
    cmp rax, 90000         ; STACK_WARN_THRESHOLD
    jle .stack_ok_194
    mov rdi, 2          ; stderr
    lea rsi, [rel stack_warn_msg]
    mov rdx, 45         ; message length
    mov rax, 1          ; sys_write
    syscall
.stack_ok_194:
    lea rax, [rel .STR195]
section .data
.STR195: db 0x0a, 0x3d, 0x3d, 0x3d, 0x20, 0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x20, 0x4f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x20, 0x54, 0x65, 0x73, 0x74, 0x73, 0x20, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L196:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L198
    cmp byte [rsi], 0
    je .L197
    inc rdx
    inc rsi
    jmp .L196
.L198:
    mov rdx, 0          ; truncate to 0 on overflow
.L197:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @concat
    lea rax, [rel .STR199]
section .data
.STR199: db 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x00
section .text
    mov r12, rax        ; r12 = str1
    lea rax, [rel .STR200]
section .data
.STR200: db 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x00
section .text
    mov r13, rax        ; r13 = str2
    mov rdi, r12
    call strlen
    mov r14, rax        ; r14 = len1
    mov rdi, r13
    call strlen
    mov r15, rax        ; r15 = len2
    lea rdi, [r14 + r15 + 1]  ; total size
    call malloc
    mov rbx, rax        ; rbx = result buffer
    mov rdi, rbx
    mov rsi, r12
    mov rcx, r14
    rep movsb
    mov rsi, r13
    mov rcx, r15
    rep movsb
    mov byte [rdi], 0
    mov rax, rbx        ; return result
    push rax            ; var result
    lea rax, [rel .STR201]
section .data
.STR201: db 0x40, 0x63, 0x6f, 0x6e, 0x63, 0x61, 0x74, 0x5b, 0x22, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x22, 0x2c, 0x20, 0x22, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x22, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L202:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L204
    cmp byte [rsi], 0
    je .L203
    inc rdx
    inc rsi
    jmp .L202
.L204:
    mov rdx, 0          ; truncate to 0 on overflow
.L203:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 8]  ; load result
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L205:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L207
    cmp byte [rsi], 0
    je .L206
    inc rdx
    inc rsi
    jmp .L205
.L207:
    mov rdx, 0          ; truncate to 0 on overflow
.L206:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    lea rax, [rel .STR208]
section .data
.STR208: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L209:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L211
    cmp byte [rsi], 0
    je .L210
    inc rdx
    inc rsi
    jmp .L209
.L211:
    mov rdx, 0          ; truncate to 0 on overflow
.L210:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @substr
    lea rax, [rel .STR212]
section .data
.STR212: db 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x00
section .text
    push rax            ; save str
    mov rax, 5
    push rax            ; save len
    inc rax             ; +1 for null
    mov rdi, rax
    call malloc         ; allocate buffer
    mov rdi, rax        ; dest
    pop rdx             ; len
    pop rsi             ; str
    mov rax, 0
    add rsi, rax        ; str + start
    mov rcx, rdx        ; count
    rep movsb           ; copy bytes
    mov byte [rdi], 0   ; null terminate
    sub rdi, rdx        ; restore buffer start
    mov rax, rdi
    push rax            ; var sub
    lea rax, [rel .STR213]
section .data
.STR213: db 0x40, 0x73, 0x75, 0x62, 0x73, 0x74, 0x72, 0x5b, 0x22, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x22, 0x2c, 0x20, 0x30, 0x2c, 0x20, 0x35, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L214:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L216
    cmp byte [rsi], 0
    je .L215
    inc rdx
    inc rsi
    jmp .L214
.L216:
    mov rdx, 0          ; truncate to 0 on overflow
.L215:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 16]  ; load sub
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L217:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L219
    cmp byte [rsi], 0
    je .L218
    inc rdx
    inc rsi
    jmp .L217
.L219:
    mov rdx, 0          ; truncate to 0 on overflow
.L218:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    lea rax, [rel .STR220]
section .data
.STR220: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L221:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L223
    cmp byte [rsi], 0
    je .L222
    inc rdx
    inc rsi
    jmp .L221
.L223:
    mov rdx, 0          ; truncate to 0 on overflow
.L222:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @substr
    lea rax, [rel .STR224]
section .data
.STR224: db 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x00
section .text
    push rax            ; save str
    mov rax, 5
    push rax            ; save len
    inc rax             ; +1 for null
    mov rdi, rax
    call malloc         ; allocate buffer
    mov rdi, rax        ; dest
    pop rdx             ; len
    pop rsi             ; str
    mov rax, 6
    add rsi, rax        ; str + start
    mov rcx, rdx        ; count
    rep movsb           ; copy bytes
    mov byte [rdi], 0   ; null terminate
    sub rdi, rdx        ; restore buffer start
    mov rax, rdi
    push rax            ; var sub2
    lea rax, [rel .STR225]
section .data
.STR225: db 0x40, 0x73, 0x75, 0x62, 0x73, 0x74, 0x72, 0x5b, 0x22, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x22, 0x2c, 0x20, 0x36, 0x2c, 0x20, 0x35, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L226:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L228
    cmp byte [rsi], 0
    je .L227
    inc rdx
    inc rsi
    jmp .L226
.L228:
    mov rdx, 0          ; truncate to 0 on overflow
.L227:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 24]  ; load sub2
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L229:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L231
    cmp byte [rsi], 0
    je .L230
    inc rdx
    inc rsi
    jmp .L229
.L231:
    mov rdx, 0          ; truncate to 0 on overflow
.L230:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    lea rax, [rel .STR232]
section .data
.STR232: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L233:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L235
    cmp byte [rsi], 0
    je .L234
    inc rdx
    inc rsi
    jmp .L233
.L235:
    mov rdx, 0          ; truncate to 0 on overflow
.L234:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @strcmp
    lea rax, [rel .STR236]
section .data
.STR236: db 0x61, 0x62, 0x63, 0x00
section .text
    mov rdi, rax        ; str1
    lea rax, [rel .STR237]
section .data
.STR237: db 0x61, 0x62, 0x63, 0x00
section .text
    mov rsi, rax        ; str2
    call strcmp
    push rax            ; var cmp1
    lea rax, [rel .STR238]
section .data
.STR238: db 0x40, 0x73, 0x74, 0x72, 0x63, 0x6d, 0x70, 0x5b, 0x22, 0x61, 0x62, 0x63, 0x22, 0x2c, 0x20, 0x22, 0x61, 0x62, 0x63, 0x22, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L239:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L241
    cmp byte [rsi], 0
    je .L240
    inc rdx
    inc rsi
    jmp .L239
.L241:
    mov rdx, 0          ; truncate to 0 on overflow
.L240:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 32]  ; load cmp1
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR242]
section .data
.STR242: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x30, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L243:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L245
    cmp byte [rsi], 0
    je .L244
    inc rdx
    inc rsi
    jmp .L243
.L245:
    mov rdx, 0          ; truncate to 0 on overflow
.L244:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @strcmp
    lea rax, [rel .STR246]
section .data
.STR246: db 0x61, 0x62, 0x63, 0x00
section .text
    mov rdi, rax        ; str1
    lea rax, [rel .STR247]
section .data
.STR247: db 0x61, 0x62, 0x64, 0x00
section .text
    mov rsi, rax        ; str2
    call strcmp
    push rax            ; var cmp2
    lea rax, [rel .STR248]
section .data
.STR248: db 0x40, 0x73, 0x74, 0x72, 0x63, 0x6d, 0x70, 0x5b, 0x22, 0x61, 0x62, 0x63, 0x22, 0x2c, 0x20, 0x22, 0x61, 0x62, 0x64, 0x22, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L249:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L251
    cmp byte [rsi], 0
    je .L250
    inc rdx
    inc rsi
    jmp .L249
.L251:
    mov rdx, 0          ; truncate to 0 on overflow
.L250:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 40]  ; load cmp2
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR252]
section .data
.STR252: db 0x20, 0x28, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74, 0x65, 0x64, 0x20, 0x6e, 0x65, 0x67, 0x61, 0x74, 0x69, 0x76, 0x65, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L253:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L255
    cmp byte [rsi], 0
    je .L254
    inc rdx
    inc rsi
    jmp .L253
.L255:
    mov rdx, 0          ; truncate to 0 on overflow
.L254:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, 0
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 40         ; pop 5 block-local variables
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

test_math_security:
    push rbp
    mov rbp, rsp
    inc qword [rel g_recursion_depth]  ; increment depth counter
    mov rax, [rel g_recursion_depth]
    cmp rax, 100000                       ; MAX_RECURSION_DEPTH
    jle .recursion_ok_256
    dec qword [rel g_recursion_depth]  ; undo increment on error
    mov rdi, 1          ; stderr
    lea rsi, [rel stack_limit_msg]
    mov rdx, 31         ; message length
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1          ; exit code
    mov rax, 60         ; sys_exit
    syscall
.recursion_ok_256:
    cmp rax, 90000         ; STACK_WARN_THRESHOLD
    jle .stack_ok_257
    mov rdi, 2          ; stderr
    lea rsi, [rel stack_warn_msg]
    mov rdx, 45         ; message length
    mov rax, 1          ; sys_write
    syscall
.stack_ok_257:
    lea rax, [rel .STR258]
section .data
.STR258: db 0x0a, 0x3d, 0x3d, 0x3d, 0x20, 0x4d, 0x61, 0x74, 0x68, 0x20, 0x26, 0x20, 0x53, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x20, 0x54, 0x65, 0x73, 0x74, 0x73, 0x20, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L259:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L261
    cmp byte [rsi], 0
    je .L260
    inc rdx
    inc rsi
    jmp .L259
.L261:
    mov rdx, 0          ; truncate to 0 on overflow
.L260:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @rand
    mov rax, 4
    mov rdi, rax        ; size
    mov rax, 318        ; sys_getrandom
    xor rsi, rsi        ; buf (use rax)
    xor rdx, rdx        ; flags
    syscall
    push rax            ; var r
    lea rax, [rel .STR262]
section .data
.STR262: db 0x40, 0x72, 0x61, 0x6e, 0x64, 0x5b, 0x34, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L263:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L265
    cmp byte [rsi], 0
    je .L264
    inc rdx
    inc rsi
    jmp .L263
.L265:
    mov rdx, 0          ; truncate to 0 on overflow
.L264:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 8]  ; load r
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR266]
section .data
.STR266: db 0x20, 0x28, 0x72, 0x61, 0x6e, 0x64, 0x6f, 0x6d, 0x20, 0x69, 0x6e, 0x74, 0x29, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L267:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L269
    cmp byte [rsi], 0
    je .L268
    inc rdx
    inc rsi
    jmp .L267
.L269:
    mov rdx, 0          ; truncate to 0 on overflow
.L268:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    ; Builtin function @hash
    mov rax, 1
    mov rcx, rax        ; algorithm
    lea rax, [rel .STR270]
section .data
.STR270: db 0x74, 0x65, 0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61, 0x00
section .text
    mov rdi, rax        ; buffer
    cmp rcx, 1
    je .hash_crc32
    xor rax, rax        ; hash=0
.hash_loop:
    movzx rbx, byte [rdi]
    test rbx, rbx
    jz .hash_done
    shl rax, 5
    add rax, rbx
    inc rdi
    jmp .hash_loop
.hash_crc32:
    mov rax, 0xFFFFFFFF ; CRC32 init value
.hash_crc32_loop:
    movzx rbx, byte [rdi]
    test rbx, rbx       ; check for null terminator
    jz .hash_crc32_done
    movzx rcx, al       ; get low byte of CRC
    xor rcx, rbx        ; XOR with data byte
    and rcx, 0xFF
    xor r8, r8          ; result = 0
    mov r9, 8           ; loop 8 times
.hash_crc32_poly:
    mov r10, rcx
    and r10, 1
    shr rcx, 1
    test r10, r10
    jz .hash_crc32_skip_poly
    xor rcx, 0xEDB88320
.hash_crc32_skip_poly:
    dec r9
    jnz .hash_crc32_poly
    shr rax, 8          ; shift CRC right
    xor rax, rcx        ; XOR with polynomial result
    and rax, 0xFFFFFFFF ; keep 32-bit
    inc rdi
    jmp .hash_crc32_loop
.hash_crc32_done:
    xor rax, 0xFFFFFFFF ; final XOR
.hash_done:
    push rax            ; var h
    lea rax, [rel .STR271]
section .data
.STR271: db 0x40, 0x68, 0x61, 0x73, 0x68, 0x5b, 0x22, 0x74, 0x65, 0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61, 0x22, 0x2c, 0x20, 0x31, 0x5d, 0x20, 0x3d, 0x20, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L272:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L274
    cmp byte [rsi], 0
    je .L273
    inc rdx
    inc rsi
    jmp .L272
.L274:
    mov rdx, 0          ; truncate to 0 on overflow
.L273:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, [rbp - 16]  ; load h
    mov rdi, rax
    call print_int_no_newline
    lea rax, [rel .STR275]
section .data
.STR275: db 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L276:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L278
    cmp byte [rsi], 0
    je .L277
    inc rdx
    inc rsi
    jmp .L276
.L278:
    mov rdx, 0          ; truncate to 0 on overflow
.L277:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, 0
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 16         ; pop 2 block-local variables
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

main:
    push rbp
    mov rbp, rsp
    inc qword [rel g_recursion_depth]  ; increment depth counter
    mov rax, [rel g_recursion_depth]
    cmp rax, 100000                       ; MAX_RECURSION_DEPTH
    jle .recursion_ok_279
    dec qword [rel g_recursion_depth]  ; undo increment on error
    mov rdi, 1          ; stderr
    lea rsi, [rel stack_limit_msg]
    mov rdx, 31         ; message length
    mov rax, 1          ; sys_write
    syscall
    mov rdi, 1          ; exit code
    mov rax, 60         ; sys_exit
    syscall
.recursion_ok_279:
    cmp rax, 90000         ; STACK_WARN_THRESHOLD
    jle .stack_ok_280
    mov rdi, 2          ; stderr
    lea rsi, [rel stack_warn_msg]
    mov rdx, 45         ; message length
    mov rax, 1          ; sys_write
    syscall
.stack_ok_280:
    lea rax, [rel .STR281]
section .data
.STR281: db 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L282:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L284
    cmp byte [rsi], 0
    je .L283
    inc rdx
    inc rsi
    jmp .L282
.L284:
    mov rdx, 0          ; truncate to 0 on overflow
.L283:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    lea rax, [rel .STR285]
section .data
.STR285: db 0x20, 0x52, 0x61, 0x73, 0x43, 0x6f, 0x64, 0x65, 0x20, 0x42, 0x75, 0x69, 0x6c, 0x74, 0x69, 0x6e, 0x73, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x53, 0x75, 0x69, 0x74, 0x65, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L286:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L288
    cmp byte [rsi], 0
    je .L287
    inc rdx
    inc rsi
    jmp .L286
.L288:
    mov rdx, 0          ; truncate to 0 on overflow
.L287:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    lea rax, [rel .STR289]
section .data
.STR289: db 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L290:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L292
    cmp byte [rsi], 0
    je .L291
    inc rdx
    inc rsi
    jmp .L290
.L292:
    mov rdx, 0          ; truncate to 0 on overflow
.L291:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    push 0              ; var result (uninitialized)
    ; Check recursion depth limit (1000 levels max)
    mov rcx, [rel g_recursion_depth]
    cmp rcx, 1000
    jge .recursion_limit_293
    mov rcx, [rel g_recursion_depth]
    inc rcx
    mov [rel g_recursion_depth], rcx
    call test_system
    mov rcx, [rel g_recursion_depth]
    dec rcx
    mov [rel g_recursion_depth], rcx
    jmp .recursion_ok_293
.recursion_limit_293:
    xor rax, rax        ; return 0 on recursion limit
.recursion_ok_293:
    mov [rbp - 8], rax  ; assign result
    mov rax, [rbp - 8]  ; load result
    push rax
    mov rax, 0
    mov rbx, rax
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    cmp rax, 0
    je .L294
    mov rax, [rbp - 8]  ; load result
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    jmp .L295
.L294:
.L295:
    ; Check recursion depth limit (1000 levels max)
    mov rcx, [rel g_recursion_depth]
    cmp rcx, 1000
    jge .recursion_limit_296
    mov rcx, [rel g_recursion_depth]
    inc rcx
    mov [rel g_recursion_depth], rcx
    call test_memory
    mov rcx, [rel g_recursion_depth]
    dec rcx
    mov [rel g_recursion_depth], rcx
    jmp .recursion_ok_296
.recursion_limit_296:
    xor rax, rax        ; return 0 on recursion limit
.recursion_ok_296:
    mov [rbp - 8], rax  ; assign result
    mov rax, [rbp - 8]  ; load result
    push rax
    mov rax, 0
    mov rbx, rax
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    cmp rax, 0
    je .L297
    mov rax, [rbp - 8]  ; load result
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    jmp .L298
.L297:
.L298:
    ; Check recursion depth limit (1000 levels max)
    mov rcx, [rel g_recursion_depth]
    cmp rcx, 1000
    jge .recursion_limit_299
    mov rcx, [rel g_recursion_depth]
    inc rcx
    mov [rel g_recursion_depth], rcx
    call test_type_conversion
    mov rcx, [rel g_recursion_depth]
    dec rcx
    mov [rel g_recursion_depth], rcx
    jmp .recursion_ok_299
.recursion_limit_299:
    xor rax, rax        ; return 0 on recursion limit
.recursion_ok_299:
    mov [rbp - 8], rax  ; assign result
    mov rax, [rbp - 8]  ; load result
    push rax
    mov rax, 0
    mov rbx, rax
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    cmp rax, 0
    je .L300
    mov rax, [rbp - 8]  ; load result
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    jmp .L301
.L300:
.L301:
    ; Check recursion depth limit (1000 levels max)
    mov rcx, [rel g_recursion_depth]
    cmp rcx, 1000
    jge .recursion_limit_302
    mov rcx, [rel g_recursion_depth]
    inc rcx
    mov [rel g_recursion_depth], rcx
    call test_strings
    mov rcx, [rel g_recursion_depth]
    dec rcx
    mov [rel g_recursion_depth], rcx
    jmp .recursion_ok_302
.recursion_limit_302:
    xor rax, rax        ; return 0 on recursion limit
.recursion_ok_302:
    mov [rbp - 8], rax  ; assign result
    mov rax, [rbp - 8]  ; load result
    push rax
    mov rax, 0
    mov rbx, rax
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    cmp rax, 0
    je .L303
    mov rax, [rbp - 8]  ; load result
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    jmp .L304
.L303:
.L304:
    ; Check recursion depth limit (1000 levels max)
    mov rcx, [rel g_recursion_depth]
    cmp rcx, 1000
    jge .recursion_limit_305
    mov rcx, [rel g_recursion_depth]
    inc rcx
    mov [rel g_recursion_depth], rcx
    call test_math_security
    mov rcx, [rel g_recursion_depth]
    dec rcx
    mov [rel g_recursion_depth], rcx
    jmp .recursion_ok_305
.recursion_limit_305:
    xor rax, rax        ; return 0 on recursion limit
.recursion_ok_305:
    mov [rbp - 8], rax  ; assign result
    mov rax, [rbp - 8]  ; load result
    push rax
    mov rax, 0
    mov rbx, rax
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    cmp rax, 0
    je .L306
    mov rax, [rbp - 8]  ; load result
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    jmp .L307
.L306:
.L307:
    lea rax, [rel .STR308]
section .data
.STR308: db 0x0a, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L309:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L311
    cmp byte [rsi], 0
    je .L310
    inc rdx
    inc rsi
    jmp .L309
.L311:
    mov rdx, 0          ; truncate to 0 on overflow
.L310:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    lea rax, [rel .STR312]
section .data
.STR312: db 0x20, 0x41, 0x6c, 0x6c, 0x20, 0x62, 0x75, 0x69, 0x6c, 0x74, 0x69, 0x6e, 0x20, 0x74, 0x65, 0x73, 0x74, 0x73, 0x20, 0x63, 0x6f, 0x6d, 0x70, 0x6c, 0x65, 0x74, 0x65, 0x64, 0x21, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L313:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L315
    cmp byte [rsi], 0
    je .L314
    inc rdx
    inc rsi
    jmp .L313
.L315:
    mov rdx, 0          ; truncate to 0 on overflow
.L314:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    lea rax, [rel .STR316]
section .data
.STR316: db 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x0a, 0x00
section .text
    mov rdi, rax        ; string address
    mov rsi, rax
    xor rdx, rdx        ; length counter
.L317:
    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check
    jge .L319
    cmp byte [rsi], 0
    je .L318
    inc rdx
    inc rsi
    jmp .L317
.L319:
    mov rdx, 0          ; truncate to 0 on overflow
.L318:
    mov rsi, rdi        ; restore string address
    mov rdi, 1          ; stdout
    mov rax, 1          ; sys_write
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, $newline
    mov rdx, 1
    syscall
    mov rax, 0
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 8         ; pop 1 block-local variables
    dec qword [rel g_recursion_depth]  ; decrement depth counter
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

_start:
    call main
    mov rdi, rax           ; exit code
    mov rax, 60            ; sys_exit
    syscall
