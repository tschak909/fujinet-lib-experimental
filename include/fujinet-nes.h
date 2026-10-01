#ifndef FUJINET_NES_H
#define FUJINET_NES_H

/*
  NES-only facilities that the cross-platform API has no place for.

  The vblank NMI is harmless to a transaction in flight as long as the handler
  never WRITES to $5500-$57FF (reads anywhere are inert): a register write is
  one store and the TX stream is append-only, so an interrupted transaction
  simply resumes. cc65's own conio handler only touches the PPU and RAM.

  Building through makefiles/platforms/nes.mk links against the FujiNet
  linker config, which keeps $FFF0-$FFF9 clear for the "FUJI" claim that
  nes-romstamp.py writes: without it the cartridge shuts the mailbox down for
  the session the moment the image boots.
*/

#include <fujinet-int.h>

/* fuji_nes_boot_state() values. */
#define FUJI_NES_BOOT_IDLE   0
#define FUJI_NES_BOOT_XFER   1
#define FUJI_NES_BOOT_READY  2
#define FUJI_NES_BOOT_FAILED 0x80

/*
  Is a FujiNet cartridge actually underneath us, and does it speak a protocol
  version we understand? On a plain game cartridge these addresses are open
  bus.
*/
extern bool fuji_nes_present(void);

/*
  fuji_mount_disk_image() only starts the transfer. The image itself arrives
  asynchronously, pushed to the cartridge while the console keeps running, so
  poll fuji_nes_boot_state() until it reads READY (or FAILED, in which case
  fuji_nes_boot_error() says why). fuji_nes_boot_percent() runs 0-100 and is
  there to drive a progress bar.
*/
extern uint8_t fuji_nes_boot_state(void);
extern uint8_t fuji_nes_boot_percent(void);
extern uint8_t fuji_nes_boot_error(void);

/*
  Boot the image that was just pushed: arm the load and jump into the
  cartridge's loader ROM, which copies it into the SRAMs and cold-starts it.
  Does not return. Only meaningful once fuji_nes_boot_state() reads READY.
*/
extern void fuji_nes_boot(void);

#endif /* FUJINET_NES_H */
