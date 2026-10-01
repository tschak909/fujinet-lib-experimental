#include "fujinet-bus-nes.h"
#include "fujinet-nes.h"

bool fuji_nes_present(void)
{
  return FN_MAGIC0 == 'F' && FN_MAGIC1 == 'N' && FN_PROTOVER == FN_PROTO_VER;
}

uint8_t fuji_nes_boot_state(void)
{
  return FN_BOOTSTAT;
}

uint8_t fuji_nes_boot_percent(void)
{
  return FN_BOOTPCT;
}

uint8_t fuji_nes_boot_error(void)
{
  return FN_BOOTERR;
}

/*
  Hand the console to the loader ROM at $5800. It is cartridge-served and
  untouched by the SRAM copy it performs, so nothing has to be moved into
  console RAM first: arm the load, put the PPU and APU to sleep, and jump.
  The loader copies the staged image into the SRAMs and cold-starts it
  through its own reset vector. Does not return.
*/
void fuji_nes_boot(void)
{
  fn_regwr(FNR_BOOTLOCK, FN_BOOTLOCK_MAGIC);
  __asm__("sei");
  *(volatile uint8_t *) 0x2000 = 0;     /* NMI off */
  *(volatile uint8_t *) 0x2001 = 0;     /* rendering off */
  *(volatile uint8_t *) 0x4015 = 0;     /* APU channels off */
  ((void (*)(void)) FN_LOADER)();
  for (;;)
    ;
}
