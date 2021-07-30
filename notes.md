
### Interrupt flow

- check if if there are interrupts that are both requested and enabled (check ie & if)
- check IME reg && cpsr.I (interrupts enabled)
- if interrupts enabled, switch CPU mode to IRQ
- count cycles for updating PC to 0x18
