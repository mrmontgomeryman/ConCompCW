#include "hilevel.h"

void hilevel_handler_rst() {
  char* x = "hello world\n";

  while( 1 ) {
    for( int i = 0; i < 12; i++ ) {
      PL011_putc( UART0, x[ i ], true );
    }
  }

  return;
}
