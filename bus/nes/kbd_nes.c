/*
  The Famicom expansion-port keyboards: Nintendo's Family BASIC Keyboard
  (HVC-007) and the Subor keyboard. Both are scanned the same way -- a write
  to $4016 picks the row (bit 0 resets to row 0 on the Family BASIC; moving
  the column from 1 to 0 advances the row), bit 1 picks which half of the
  row, bit 2 enables the keyboard -- and $4017 bits 1-4 return that half's
  four keys, active low. They differ in size (9 rows and 13) and in what an
  idle port reads: a disabled Subor still drives 0x1E, a disabled Family
  BASIC keyboard reads 0, which is how the two are told apart.

  Plain stores only, as everywhere in this target.

  The character tables follow the matrices MesenCE emulates
  (Core/NES/Input/FamilyBasicKeyboard.h, SuborKeyboard.h). Letters are
  lower case, upper case with Shift (the Family BASIC keyboard itself has
  no lower case; a FujiNet client wants both for passwords). The Family
  BASIC keyboard's shifted digits and symbols follow its key caps; the
  Subor's follow a US PC keyboard. The tables are in matrix order (row * 8
  + column * 4 + bit); keep it.
*/

#include <string.h>

#include "fujinet-nes.h"

#define KBD_OUT (*(volatile uint8_t *) 0x4016)
#define KBD_IN  (*(volatile uint8_t *) 0x4017)

static const char fb_plain[72] = {
    0, FUJI_NES_KEY_ENTER, '[', ']', 0, 0, '\\', 0,
    0, '@', ':', ';', '_', '/', '-', '^',
    0, 'o', 'l', 'k', '.', ',', 'p', '0',
    0, 'i', 'u', 'j', 'm', 'n', '9', '8',
    0, 'y', 'g', 'h', 'b', 'v', '7', '6',
    0, 't', 'r', 'd', 'f', 'c', '5', '4',
    0, 'w', 's', 'a', 'x', 'z', 'e', '3',
    0, FUJI_NES_KEY_ESC, 'q', 0, 0, 0, '1', '2',
    FUJI_NES_KEY_HOME, FUJI_NES_KEY_UP, FUJI_NES_KEY_RIGHT, FUJI_NES_KEY_LEFT, FUJI_NES_KEY_DOWN, ' ', FUJI_NES_KEY_BS, 0,
};

static const char fb_shift[72] = {
    0, FUJI_NES_KEY_ENTER, '{', '}', 0, 0, '|', 0,
    0, '`', '*', '+', '_', '?', '=', '~',
    0, 'O', 'L', 'K', '>', '<', 'P', '0',
    0, 'I', 'U', 'J', 'M', 'N', ')', '(',
    0, 'Y', 'G', 'H', 'B', 'V', '\'', '&',
    0, 'T', 'R', 'D', 'F', 'C', '%', '$',
    0, 'W', 'S', 'A', 'X', 'Z', 'E', '#',
    0, FUJI_NES_KEY_ESC, 'Q', 0, 0, 0, '!', '"',
    FUJI_NES_KEY_HOME, FUJI_NES_KEY_UP, FUJI_NES_KEY_RIGHT, FUJI_NES_KEY_LEFT, FUJI_NES_KEY_DOWN, ' ', FUJI_NES_KEY_BS, 0,
};

static const char sb_plain[104] = {
    '4', 'g', 'f', 'c', 0, 'e', '5', 'v',
    '2', 'd', 's', 0, 0, 'w', '3', 'x',
    0, FUJI_NES_KEY_BS, 0, FUJI_NES_KEY_RIGHT, 0, 0, 0x7F, FUJI_NES_KEY_HOME,
    '9', 'i', 'l', ',', 0, 'o', '0', '.',
    ']', FUJI_NES_KEY_ENTER, FUJI_NES_KEY_UP, FUJI_NES_KEY_LEFT, 0, '[', '\\', FUJI_NES_KEY_DOWN,
    'q', 0, 'z', FUJI_NES_KEY_TAB, FUJI_NES_KEY_ESC, 'a', '1', 0,
    '7', 'y', 'k', 'm', 0, 'u', '8', 'j',
    '-', ';', '\'', '/', 0, 'p', '=', 0,
    't', 'h', 'n', ' ', 0, 'r', '6', 'b',
    '6', FUJI_NES_KEY_ENTER, '4', '8', 0, 0, 0, 0,
    0, '4', '7', 0, 0, '1', '2', '8',
    '-', '+', '*', '9', 0, '5', '/', 0,
    '`', '6', 0, ' ', 0, '3', '.', '0',
};

static const char sb_shift[104] = {
    '$', 'G', 'F', 'C', 0, 'E', '%', 'V',
    '@', 'D', 'S', 0, 0, 'W', '#', 'X',
    0, FUJI_NES_KEY_BS, 0, FUJI_NES_KEY_RIGHT, 0, 0, 0x7F, FUJI_NES_KEY_HOME,
    '(', 'I', 'L', '<', 0, 'O', ')', '>',
    '}', FUJI_NES_KEY_ENTER, FUJI_NES_KEY_UP, FUJI_NES_KEY_LEFT, 0, '{', '|', FUJI_NES_KEY_DOWN,
    'Q', 0, 'Z', FUJI_NES_KEY_TAB, FUJI_NES_KEY_ESC, 'A', '!', 0,
    '&', 'Y', 'K', 'M', 0, 'U', '*', 'J',
    '_', ':', '"', '?', 0, 'P', '+', 0,
    'T', 'H', 'N', ' ', 0, 'R', '^', 'B',
    '6', FUJI_NES_KEY_ENTER, '4', '8', 0, 0, 0, 0,
    0, '4', '7', 0, 0, '1', '2', '8',
    '-', '+', '*', '9', 0, '5', '/', 0,
    '~', '6', 0, ' ', 0, '3', '.', '0',
};
/* Matrix positions (row * 8 + column * 4 + bit) with a job of their own. */
#define FB_RSHIFT   5
#define FB_LSHIFT   60
#define SB_SHIFT    63
#define SB_CAPS     41
#define SB_NONE     76      /* a placeholder in MesenCE's Subor matrix */

static uint8_t kbd_type;
static uint8_t caps;
static uint8_t prev[26];

uint8_t fuji_nes_kbd_detect(void)
{
  uint8_t v;

  KBD_OUT = 0;
  v = KBD_IN & 0x1E;
  if (v == 0x1E) {
    kbd_type = FUJI_NES_KBD_SUBOR;
  } else {
    KBD_OUT = 5;                /* row 0, column 0, enabled */
    KBD_OUT = 4;
    v = KBD_IN & 0x1E;
    KBD_OUT = 0;
    /* an idle Family BASIC keyboard reads all four keys up */
    kbd_type = v ? FUJI_NES_KBD_FAMILY_BASIC : FUJI_NES_KBD_NONE;
  }
  memset(prev, 0, sizeof prev);
  caps = 0;
  return kbd_type;
}

uint8_t fuji_nes_kbd_type(void)
{
  return kbd_type;
}

void fuji_nes_kbd_scan(uint8_t rows[26])
{
  uint8_t r, n, a, b;

  n = (kbd_type == FUJI_NES_KBD_SUBOR) ? 13 : 9;
  if (kbd_type == FUJI_NES_KBD_FAMILY_BASIC)
    KBD_OUT = 5;                /* back to row 0 */
  for (r = 0; r < n; ++r) {
    KBD_OUT = 4;
    a = KBD_IN;
    KBD_OUT = 6;
    b = KBD_IN;
    rows[r * 2] = (uint8_t)((~a >> 1) & 0x0F);
    rows[r * 2 + 1] = (uint8_t)((~b >> 1) & 0x0F);
  }
  /* The Subor has no row reset: one more column change while enabled
     brings it round from row 12 to row 0 for the next scan. */
  if (kbd_type == FUJI_NES_KBD_SUBOR)
    KBD_OUT = 4;
  KBD_OUT = 0;
  for (r = n * 2; r < 26; ++r)
    rows[r] = 0;
}

char fuji_nes_kbd_getc(void)
{
  uint8_t rows[26];
  uint8_t i, bit, newly, n, p, shift;
  const char *plain, *shifted;
  char c = 0;

  if (kbd_type == FUJI_NES_KBD_NONE)
    return 0;
  fuji_nes_kbd_scan(rows);
  if (kbd_type == FUJI_NES_KBD_SUBOR) {
    n = 26;
    plain = sb_plain;
    shifted = sb_shift;
    shift = (uint8_t)(rows[SB_SHIFT >> 2] & (1 << (SB_SHIFT & 3)));
  } else {
    n = 18;
    plain = fb_plain;
    shifted = fb_shift;
    shift = (uint8_t)((rows[FB_RSHIFT >> 2] & (1 << (FB_RSHIFT & 3)))
                    | (rows[FB_LSHIFT >> 2] & (1 << (FB_LSHIFT & 3))));
  }

  for (i = 0; i < n; ++i) {
    newly = (uint8_t)(rows[i] & ~prev[i]);
    prev[i] = rows[i];
    if (c || !newly)
      continue;
    for (bit = 0; bit < 4; ++bit) {
      if (!(newly & (1 << bit)))
        continue;
      p = (uint8_t)(i * 4 + bit);
      if (kbd_type == FUJI_NES_KBD_SUBOR) {
        if (p == SB_NONE)
          continue;
        if (p == SB_CAPS) {
          caps = (uint8_t)!caps;
          continue;
        }
      }
      c = shift ? shifted[p] : plain[p];
      if (caps && c >= 'a' && c <= 'z')
        c = (char)(c - 0x20);
      else if (caps && c >= 'A' && c <= 'Z')
        c = (char)(c + 0x20);
      if (c)
        break;
    }
  }
  return c;
}
