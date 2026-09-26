

.macro gen_symbol symbol_name
.global \symbol_name
.hidden \symbol_name
\symbol_name:
.endm



.section .rodata
gen_symbol jumptable_entry_template_begin
jumptable_entry_call_destination:
.quad 0
jumptable_entry_call_count:
.long 0
jumptable_entry_call_code:
    auipc   t0, 0
.equ offset, jumptable_entry_call_count-jumptable_entry_call_code
    lw      t1, offset(t0)
    addi    t1, t1, 1
    sw      t1, offset(t0)
.equ offset, jumptable_entry_call_destination-jumptable_entry_call_code
    ld      t2, offset(t0)
    jr      t2
gen_symbol jumptable_entry_template_end



.text

# void jumptable_entry_init(void* jumptable_entry, void* destination);
gen_symbol jumptable_entry_init
.equ offset, jumptable_entry_call_destination-jumptable_entry_template_begin
    sd      a1, offset(a0)
    ret


#void* jumptable_entry_get_code_addr(void* jumptable_entry);
gen_symbol jumptable_entry_get_code_addr
.equ offset, jumptable_entry_call_code-jumptable_entry_template_begin
     addi    a0, a0, offset
     ret



# uint32_t jumptable_entry_get_count(void* jumptable_entry)
gen_symbol jumptable_entry_get_count
.equ offset, jumptable_entry_call_count-jumptable_entry_template_begin
    lw      a0, offset(a0)
    ret
