.global lolevel_handler_rst

lolevel_handler_rst: msr   cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
                     ldr   sp, =tos_svc            @ initialise SVC mode stack

                     bl    hilevel_handler_rst     @ invoke high-level C function
                     b     .                       @ halt
