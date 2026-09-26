.section absolute
elf_start:


.text
.global get_elf_header
.hidden get_elf_header
get_elf_header:
    auipc   a0, %pcrel_hi(elf_start)
    addi    a0, a0, %pcrel_lo(get_elf_header)
    ret
