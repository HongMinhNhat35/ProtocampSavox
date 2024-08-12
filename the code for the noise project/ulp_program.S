.bss
.global microphone_data
microphone_data:
    .long 0

.global data_index
data_index:
    .long 0

.text
.global entry_start
entry_start:
    movi r2, 1000
    movi r3, 0

collect_data:
    movi r0, 1234
    movi r1, 0x1000
    st r0, r1, r3

    addi r3, r3, 4
    sub r2, r2, 1
    bnez r2, collect_data

    halt
