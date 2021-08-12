
### Interrupt flow

- check if if there are interrupts that are both requested and enabled (check ie & if)
- check IME reg && cpsr.I (interrupts enabled)
- if interrupts enabled, switch CPU mode to IRQ
- count cycles for updating PC to 0x18


### PPU flow

```
for each scanline:
    for each dirty object:
        update transformation on object for all y-coords >= current scanline
        mark object as clean
```
#### to render (do this once per frame):

```
for each object from lowest to highest priority
    display object pixels on screen
```

- render the each scanline at the end of the 1232 cycles one scan takes

### objects

keep sorted onjs v 