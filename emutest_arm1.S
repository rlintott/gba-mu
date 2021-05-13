@ vim:ft=armv4
@ expected result: 0x100 = 5, 0x104 = 5, 0x108 = 5

mov r1, #2
mov r1, #2
mov r2, #3
adds r3, r2, r1
mov r4, #0
movne r4, r3
mov r6, #0x100

str r3, [r6, #0]
str r4, [r6, #4]

ldr r5, [r6, r0, LSL#2]
str r5, [r6, #8]
