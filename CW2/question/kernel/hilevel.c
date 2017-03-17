#include "hilevel.h"

pcb_t pcb[ 2 ], *current = NULL;

void scheduler( ctx_t* ctx ) {
  if      ( current == &pcb[ 0 ] ) {
    memcpy( &pcb[ 1 ].ctx, ctx, sizeof( ctx_t ) ); // preserve P_4
    memcpy( &pcb[ 2 ].ctx, ctx, sizeof( ctx_t ) ); // preserve P_5
    memcpy( ctx, &pcb[ 0 ].ctx, sizeof( ctx_t ) ); // restore  P_3
    current = &pcb[ 0 ];
  }
  else if ( current == &pcb[ 1 ] ) {
    memcpy( &pcb[ 0 ].ctx, ctx, sizeof( ctx_t ) ); // preserve P_3
    memcpy( &pcb[ 2 ].ctx, ctx, sizeof( ctx_t ) ); // preserve P_5
    memcpy( ctx, &pcb[ 1 ].ctx, sizeof( ctx_t ) ); // restore  P_4
    current = &pcb[ 1 ];
  }
  else if ( current == &pcb[ 2 ] ) {
    memcpy( &pcb[ 0 ].ctx, ctx, sizeof( ctx_t ) ); // preserve P_3
    memcpy( &pcb[ 1 ].ctx, ctx, sizeof( ctx_t ) ); // preserve P_4
    memcpy( ctx, &pcb[ 2 ].ctx, sizeof( ctx_t ) ); // restore  P_5
    current = &pcb[ 2 ];
  }

  return;
}

extern void     main_P3();
extern uint32_t tos_P3;
extern void     main_P4();
extern uint32_t tos_P4;
extern void     main_P5();
extern uint32_t tos_P5;


void hilevel_handler_rst( ctx_t* ctx              ) {
  /* Configure the mechanism for interrupt handling by
   *
   * - configuring timer st. it raises a (periodic) interrupt for each
   *   timer tick,
   * - configuring GIC st. the selected interrupts are forwarded to the
   *   processor via the IRQ interrupt signal, then
   * - enabling IRQ interrupts.
   */

   memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );
   pcb[ 0 ].pid      = 1;
   pcb[ 0 ].ctx.cpsr = 0x50;
   pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_P3 );
//   pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_P3  );

   memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );
   pcb[ 1 ].pid      = 2;
   pcb[ 1 ].ctx.cpsr = 0x50;
   pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
//   pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P4  );

   memset( &pcb[ 2 ], 0, sizeof( pcb_t ) );
   pcb[ 2 ].pid      = 3;
   pcb[ 2 ].ctx.cpsr = 0x50;
   pcb[ 2 ].ctx.pc   = ( uint32_t )( &main_P5 );
//   pcb[ 2 ].ctx.sp   = ( uint32_t )( &tos_P5  );

   current = &pcb[ 0 ]; memcpy( ctx, &current->ctx, sizeof( ctx_t ) );

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  int_enable_irq();

  return;
}

void hilevel_handler_irq() {
  // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_TIMER0 ) {
    PL011_putc( UART0, 'T', true ); TIMER0->Timer1IntClr = 0x01;
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  /* Based on the identified encoded as an immediate operand in the
   * instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call,
   * - write any return value back to preserved usr mode registers.
   */

switch( id ) {
    case 0x00 : { // 0x00 => yield()
      //scheduler( ctx );
      break;
    }
    case 0x01 : { // 0x01 => write( fd, x, n ) copy this block into CW2
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;
      break;
    }
    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
