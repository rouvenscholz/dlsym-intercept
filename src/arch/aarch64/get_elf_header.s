.section absolute
elf_start:


.text
.global get_elf_header
.hidden get_elf_header
get_elf_header:
    adrp x0, elf_start
    add  x0, x0, :lo12:elf_start
    ret
