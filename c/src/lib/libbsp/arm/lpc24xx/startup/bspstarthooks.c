/**
 * @file
 *
 * @ingroup lpc24xx
 *
 * @brief Startup code.
 */

/*
 * Copyright (c) 2008-2011 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.com/license/LICENSE.
 */

#include <stdbool.h>

#include <rtems/score/armv7m.h>

#include <bspopts.h>
#include <bsp/io.h>
#include <bsp/start.h>
#include <bsp/lpc24xx.h>
#include <bsp/lpc-emc.h>
#include <bsp/start-config.h>

static BSP_START_TEXT_SECTION void lpc24xx_cpu_delay(unsigned ticks)
{
  unsigned i = 0;

  /* One loop execution needs four instructions */
  ticks /= 4;

  for (i = 0; i <= ticks; ++i) {
    __asm__ volatile ("nop");
  }
}

static BSP_START_TEXT_SECTION void lpc24xx_udelay(unsigned us)
{
  lpc24xx_cpu_delay(us * (LPC24XX_CCLK / 1000000));
}

static BSP_START_TEXT_SECTION void lpc24xx_init_pinsel(void)
{
  lpc24xx_pin_config(
    &lpc24xx_start_config_pinsel [0],
    LPC24XX_PIN_SET_FUNCTION
  );
}

static BSP_START_TEXT_SECTION void lpc24xx_init_emc_static(void)
{
  size_t i = 0;
  size_t chip_count = lpc24xx_start_config_emc_static_chip_count;

  for (i = 0; i < chip_count; ++i) {
    const lpc24xx_emc_static_chip_config *chip_config =
      &lpc24xx_start_config_emc_static_chip [i];
    lpc24xx_emc_static_chip_config chip_config_on_stack;
    size_t config_size = sizeof(chip_config_on_stack.config);

    bsp_start_memcpy(
      (int *) &chip_config_on_stack.config,
      (const int *) &chip_config->config,
      config_size
    );
    bsp_start_memcpy(
      (int *) chip_config->chip_select,
      (const int *) &chip_config_on_stack.config,
      config_size
    );
  }
}

static BSP_START_TEXT_SECTION void lpc24xx_init_emc_dynamic(void)
{
  size_t chip_count = lpc24xx_start_config_emc_dynamic_chip_count;

  if (chip_count > 0) {
    bool do_initialization = true;
    size_t i = 0;

    for (i = 0; do_initialization && i < chip_count; ++i) {
      const lpc24xx_emc_dynamic_chip_config *chip_cfg =
        &lpc24xx_start_config_emc_dynamic_chip [i];
      volatile lpc_emc_dynamic *chip_select = chip_cfg->chip_select;

      do_initialization = (chip_select->config & EMC_DYN_CFG_B) == 0;
    }

    if (do_initialization) {
      volatile lpc_emc *emc = (volatile lpc_emc *) EMC_BASE_ADDR;
      const lpc24xx_emc_dynamic_config *cfg =
        &lpc24xx_start_config_emc_dynamic [0];
      uint32_t dynamiccontrol = EMC_DYN_CTRL_CE | EMC_DYN_CTRL_CS;

      emc->dynamicreadconfig = cfg->readconfig;

      /* Timings */
      emc->dynamictrp = cfg->trp;
      emc->dynamictras = cfg->tras;
      emc->dynamictsrex = cfg->tsrex;
      emc->dynamictapr = cfg->tapr;
      emc->dynamictdal = cfg->tdal;
      emc->dynamictwr = cfg->twr;
      emc->dynamictrc = cfg->trc;
      emc->dynamictrfc = cfg->trfc;
      emc->dynamictxsr = cfg->txsr;
      emc->dynamictrrd = cfg->trrd;
      emc->dynamictmrd = cfg->tmrd;

      /* NOP period */
      emc->dynamiccontrol = dynamiccontrol | EMC_DYN_CTRL_I_NOP;
      lpc24xx_udelay(200);

      /* Precharge */
      emc->dynamiccontrol = dynamiccontrol | EMC_DYN_CTRL_I_PALL;
      emc->dynamicrefresh = 1;

      /*
       * Perform several refresh cycles with a memory refresh every 16 AHB
       * clock cycles.  Wait until eight SDRAM refresh cycles have occurred
       * (128 AHB clock cycles).
       */
      lpc24xx_cpu_delay(128);

      /* Refresh timing */
      emc->dynamicrefresh = cfg->refresh;
      lpc24xx_cpu_delay(128);

      for (i = 0; i < chip_count; ++i) {
        const lpc24xx_emc_dynamic_chip_config *chip_cfg =
          &lpc24xx_start_config_emc_dynamic_chip [i];
        volatile lpc_emc_dynamic *chip_select = chip_cfg->chip_select;
        uint32_t config = chip_cfg->config;

        /* Chip select */
        chip_select->config = config;
        chip_select->rascas = chip_cfg->rascas;

        /* Set modes */
        emc->dynamiccontrol = dynamiccontrol | EMC_DYN_CTRL_I_MODE;
        *(volatile uint32_t *)(chip_cfg->address + chip_cfg->mode);

        /* Enable buffer */
        chip_select->config = config | EMC_DYN_CFG_B;
      }

      emc->dynamiccontrol = 0;
    }
  }
}

static BSP_START_TEXT_SECTION void lpc24xx_init_main_oscillator(void)
{
  #ifdef ARM_MULTILIB_ARCH_V4
    if ((SCS & 0x40) == 0) {
      SCS |= 0x20;
      while ((SCS & 0x40) == 0) {
        /* Wait */
      }
    }
  #endif
}

#ifdef ARM_MULTILIB_ARCH_V4

static BSP_START_TEXT_SECTION void lpc24xx_pll_config(
  uint32_t val
)
{
  PLLCON = val;
  PLLFEED = 0xaa;
  PLLFEED = 0x55;
}

/**
 * @brief Sets the Phase Locked Loop (PLL).
 *
 * All parameter values are the actual register field values.
 *
 * @param clksrc Selects the clock source for the PLL.
 *
 * @param nsel Selects PLL pre-divider value (sometimes named psel).
 *
 * @param msel Selects PLL multiplier value.
 *
 * @param cclksel Selects the divide value for creating the CPU clock (CCLK)
 * from the PLL output.
 */
static BSP_START_TEXT_SECTION void lpc24xx_set_pll(
  unsigned clksrc,
  unsigned nsel,
  unsigned msel,
  unsigned cclksel
)
{
  uint32_t pllstat = PLLSTAT;
  uint32_t pllcfg = SET_PLLCFG_NSEL(0, nsel) | SET_PLLCFG_MSEL(0, msel);
  uint32_t clksrcsel = SET_CLKSRCSEL_CLKSRC(0, clksrc);
  uint32_t cclkcfg = SET_CCLKCFG_CCLKSEL(0, cclksel | 1);
  bool pll_enabled = (pllstat & PLLSTAT_PLLE) != 0;

  /* Disconnect PLL if necessary */
  if ((pllstat & PLLSTAT_PLLC) != 0) {
    if (pll_enabled) {
      /* Check if we run already with the desired settings */
      if (PLLCFG == pllcfg && CLKSRCSEL == clksrcsel && CCLKCFG == cclkcfg) {
        /* Nothing to do */
        return;
      }
      lpc24xx_pll_config(PLLCON_PLLE);
    } else {
      lpc24xx_pll_config(0);
    }
  }

  /* Set CPU clock divider to a reasonable save value */
  CCLKCFG = 0;

  /* Disable PLL if necessary */
  if (pll_enabled) {
    lpc24xx_pll_config(0);
  }

  /* Select clock source */
  CLKSRCSEL = clksrcsel;

  /* Set PLL Configuration Register */
  PLLCFG = pllcfg;

  /* Enable PLL */
  lpc24xx_pll_config(PLLCON_PLLE);

  /* Wait for lock */
  while ((PLLSTAT & PLLSTAT_PLOCK) == 0) {
    /* Wait */
  }

  /* Set CPU clock divider and ensure that we have an odd value */
  CCLKCFG = cclkcfg;

  /* Connect PLL */
  lpc24xx_pll_config(PLLCON_PLLE | PLLCON_PLLC);
}

#endif /* ARM_MULTILIB_ARCH_V4 */

static BSP_START_TEXT_SECTION void lpc24xx_init_pll(void)
{
  #ifdef ARM_MULTILIB_ARCH_V4
    #if LPC24XX_OSCILLATOR_MAIN == 12000000U
      #if LPC24XX_CCLK == 72000000U
        lpc24xx_set_pll(1, 0, 11, 3);
      #elif LPC24XX_CCLK == 51612800U
        lpc24xx_set_pll(1, 30, 399, 5);
      #else
        #error "unexpected CCLK"
      #endif
    #elif LPC24XX_OSCILLATOR_MAIN == 3686400U
      #if LPC24XX_CCLK == 58982400U
        lpc24xx_set_pll(1, 0, 47, 5);
      #else
        #error "unexpected CCLK"
      #endif
    #else
      #error "unexpected main oscillator frequency"
    #endif
  #endif
}

static BSP_START_TEXT_SECTION void lpc24xx_init_memory_map(void)
{
  #ifdef ARM_MULTILIB_ARCH_V4
    /* Re-map interrupt vectors to internal RAM */
    MEMMAP = SET_MEMMAP_MAP(MEMMAP, 2);
  #endif

  /* Use normal memory map */
  EMC_CTRL &= ~0x2U;
}

static BSP_START_TEXT_SECTION void lpc24xx_init_memory_accelerator(void)
{
  #ifdef ARM_MULTILIB_ARCH_V4
    /* Fully enable memory accelerator module functions (MAM) */
    MAMCR = 0;
    #if LPC24XX_CCLK <= 20000000U
      MAMTIM = 0x1;
    #elif LPC24XX_CCLK <= 40000000U
      MAMTIM = 0x2;
    #elif LPC24XX_CCLK <= 60000000U
      MAMTIM = 0x3;
    #else
      MAMTIM = 0x4;
    #endif
    MAMCR = 0x2;

    /* Enable fast IO for ports 0 and 1 */
    SCS |= 0x1;
  #endif
}

static BSP_START_TEXT_SECTION void lpc24xx_stop_gpdma(void)
{
  #ifdef LPC24XX_STOP_GPDMA
    #ifdef ARM_MULTILIB_ARCH_V4
      bool has_power = (PCONP & PCONP_GPDMA) != 0;
    #endif

    if (has_power) {
      GPDMA_CONFIG = 0;

      #ifdef ARM_MULTILIB_ARCH_V4
        PCONP &= ~PCONP_GPDMA;
      #endif
    }
  #endif
}

static BSP_START_TEXT_SECTION void lpc24xx_stop_ethernet(void)
{
  #ifdef LPC24XX_STOP_ETHERNET
    #ifdef ARM_MULTILIB_ARCH_V4
      bool has_power = (PCONP & PCONP_ETHERNET) != 0;
    #endif

    if (has_power) {
      MAC_COMMAND = 0x38;
      MAC_MAC1 = 0xcf00;
      MAC_MAC1 = 0;

      #ifdef ARM_MULTILIB_ARCH_V4
        PCONP &= ~PCONP_ETHERNET;
      #endif
    }
  #endif
}

static BSP_START_TEXT_SECTION void lpc24xx_stop_usb(void)
{
  #ifdef LPC24XX_STOP_USB
    #ifdef ARM_MULTILIB_ARCH_V4
      bool has_power = (PCONP & PCONP_USB) != 0;
    #endif

    if (has_power) {
      OTG_CLK_CTRL = 0;

      #ifdef ARM_MULTILIB_ARCH_V4
        PCONP &= ~PCONP_USB;
      #endif
    }
  #endif
}

BSP_START_TEXT_SECTION void bsp_start_hook_0(void)
{
  lpc24xx_init_main_oscillator();
  lpc24xx_init_pll();
  lpc24xx_init_pinsel();
  lpc24xx_init_emc_static();
}

BSP_START_TEXT_SECTION void bsp_start_hook_1(void)
{
  lpc24xx_init_memory_map();
  lpc24xx_init_memory_accelerator();
  lpc24xx_init_emc_dynamic();
  lpc24xx_stop_gpdma();
  lpc24xx_stop_ethernet();
  lpc24xx_stop_usb();
  bsp_start_copy_sections();

  /* At this point we can use objects outside the .start section */
}
