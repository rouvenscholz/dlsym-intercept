.section absolute
elf_start:


.text
.global get_elf_header
.hidden get_elf_header
get_elf_header:
    lea     elf_start(%rip), %rax
    ret
