/*
 * Copyright (c) 2016 Jean-Francois Moine <moinejf@free.fr>
 * Adapted from the sun8i SDK
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk-provider.h>
#include <linux/reset-controller.h>

#include <dt-bindings/clock/sun8i-a83t.h>
#include <dt-bindings/reset/sun8i-a83t.h>

#include "ccu.h"

static const char losc[] __initconst = "osc32k";
static const char hosc[] __initconst = "osc24M";
static const char iosc[] __initconst = "osc16M";
static const char * const child_hosc[] __initconst = { hosc };

/* ---- CCU ---- */

/* 2 * cpux */
/*	rate = 24MHz * n / p */
static const char pll_c0cpux[] __initconst = "pll-c0cpux";
static const struct clk_init_data pll_c0cpux_init __initconst = {
	CCU_HW(pll_c0cpux, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_c0cpux_clk __initconst = {
	CCU_REG(0x000),
	.hw.init = &pll_c0cpux_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 0),
	CCU_N(8, 8), .n_min = 12,
	CCU_P(16, 1),			/* only when rate < 288MHz */
	.features = CCU_FEATURE_N0,
};

static const char pll_c1cpux[] __initconst = "pll-c1cpux";
static const struct clk_init_data pll_c1cpux_init __initconst = {
	CCU_HW(pll_c1cpux, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_c1cpux_clk __initconst = {
	CCU_REG(0x004),
	.hw.init = &pll_c1cpux_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 1),
	CCU_N(8, 8), .n_min = 12,
	CCU_P(16, 1),			/* only when rate < 288MHz */
	.features = CCU_FEATURE_N0,
};

/* audio */
/*	rate = 22579200Hz or 24576000Hz with sigma-delta */
/*   or	rate = 24MHz * n / (d1 + 1) / (d2 + 1) / (p + 1) */
static const char pll_audio[] __initconst = "pll-audio";
#define NDP_MASK (GENMASK(15, 8) | BIT(16) | BIT(18) | GENMASK(5, 0))
#define SDM_BIT BIT(24)
static const struct frac audio_fracs[] = {
	{
		.rate = 22579200,
		.mask =  SDM_BIT | NDP_MASK,
		.val = SDM_BIT | (54 << 8) | BIT(18) | (28 << 0),
						/* n=54 d1=0 d2=1 p=28 */
		.sd_reg = 0x284,
		.sd_val = 0xc00121ff,
	},
	{
		.rate = 24576000,
		.mask = SDM_BIT | NDP_MASK,
		.val = SDM_BIT | (61 << 8) | BIT(18) | (29 << 0),
						/* n=61 d1=0 d2=1 p=29 */
		.sd_reg = 0x284,
		.sd_val = 0xc000e147,
	},
	{
		.rate = 0,
		.mask = SDM_BIT,
		.val = 0,
	},
};
static const struct ccu_extra audio_extra = {
	CCU_EXTRA_FRAC(audio_fracs),
};
static const struct clk_init_data pll_audio_init __initconst = {
	CCU_HW(pll_audio, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_audio_clk __initconst = {
	CCU_REG(0x008),
	.hw.init = &pll_audio_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 2),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_D2(18, 1),
	CCU_M(0, 6),		/* p = divider */
	.features = CCU_FEATURE_N0,
	.extra = &audio_extra,
};
static const char * const child_pll_audio[] __initconst = { pll_audio };

/* video 0 */
/*	rate = 24MHz * n / (d1 + 1) >> p */
static const char pll_video0[] __initconst = "pll-video0";
static const struct clk_init_data pll_video0_init __initconst = {
	CCU_HW(pll_video0, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_video0_clk __initconst = {
	CCU_REG(0x010),
	.hw.init = &pll_video0_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 3),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_P(0, 2),
	.features = CCU_FEATURE_N0,
};
static const char * const child_pll_video0[] __initconst = { pll_video0 };

/* video engine */
/*	rate = 24MHz * n / (d1 + 1) / (d2 + 1) */
static const char pll_ve[] __initconst = "pll-ve";
static const struct clk_init_data pll_ve_init __initconst = {
	CCU_HW(pll_ve, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_ve_clk __initconst = {
	CCU_REG(0x018),
	.hw.init = &pll_ve_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 4),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_D2(18, 1),
	.features = CCU_FEATURE_N0,
};
static const char * const child_pll_ve[] __initconst = { pll_ve };

/* ddr */
/*	rate = 24MHz * (n + 1) / (d1 + 1) / (d2 + 1)
 *	bit 21: DDR_CLOCK = PLL_DDR / PLL_PERIPH (default DDR)
 */
static const char pll_ddr[] __initconst = "pll-ddr";
static const struct clk_init_data pll_ddr_init __initconst = {
	CCU_HW(pll_ddr, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_ddr_clk __initconst = {
	CCU_REG(0x020),
	.hw.init = &pll_ddr_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 5),
	CCU_N(8, 6), .n_min = 12,
	CCU_D1(16, 1),
	CCU_D2(18, 1),
	CCU_UPD(30),
};
static const char * const child_pll_ddr[] __initconst = { pll_ddr };

/* periph */
/*	rate = 24MHz * n / (d1 + 1) / (d2 + 1) */
static const char pll_periph[] __initconst = "pll-periph";
static const struct clk_init_data pll_periph_init __initconst = {
	CCU_HW(pll_periph, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_periph_clk __initconst = {
	CCU_REG(0x028),
	.hw.init = &pll_periph_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 6),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_D2(18, 1),
	.features = CCU_FEATURE_N0,
};
static const char * const child_pll_periph[] __initconst = { pll_periph };

/* gpu */
/*	rate = 24MHz * n / (d1 + 1) / (d2 + 1) */
static const char pll_gpu[] __initconst = "pll-gpu";
static const struct clk_init_data pll_gpu_init __initconst = {
	CCU_HW(pll_gpu, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_gpu_clk __initconst = {
	CCU_REG(0x038),
	.hw.init = &pll_gpu_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 7),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_D2(18, 1),
	.features = CCU_FEATURE_N0,
};
static const char * const child_pll_gpu[] __initconst = { pll_gpu };

/* hsic */
/*	rate = 24MHz * n / (d1 + 1) / (d2 + 1) */
static const char pll_hsic[] __initconst = "pll-hsic";
static const struct clk_init_data pll_hsic_init __initconst = {
	CCU_HW(pll_hsic, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_hsic_clk __initconst = {
	CCU_REG(0x044),
	.hw.init = &pll_hsic_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 8),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_D2(18, 1),
	.features = CCU_FEATURE_N0,
};

/* display engine */
/*	rate = 24MHz * n / (d1 + 1) / (d2 + 1) */
static const char pll_de[] __initconst = "pll-de";
static const struct clk_init_data pll_de_init __initconst = {
	CCU_HW(pll_de, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_de_clk __initconst = {
	CCU_REG(0x048),
	.hw.init = &pll_de_init,
	CCU_RESET(0x2c4, 12),
	CCU_BUS(0x64, 12),
	CCU_GATE(31),
	CCU_LOCK(0x20c, 9),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_D2(18, 1),
	.features = CCU_FEATURE_N0,
};

/* video 1 */
/*	rate = 24MHz * n / (d1 + 1) >> p */
static const char pll_video1[] __initconst = "pll-video1";
static const struct clk_init_data pll_video1_init __initconst = {
	CCU_HW(pll_video1, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_video1_clk __initconst = {
	CCU_REG(0x04c),
	.hw.init = &pll_video1_init,
	CCU_GATE(31),
	CCU_LOCK(0x20c, 10),
	CCU_N(8, 8), .n_min = 12,
	CCU_D1(16, 1),
	CCU_P(0, 2),
	.features = CCU_FEATURE_N0,
};
static const char * const child_pll_video1[] __initconst = { pll_video1 };

static const char c0cpux[] __initconst = "c0cpux";
static const char * const c0cpux_parents[] __initconst = {
				hosc, pll_c0cpux
};
static const struct clk_init_data c0cpux_init __initconst = {
	CCU_HW(c0cpux, c0cpux_parents, &ccu_periph_ops),
	.flags = CLK_IS_CRITICAL,
};
static const struct ccu c0cpux_clk __initconst = {
	CCU_REG(0x050),
	.hw.init = &c0cpux_init,
	CCU_MUX(12, 1),
};
static const char * const child_c0cpux[] = { c0cpux };

static const char axi0[] __initconst = "axi0";
static const struct clk_init_data axi0_init __initconst = {
	CCU_HW(axi0, child_c0cpux, &ccu_periph_ops),
};
static const struct ccu axi0_clk __initconst = {
	CCU_REG(0x050),
	.hw.init = &axi0_init,
	CCU_M(0, 2),
};

static const char c1cpux[] __initconst = "c1cpux";
static const char * const c1cpux_parents[] __initconst = {
					hosc, pll_c1cpux
};
static const struct clk_init_data c1cpux_init __initconst = {
	CCU_HW(c1cpux, c1cpux_parents, &ccu_periph_ops),
	.flags = CLK_IS_CRITICAL,
};
static const struct ccu c1cpux_clk __initconst = {
	CCU_REG(0x050),
	.hw.init = &c1cpux_init,
	CCU_MUX(28, 1),
};
static const char * const child_c1cpux[] = { c1cpux };

static const char axi1[] __initconst = "axi1";
static const struct clk_init_data axi1_init __initconst = {
	CCU_HW("axi1", child_c1cpux, &ccu_periph_ops),
};
static const struct ccu axi1_clk __initconst = {
	CCU_REG(0x050),
	.hw.init = &axi1_init,
	CCU_M(16, 2),
};

static const char ahb1[] __initconst = "ahb1";
static const char * const ahb1_parents[] __initconst = {
					losc, hosc, pll_periph
};
static const struct ccu_extra ahb1_extra = {
	.variable_prediv = { .index = 2, .shift = 6, .width = 2 },
};
static const struct clk_init_data ahb1_init __initconst = {
	CCU_HW(ahb1, ahb1_parents, &ccu_periph_ops),
};
static const struct ccu ahb1_clk __initconst = {
	CCU_REG(0x054),
	.hw.init = &ahb1_init,
	CCU_MUX(12, 2),
	CCU_P(4, 2),
	.features = CCU_FEATURE_MUX_VARIABLE_PREDIV,
	.extra = &ahb1_extra,
};
static const char * const child_ahb1[] __initconst = { ahb1 };

static const char apb1[] __initconst = "apb1";
static const struct clk_init_data apb1_init __initconst = {
	CCU_HW("apb1", child_ahb1, &ccu_periph_ops),
};
static const struct ccu apb1_clk __initconst = {
	CCU_REG(0x054),

	.hw.init = &apb1_init,
	CCU_M(8, 2),
};
static const char * const child_apb1[] __initconst = { apb1 };

static const char apb2[] __initconst = "apb2";
static const char * const apb2_parents[] __initconst = {
				losc, hosc, pll_periph, pll_periph
};
static const struct clk_init_data apb2_init __initconst = {
	CCU_HW(apb2, apb2_parents, &ccu_periph_ops),
};
static const struct ccu apb2_clk __initconst = {
	CCU_REG(0x058),
	.hw.init = &apb2_init,
	CCU_MUX(24, 2),
	CCU_M(0, 5),
	CCU_P(16, 2),
};
static const char * const child_apb2[] __initconst = { "apb2" };

static const char ahb2[] __initconst = "ahb2";
#if 1 // from bpi-m3 code
static const struct clk_init_data ahb2_init __initconst = {
	CCU_HW(ahb2, child_pll_periph, &ccu_fixed_factor_ops),
};
static const struct ccu ahb2_clk __initconst = {
	.hw.init = &ahb2_init,
	CCU_FIXED(1, 2),
};
#else // from doc
static const char * const ahb2_parents[] __initconst = {
				ahb1, pll_periph
};
static const struct ccu_extra ahb2_extra = {
	.fixed_div = { 1, 2},
};
static const struct clk_init_data ahb2_init __initconst = {
	CCU_HW(ahb2, ahb2_parents, &ccu_periph_ops),
};
static const struct ccu ahb2_clk __initconst = {
	CCU_REG(0x05c),
	.hw.init = &ahb2_init,
	CCU_MUX(0, 2),
	.features = CCU_FEATURE_MUX_FIXED_PREDIV,
	.extra = &ahb2_extra,
};
#endif
static const char * const child_ahb2[] __initconst = { ahb2 };

#if 0 //useless
static const char bus_mipi_dsi[] __initconst = "bus-mipi-dsi";
static const struct clk_init_data bus_mipi_dsi_init __initconst = {
	CCU_HW(bus_mipi_dsi, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_mipi_dsi_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_mipi_dsi_init,
	CCU_GATE(1),
};
static const char bus_ss[] __initconst = "bus-ss";
static const struct clk_init_data bus_ss_init __initconst = {
	CCU_HW(bus_ss, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_ss_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ss_init,
	CCU_GATE(5),
};
#endif
static const char bus_dma[] __initconst = "bus-dma";
static const struct clk_init_data bus_dma_init __initconst = {
	CCU_HW(bus_dma, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_dma_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_dma_init,
	CCU_GATE(6),
};
static const char bus_mmc0[] __initconst = "bus-mmc0";
static const struct clk_init_data bus_mmc0_init __initconst = {
	CCU_HW(bus_mmc0, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_mmc0_clk __initconst = {
////	CCU_REG(0x060),
	.hw.init = &bus_mmc0_init,
////	CCU_GATE(8),
};
static const char bus_mmc1[] __initconst = "bus-mmc1";
static const struct clk_init_data bus_mmc1_init __initconst = {
	CCU_HW(bus_mmc1, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_mmc1_clk __initconst = {
////	CCU_REG(0x060),
	.hw.init = &bus_mmc1_init,
////	CCU_GATE(9),
};
static const char bus_mmc2[] __initconst = "bus-mmc2";
static const struct clk_init_data bus_mmc2_init __initconst = {
	CCU_HW(bus_mmc2, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_mmc2_clk __initconst = {
////	CCU_REG(0x060),
	.hw.init = &bus_mmc2_init,
////	CCU_GATE(10),
};
#if 0 //useless
static const char bus_nand[] __initconst = "bus-nand";
static const struct clk_init_data bus_nand_init __initconst = {
	CCU_HW(bus_nand, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_nand_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_nand_init,
	CCU_GATE(13),
};
static const char bus_dram[] __initconst = "bus-dram";
static const struct clk_init_data bus_dram_init __initconst = {
	CCU_HW(bus_dram, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_dram_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_dram_init,
	CCU_GATE(14),
};
static const char bus_emac[] __initconst = "bus-emac";
static const struct clk_init_data bus_emac_init __initconst = {
	CCU_HW(bus_emac, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_emac_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_emac_init,
	CCU_GATE(17),
};
static const char bus_hstimer[] __initconst = "bus-hstimer";
static const struct clk_init_data bus_hstimer_init __initconst = {
	CCU_HW(bus_hstimer, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_hstimer_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_hstimer_init,
	CCU_GATE(19),
};
static const char bus_spi0[] __initconst = "bus-spi0";
static const struct clk_init_data bus_spi0_init __initconst = {
	CCU_HW(bus_spi0, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_spi0_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_spi0_init
	CCU_GATE(20),
};
static const char bus_spi1[] __initconst = "bus-spi1";
static const struct clk_init_data bus_spi1_init __initconst = {
	CCU_HW(bus_spi1, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_spi1_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_spi1_init,
	CCU_GATE(21),
};
#endif
static const char bus_usbdrd[] __initconst = "bus-usbdrd";
static const struct clk_init_data bus_usbdrd_init __initconst = {
	CCU_HW(bus_usbdrd, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_usbdrd_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_usbdrd_init,
	CCU_GATE(24),
};
static const char bus_ehci0[] __initconst = "bus-ehci0";
static const struct clk_init_data bus_ehci0_init __initconst = {
	CCU_HW(bus_ehci0, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ehci0_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ehci0_init,
	CCU_GATE(26),
};
static const char bus_ehci1[] __initconst = "bus-ehci1";
static const struct clk_init_data bus_ehci1_init __initconst = {
	CCU_HW(bus_ehci1, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ehci1_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ehci1_init,
	CCU_GATE(27),
};
static const char bus_ohci0[] __initconst = "bus-ohci0";
static const struct clk_init_data bus_ohci0_init __initconst = {
	CCU_HW(bus_ohci0, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ohci0_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ohci0_init,
	CCU_GATE(29),
};
#if 0 //useless
static const char bus_ve[] __initconst = "bus-ve";
static const struct clk_init_data bus_ve_init __initconst = {
	CCU_HW(bus_ve, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_ve_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_ve_init,
	CCU_GATE(0),
};
static const char bus_csie[] __initconst = "bus-csi";
static const struct clk_init_data bus_csi_init __initconst = {
	CCU_HW(bus_csi, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_csi_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_csi_init,
	CCU_GATE(8),
};
static const char bus_gpu[] __initconst = "bus-gpu";
static const struct clk_init_data bus_gpu_init __initconst = {
	CCU_HW(bus_gpu, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_gpu_clk= {
	CCU_REG(0x064),
	.hw.init = &bus_gpu_init,
	CCU_GATE(20),
};
static const char bus_msgbox[] __initconst = "bus-msgbox";
static const struct clk_init_data bus_msgbox_init __initconst = {
	CCU_HW(bus_msgbox, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_msgbox_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_msgbox_init,
	CCU_GATE(21),
};
static const char bus_spinlock[] __initconst = "bus-spinlock";
static const struct clk_init_data bus_spinlock_init __initconst = {
	CCU_HW(bus_spinlock, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_spinlock_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_spinlock_init,
	CCU_GATE(22),
};

static const char bus_spdif[] __initconst = "bus-spdif";
static const struct clk_init_data bus_spdif_init __initconst = {
	CCU_HW(bus_spdif, child_apb1, &ccu_periph_ops),
};
static const struct ccu bus_spdif_clk __initconst = {
	CCU_REG(0x068),
	.hw.init = &bus_spdif_init,
	CCU_GATE(1),
};
#endif
static const char bus_pio[] __initconst = "bus-pio";
static const struct clk_init_data bus_pio_init __initconst = {
	CCU_HW(bus_pio, child_apb1, &ccu_periph_ops),
};
static const struct ccu bus_pio_clk __initconst = {
	CCU_REG(0x068),
	.hw.init = &bus_pio_init,
	CCU_GATE(5),
};
#if 0 //useless
static const char bus_i2c0[] __initconst = "bus-i2c0";
static const struct clk_init_data bus_i2c0_init __initconst = {
	CCU_HW(bus_i2c0, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_i2c0_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_i2c0_init,
	CCU_GATE(0),
};
static const char bus_i2c1[] __initconst = "bus-i2c1";
static const struct clk_init_data bus_i2c1_init __initconst = {
	CCU_HW(bus_i2c1, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_i2c1_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_i2c1_init,
	CCU_GATE(1),
};
static const char bus_i2c2[] __initconst = "bus-i2c2";
static const struct clk_init_data bus_i2c2_init __initconst = {
	CCU_HW(bus_i2c2, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_i2c2_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_i2c2_init,
	CCU_GATE(2),
};
#endif
static const char bus_uart0[] __initconst = "bus-uart0";
static const struct clk_init_data bus_uart0_init __initconst = {
	CCU_HW(bus_uart0, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_uart0_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_uart0_init,
	CCU_GATE(16),
};
static const char bus_uart1[] __initconst = "bus-uart1";
static const struct clk_init_data bus_uart1_init __initconst = {
	CCU_HW(bus_uart1, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_uart1_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_uart1_init,
	CCU_GATE(17),
};
static const char bus_uart2[] __initconst = "bus-uart2";
static const struct clk_init_data bus_uart2_init __initconst = {
	CCU_HW(bus_uart2, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_uart2_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_uart2_init,
	CCU_GATE(18),
};
static const char bus_uart3[] __initconst = "bus-uart3";
static const struct clk_init_data bus_uart3_init __initconst = {
	CCU_HW(bus_uart3, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_uart3_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_uart3_init,
	CCU_GATE(19),
};
static const char bus_uart4[] __initconst = "bus-uart4";
static const struct clk_init_data bus_uart4_init __initconst = {
	CCU_HW(bus_uart4, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_uart4_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_uart4_init,
	CCU_GATE(20),
};

static const char cci400[] __initconst = "cci400";
static const char * const cci400_parents[] __initconst = {
				hosc, pll_periph, pll_hsic
};
static const struct clk_init_data cci400_init __initconst = {
	CCU_HW(cci400, cci400_parents, &ccu_periph_ops),
};
static const struct ccu cci400_clk __initconst = {
	CCU_REG(0x078),
	.hw.init = &cci400_init,
	CCU_MUX(24, 2),
	CCU_M(0, 2),
};

static const char nand[] __initconst = "nand";
static const char * const mmc_parents[] __initconst = { hosc, pll_periph };
static const struct clk_init_data nand_init __initconst = {
	CCU_HW(nand, mmc_parents, &ccu_periph_ops),
};
static const struct ccu nand_clk __initconst = {
	CCU_REG(0x080),
	.hw.init = &nand_init,
	CCU_MUX(24, 2),
	CCU_BUS(0x060, 13),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
};

static const char mmc0[] __initconst = "mmc0";
static const struct clk_init_data mmc0_init __initconst = {
	CCU_HW(mmc0, mmc_parents, &ccu_periph_ops),
};
static const struct ccu mmc0_clk __initconst = {
	CCU_REG(0x088),
	.hw.init = &mmc0_init,
	CCU_MUX(24, 2),
	CCU_RESET(0x2c0, 8),
	CCU_BUS(0x060, 8),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
	.features = CCU_FEATURE_SET_RATE_GATE,
};
static const char * const child_mmc0[] __initconst = { mmc0 };
static const char mmc0_sample[] __initconst = "mmc0-sample";
static const struct clk_init_data mmc0_sample_init __initconst = {
	CCU_HW(mmc0_sample, child_mmc0, &ccu_phase_ops),
};
static const struct ccu mmc0_sample_clk __initconst = {
	CCU_REG(0x088),
	.hw.init = &mmc0_sample_init,
	CCU_PHASE(20, 3),
};
static const char mmc0_output[] __initconst = "mmc0-output";
static const struct clk_init_data mmc0_output_init __initconst = {
	CCU_HW(mmc0_output, child_mmc0, &ccu_phase_ops),
};
static const struct ccu mmc0_output_clk __initconst = {
	CCU_REG(0x088),
	.hw.init = &mmc0_output_init,
	CCU_PHASE(8, 3),
};

static const char mmc1[] __initconst = "mmc1";
static const struct clk_init_data mmc1_init __initconst = {
	CCU_HW(mmc1, mmc_parents, &ccu_periph_ops),
};
static const struct ccu mmc1_clk __initconst = {
	CCU_REG(0x08c),
	.hw.init = &mmc1_init,
	CCU_MUX(24, 2),
	CCU_RESET(0x2c0, 9),
	CCU_BUS(0x060, 9),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
	.features = CCU_FEATURE_SET_RATE_GATE,
};
static const char * const child_mmc1[] __initconst = { mmc1 };
static const char mmc1_sample[] __initconst = "mmc1-sample";
static const struct clk_init_data mmc1_sample_init __initconst = {
	CCU_HW(mmc1_sample, child_mmc1, &ccu_phase_ops),
};
static const struct ccu mmc1_sample_clk __initconst = {
	CCU_REG(0x08c),
	.hw.init = &mmc1_sample_init,
	CCU_PHASE(20, 3),
};
static const char mmc1_output[] __initconst = "mmc1-output";
static const struct clk_init_data mmc1_output_init __initconst = {
	CCU_HW(mmc1_output, child_mmc1, &ccu_phase_ops),
};
static const struct ccu mmc1_output_clk __initconst = {
	CCU_REG(0x08c),
	.hw.init = &mmc1_output_init,
	CCU_PHASE(8, 3),
};

static const char mmc2[] __initconst = "mmc2";
static const struct ccu_extra mmc_extra = {
	.mode_select.rate = 50000000,
	.mode_select.bit = 30,
};
static const struct clk_init_data mmc2_init __initconst = {
	CCU_HW(mmc2, mmc_parents, &ccu_periph_ops),
};
static const struct ccu mmc2_clk __initconst = {
	CCU_REG(0x090),
	.hw.init = &mmc2_init,
	CCU_MUX(24, 2),
	CCU_RESET(0x2c0, 10),
	CCU_BUS(0x060, 10),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
	.features = CCU_FEATURE_SET_RATE_GATE |
			CCU_FEATURE_MODE_SELECT,
	.extra = &mmc_extra,
};
static const char * const child_mmc2[] __initconst = { mmc2 };
static const char mmc2_sample[] __initconst = "mmc2-sample";
static const struct clk_init_data mmc2_sample_init __initconst = {
	CCU_HW(mmc2_sample, child_mmc2, &ccu_phase_ops),
};
static const struct ccu mmc2_sample_clk __initconst = {
	CCU_REG(0x090),
	.hw.init = &mmc2_sample_init,
	CCU_PHASE(20, 3),
	.features = CCU_FEATURE_MODE_SELECT,
	.extra = &mmc_extra,
};
static const char mmc2_output[] __initconst = "mmc2-output";
static const struct clk_init_data mmc2_output_init __initconst = {
	CCU_HW(mmc2_output, child_mmc2, &ccu_phase_ops),
};
static const struct ccu mmc2_output_clk __initconst = {
	CCU_REG(0x090),
	.hw.init = &mmc2_output_init,
	CCU_PHASE(8, 3),
	.features = CCU_FEATURE_MODE_SELECT,
	.extra = &mmc_extra,
};

static const char ss[] __initconst = "ss";
static const struct clk_init_data ss_init __initconst = {
	CCU_HW(ss, mmc_parents, &ccu_periph_ops),
};
static const struct ccu ss_clk __initconst = {
	CCU_REG(0x09c),
	.hw.init = &ss_init,
	CCU_MUX(24, 2),
	CCU_BUS(0x060, 5),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
};

static const char spi0[] __initconst = "spi0";
static const struct clk_init_data spi0_init __initconst = {
	CCU_HW(spi0, mmc_parents, &ccu_periph_ops),
};
static const struct ccu spi0_clk __initconst = {
	CCU_REG(0x0a0),
	.hw.init = &spi0_init,
	CCU_MUX(24, 2),
	CCU_BUS(0x060, 20),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
};

static const char spi1[] __initconst = "spi1";
static const struct clk_init_data spi1_init __initconst = {
	CCU_HW(spi1, mmc_parents, &ccu_periph_ops),
};
static const struct ccu spi1_clk __initconst = {
	CCU_REG(0x0a4),
	.hw.init = &spi1_init,
	CCU_MUX(24, 2),
	CCU_BUS(0x060, 21),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
};

static const char i2s0[] __initconst = "i2s0";
static const struct clk_init_data i2s0_init __initconst = {
	CCU_HW(i2s0, child_pll_audio, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu i2s0_clk __initconst = {
	CCU_REG(0x0b0),
	.hw.init = &i2s0_init,
	CCU_BUS(0x068, 12),
	CCU_GATE(31),
	CCU_M(0, 4),
};
static const char i2s1[] __initconst = "i2s1";
static const struct clk_init_data i2s1_init __initconst = {
	CCU_HW(i2s1, child_pll_audio, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu i2s1_clk __initconst = {
	CCU_REG(0x0b4),
	.hw.init = &i2s1_init,
	CCU_GATE(31),
	CCU_BUS(0x068, 13),
	CCU_M(0, 4),
};

static const char i2s2[] __initconst = "i2s2";
static const struct clk_init_data i2s2_init __initconst = {
	CCU_HW(i2s2, child_pll_audio, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu i2s2_clk __initconst = {
	CCU_REG(0x0b8),
	.hw.init = &i2s2_init,
	CCU_GATE(31),
	CCU_BUS(0x068, 14),
	CCU_M(0, 4),
};

static const char tdm[] __initconst = "tdm";
static const struct clk_init_data tdm_init __initconst = {
	CCU_HW(tdm, child_pll_audio, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu tdm_clk __initconst = {
	CCU_REG(0x0bc),
	.hw.init = &tdm_init,
	CCU_BUS(0x068, 15),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char spdif[] __initconst = "spdif";
static const struct clk_init_data spdif_init __initconst = {
	CCU_HW(spdif, child_pll_audio, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu spdif_clk __initconst = {
	CCU_REG(0x0c0),
	.hw.init = &spdif_init,
	CCU_BUS(0x068, 1),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char usb_phy0[] __initconst = "usb-phy0";
static const struct clk_init_data usb_phy0_init __initconst = {
	CCU_HW(usb_phy0, child_hosc, &ccu_periph_ops),
};
static const struct ccu usb_phy0_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &usb_phy0_init,
	CCU_GATE(8),
};

static const char usb_phy1[] __initconst = "usb-phy1";
static const struct clk_init_data usb_phy1_init __initconst = {
	CCU_HW(usb_phy1, child_hosc, &ccu_periph_ops),
};
static const struct ccu usb_phy1_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &usb_phy1_init,
	CCU_GATE(9),
};

static const char usb_hsic[] __initconst = "usb-hsic";
static const struct clk_init_data usb_hsic_init __initconst = {
	CCU_HW(usb_hsic, child_hosc, &ccu_periph_ops),
};
static const struct ccu usb_hsic_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &usb_hsic_init,
	CCU_GATE(10),
};

static const char osc12M[] __initconst = "osc12M";
static const struct clk_init_data osc12M_init __initconst = {
	CCU_HW(osc12M, child_hosc, &ccu_fixed_factor_ops),
};
static const struct ccu osc12M_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &osc12M_init,
	CCU_GATE(11),
	CCU_FIXED(1, 2),
};

static const char ohci0[] __initconst = "ohci0";
static const struct clk_init_data ohci0_init __initconst = {
	CCU_HW(ohci0, child_hosc, &ccu_periph_ops),
};
static const struct ccu ohci0_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &ohci0_init,
	CCU_GATE(16),
};

static const char dram[] __initconst = "dram";
static const struct clk_init_data dram_init __initconst = {
	CCU_HW(dram, child_pll_ddr, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu dram_clk __initconst = {
	CCU_REG(0x0f4),
	.hw.init = &dram_init,
	CCU_BUS(0x060, 14),
	CCU_M(0, 4),
	CCU_UPD(16),
};
static const char * const child_dram[] __initconst = { "dram" };

/* pll_ddr config not done */

static const char dram_ve[] __initconst = "dram-ve";
static const struct clk_init_data dram_ve_init __initconst = {
	CCU_HW(dram_ve, child_dram, &ccu_periph_ops),
};
static const struct ccu dram_ve_clk __initconst = {
	CCU_REG(0x0100),
	.hw.init = &dram_ve_init,
	CCU_BUS(0x064, 0),
	CCU_GATE(0),
};

static const char dram_csi[] __initconst = "dram-csi";
static const struct clk_init_data dram_csi_init __initconst = {
	CCU_HW(dram_csi, child_dram, &ccu_periph_ops),
};
static const struct ccu dram_csi_clk __initconst = {
	CCU_REG(0x0100),
	.hw.init = &dram_csi_init,
	CCU_GATE(1),
};

static const char tcon0[] __initconst = "tcon0";
static const struct clk_init_data tcon0_init __initconst = {
	CCU_HW(tcon0, child_pll_video0, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu tcon0_clk __initconst = {
	CCU_REG(0x118),
	.hw.init = &tcon0_init,
	CCU_MUX(24, 2),
	CCU_RESET(0x2c4, 4),
	CCU_BUS(0x064, 4),
	CCU_GATE(31),
};

static const char tcon1[] __initconst = "tcon1";
static const struct clk_init_data tcon1_init __initconst = {
	CCU_HW(tcon1, child_pll_video1, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu tcon1_clk __initconst = {
	CCU_REG(0x11c),
	.hw.init = &tcon1_init,
	CCU_MUX(24, 2),
	CCU_RESET(0x2c4, 5),
	CCU_BUS(0x064, 5),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char csi_misc[] __initconst = "csi-misc";
static const struct clk_init_data csi_misc_init __initconst = {
	CCU_HW(csi_misc, child_hosc, &ccu_periph_ops),
};
static const struct ccu csi_misc_clk __initconst = {
	CCU_REG(0x0130),
	.hw.init = &csi_misc_init,
	CCU_GATE(16),
};

static const char mipi_csi[] __initconst = "mipi-csi";
static const struct clk_init_data mipi_csi_init __initconst = {
	CCU_HW(mipi_csi, child_hosc, &ccu_periph_ops),
};
static const struct ccu mipi_csi_clk __initconst = {
	CCU_REG(0x0130),
	.hw.init = &mipi_csi_init,
	CCU_GATE(31),
};

static const char csi_sclk[] __initconst = "csi-sclk";
static const char * const csi_sclk_parents[] __initconst = {
			pll_periph, "", "", "", "", pll_ve
};
static const struct clk_init_data csi_sclk_init __initconst = {
	CCU_HW(csi_sclk, csi_sclk_parents, &ccu_periph_ops),
};
static const struct ccu csi_sclk_clk __initconst = {
	CCU_REG(0x134),
	.hw.init = &csi_sclk_init,
	CCU_MUX(24, 3),
	CCU_GATE(31),
	CCU_M(16, 4),
};

static const char csi_mclk[] __initconst = "csi-mclk";
static const char * const csi_mclk_parents[] __initconst = {
			"", "", "", pll_periph, "", hosc
};
static const struct clk_init_data csi_mclk_init __initconst = {
	CCU_HW(csi_mclk, csi_mclk_parents, &ccu_periph_ops),
};
static const struct ccu csi_mclk_clk __initconst = {
	CCU_REG(0x134),
	.hw.init = &csi_mclk_init,
	CCU_MUX(8, 3),
	CCU_GATE(15),
	CCU_M(0, 5),
};

static const char ve[] __initconst = "ve";
static const struct clk_init_data ve_init __initconst = {
	CCU_HW(ve, child_pll_ve, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu ve_clk __initconst = {
	CCU_REG(0x13c),
	.hw.init = &ve_init,
	CCU_GATE(31),
	CCU_M(16, 3),
};

static const char avs[] __initconst = "avs";
static const struct clk_init_data avs_init __initconst = {
	CCU_HW(avs, child_hosc, &ccu_periph_ops),
};
static const struct ccu avs_clk __initconst = {
	CCU_REG(0x0144),
	.hw.init = &avs_init,
	CCU_GATE(31),
};

static const char hdmi[] __initconst = "hdmi";
static const struct clk_init_data hdmi_init __initconst = {
	CCU_HW(hdmi, child_pll_video1, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu hdmi_clk __initconst = {
	CCU_REG(0x150),
	.hw.init = &hdmi_init,
	CCU_MUX(24, 2),
	CCU_BUS(0x064, 11),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char hdmi_ddc[] __initconst = "hdmi-ddc";
static const struct clk_init_data hdmi_ddc_init __initconst = {
	CCU_HW(hdmi_ddc, child_hosc, &ccu_periph_ops),
};
static const struct ccu hdmi_ddc_clk __initconst = {
	CCU_REG(0x0154),
	.hw.init = &hdmi_ddc_init,
	CCU_GATE(31),
};

static const char mbus[] __initconst = "mbus";
static const char * const mbus_parents[] __initconst = {
				hosc, pll_periph, pll_ddr
};
static const struct clk_init_data mbus_init __initconst = {
	CCU_HW(mbus, mbus_parents, &ccu_periph_ops),
	.flags = CLK_IS_CRITICAL,
};
static const struct ccu mbus_clk __initconst = {
	CCU_REG(0x15c),
	.hw.init = &mbus_init,
	CCU_MUX(24, 2),
	CCU_GATE(31),
	CCU_M(0, 3),
};

static const char mipi_dsi0[] __initconst = "mipi-dsi0";
static const struct clk_init_data mipi_dsi0_init __initconst = {
	CCU_HW(mipi_dsi0, child_pll_video0, &ccu_periph_ops),
};
static const struct ccu mipi_dsi0_clk __initconst = {
	CCU_REG(0x168),
	.hw.init = &mipi_dsi0_init,
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char mipi_dsi1[] __initconst = "mipi-dsi1";
static const char * const mipi_dsi1_parents[] __initconst = {
			hosc, "", "", "", "", "", "", "", "", pll_video0
};
static const struct clk_init_data mipi_dsi1_init __initconst = {
	CCU_HW(mipi_dsi1, mipi_dsi1_parents, &ccu_periph_ops),
};
static const struct ccu mipi_dsi1_clk __initconst = {
	CCU_REG(0x16c),
	.hw.init = &mipi_dsi1_init,
	CCU_MUX(24, 4),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char gpu_core[] __initconst = "gpu-core";
static const struct clk_init_data gpu_core_init __initconst = {
	CCU_HW(gpu_core, child_pll_gpu, &ccu_periph_ops),
};
static const struct ccu gpu_core_clk __initconst = {
	CCU_REG(0x1a0),
	.hw.init = &gpu_core_init,
	CCU_BUS(0x064, 20),
	CCU_GATE(31),
	CCU_M(0, 3),
};

static const char gpu_mem[] __initconst = "gpu-mem";
static const char * const gpu_mem_parents[] __initconst = {
			pll_gpu, pll_periph
};
static const struct clk_init_data gpu_mem_init __initconst = {
	CCU_HW(gpu_mem, gpu_mem_parents, &ccu_periph_ops),
};
static const struct ccu gpu_mem_clk __initconst = {
	CCU_REG(0x1a4),
	.hw.init = &gpu_mem_init,
	CCU_MUX(24, 1),
	CCU_GATE(31),
	CCU_M(0, 3),
};

static const char gpu_hyd[] __initconst = "gpu-hyd";
static const struct clk_init_data gpu_hyd_init __initconst = {
	CCU_HW(gpu_hyd, child_pll_gpu, &ccu_periph_ops),
};
static const struct ccu gpu_hyd_clk __initconst = {
	CCU_REG(0x1a8),
	.hw.init = &gpu_hyd_init,
	CCU_GATE(31),
	CCU_M(0, 3),
};

/* ---- PRCM ---- */
static const char * const cpus_parents[] __initconst = {
			losc, hosc, pll_periph, iosc
};
//fixme: postdiv, not prediv...
static const char cpus[] __initconst = "cpus";
static const struct ccu_extra cpus_extra = {
	.variable_prediv = { .index = 2, .shift = 8, .width = 4 },
};
static const struct clk_init_data cpus_init __initconst = {
	CCU_HW(cpus, cpus_parents, &ccu_periph_ops),
};
static const struct ccu cpus_clk __initconst = {	/* = ahb0 */
	CCU_REG1(0x000),
	.hw.init = &cpus_init,
	CCU_MUX(16, 2),
	CCU_P(4, 2),
	.features = CCU_FEATURE_MUX_VARIABLE_PREDIV,
	.extra = &cpus_extra,
};
static const char * const child_cpus[] __initconst = { cpus };

static const char apb0[] __initconst = "apb0";
static const struct clk_init_data apb0_init __initconst = {
	CCU_HW(apb0, child_cpus, &ccu_periph_ops),
};
static const struct ccu apb0_clk __initconst = {
	CCU_REG1(0x00c),
	.hw.init = &apb0_init,
	CCU_M(0, 2),
};
static const char * const child_apb0[] __initconst = { apb0 };

static const char bus_r_pio[] __initconst = "bus-r_pio";
static const struct clk_init_data bus_r_pio_init __initconst = {
	CCU_HW(bus_r_pio, child_apb0, &ccu_periph_ops),
};
static const struct ccu bus_r_pio_clk __initconst = {
	CCU_REG1(0x028),
	.hw.init = &bus_r_pio_init,
	CCU_GATE(0),
};
static const char bus_r_cir[] __initconst = "bus-r_cir";
static const struct clk_init_data bus_r_cir_init __initconst = {
	CCU_HW(bus_r_cir, child_apb0, &ccu_periph_ops),
};
static const struct ccu bus_r_cir_clk __initconst = {
	CCU_REG1(0x028),
	.hw.init = &bus_r_cir_init,
	CCU_GATE(1),
};

static const char bus_r_timer[] __initconst = "bus-r_timer";
static const struct clk_init_data bus_r_timer_init __initconst = {
	CCU_HW(bus_r_timer, child_apb0, &ccu_periph_ops),
};
static const struct ccu bus_r_timer_clk __initconst = {
	CCU_REG1(0x028),
	.hw.init = &bus_r_timer_init,
	CCU_GATE(2),
};
static const char bus_r_rsb[] __initconst = "bus-r_rsb";
static const struct clk_init_data bus_r_rsb_init __initconst = {
	CCU_HW(bus_r_rsb, child_apb0, &ccu_periph_ops),
};
static const struct ccu bus_r_rsb_clk __initconst = {
	CCU_REG1(0x028),
	.hw.init = &bus_r_rsb_init,
	CCU_GATE(3),
};
static const char bus_r_uart[] __initconst = "bus-r_uart";
static const struct clk_init_data bus_r_uart_init __initconst = {
	CCU_HW(bus_r_uart, child_apb0, &ccu_periph_ops),
};
static const struct ccu bus_r_uart_clk __initconst = {
	CCU_REG1(0x028),
	.hw.init = &bus_r_uart_init,
	CCU_GATE(4),
};
static const char bus_r_twi[] __initconst = "bus-r_twi";
static const struct clk_init_data bus_r_twi_init __initconst = {
	CCU_HW(bus_r_twi, child_apb0, &ccu_periph_ops),
};
static const struct ccu bus_r_twi_clk __initconst = {
	CCU_REG1(0x028),
	.hw.init = &bus_r_twi_init,
	CCU_GATE(6),
};
static const char bus_r_twd[] __initconst = "bus-r_twd";
static const struct clk_init_data bus_r_twd_init __initconst = {
	CCU_HW(bus_r_twd, child_apb0, &ccu_periph_ops),
};
static const struct ccu bus_r_twd_clk __initconst = {
	CCU_REG1(0x028),
	.hw.init = &bus_r_twd_init,
	CCU_GATE(7),
};

/* system clocks and resets */
static const struct ccu * const sun8i_a83t_ccu_clks[] __initconst = {
	[CLK_BUS_DMA]		= &bus_dma_clk,
	[CLK_BUS_EHCI0]		= &bus_ehci0_clk,
	[CLK_BUS_EHCI1]		= &bus_ehci1_clk,
	[CLK_BUS_MMC0]		= &bus_mmc0_clk,
	[CLK_BUS_MMC1]		= &bus_mmc1_clk,
	[CLK_BUS_MMC2]		= &bus_mmc2_clk,
	[CLK_BUS_OHCI0]		= &bus_ohci0_clk,
	[CLK_BUS_PIO]		= &bus_pio_clk,
	[CLK_BUS_UART0]		= &bus_uart0_clk,
	[CLK_BUS_UART1]		= &bus_uart1_clk,
	[CLK_BUS_UART2]		= &bus_uart2_clk,
	[CLK_BUS_UART3]		= &bus_uart3_clk,
	[CLK_BUS_UART4]		= &bus_uart4_clk,
	[CLK_BUS_USBDRD]	= &bus_usbdrd_clk,
	[CLK_I2S0]		= &i2s0_clk,
	[CLK_I2S1]		= &i2s1_clk,
	[CLK_I2S2]		= &i2s2_clk,
	[CLK_HDMI]		= &hdmi_clk,
	[CLK_HDMI_DDC]		= &hdmi_ddc_clk,
	[CLK_MMC0]		= &mmc0_clk,
	[CLK_MMC0_SAMPLE]	= &mmc0_sample_clk,
	[CLK_MMC0_OUTPUT]	= &mmc0_output_clk,
	[CLK_MMC1]		= &mmc1_clk,
	[CLK_MMC1_SAMPLE]	= &mmc1_sample_clk,
	[CLK_MMC1_OUTPUT]	= &mmc1_output_clk,
	[CLK_MMC2]		= &mmc2_clk,
	[CLK_MMC2_SAMPLE]	= &mmc2_sample_clk,
	[CLK_MMC2_OUTPUT]	= &mmc2_output_clk,
	[CLK_OHCI0]		= &ohci0_clk,
	[CLK_OSC12M]		= &osc12M_clk,
	[CLK_PLL_AUDIO]		= &pll_audio_clk,
	[CLK_PLL_DE]		= &pll_de_clk,
	[CLK_PLL_GPU]		= &pll_gpu_clk,
	[CLK_PLL_HSIC]		= &pll_hsic_clk,
	[CLK_PLL_PERIPH]	= &pll_periph_clk,
	[CLK_PLL_VE]		= &pll_ve_clk,
	[CLK_PLL_VIDEO0]	= &pll_video0_clk,
	[CLK_PLL_VIDEO1]	= &pll_video1_clk,
	[CLK_SPDIF]		= &spdif_clk,
	[CLK_SPI0]		= &spi0_clk,
	[CLK_SPI1]		= &spi1_clk,
	[CLK_TCON0]		= &tcon0_clk,
	[CLK_TCON1]		= &tcon1_clk,
	[CLK_TDM]		= &tdm_clk,
	[CLK_USB_PHY0]		= &usb_phy0_clk,
	[CLK_USB_PHY1]		= &usb_phy1_clk,
	[CLK_USB_HSIC]		= &usb_hsic_clk,
	[CLK_VE]		= &ve_clk,
	[CLK_BUS_R_PIO]		= &bus_r_pio_clk,
	[CLK_BUS_R_RSB]		= &bus_r_rsb_clk,
	&pll_c0cpux_clk,
	&pll_c1cpux_clk,
	&pll_ddr_clk,
	&c0cpux_clk,
	&axi0_clk,
	&c1cpux_clk,
	&axi1_clk,
	&ahb1_clk,
	&apb1_clk,
	&apb2_clk,
	&ahb2_clk,
//	&bus_mipi_dsi_clk,
//	&bus_ss_clk,
//	&bus_nand_clk,
//	&bus_dram_clk,
//	&bus_emac_clk,
//	&bus_hstimer_clk,
//	&bus_spi0_clk,
//	&bus_spi1_clk,
//	&bus_ve_clk,
//	&bus_csi_clk,
//	&bus_gpu_clk,
//	&bus_msgbox_clk,
//	&bus_spinlock_clk,
//	&bus_spdif_clk,
//	&bus_i2c0_clk,
//	&bus_i2c1_clk,
//	&bus_i2c2_clk,
	&cci400_clk,
	&nand_clk,
	&ss_clk,
	&dram_clk,
	&dram_ve_clk,
	&dram_csi_clk,
	&csi_misc_clk,
	&mipi_csi_clk,
	&csi_sclk_clk,
	&csi_mclk_clk,
	&avs_clk,
	&mbus_clk,
	&mipi_dsi0_clk,
	&mipi_dsi1_clk,
	&gpu_core_clk,
	&gpu_mem_clk,
	&gpu_hyd_clk,
	&cpus_clk,
	&apb0_clk,
	&bus_r_cir_clk,
	&bus_r_timer_clk,
	&bus_r_uart_clk,
	&bus_r_twi_clk,
	&bus_r_twd_clk,
};

static const struct ccu_reset_map sun8i_a83t_ccu_resets[] __initconst = {
	[RST_USB_PHY0]	= { 0x0cc, 0 },
	[RST_USB_PHY1]	= { 0x0cc, 1 },
	[RST_USB_HSIC]	= { 0x0cc, 2 },

	[RST_DRAM_CTR]	= { 0x0f4, 31 },
	[RST_MBUS]	= { 0x0fc, 31 },

	[RST_MIPI_DSI]	= { 0x2c0, 1 },
	[RST_CE]	= { 0x2c0, 5 },
	[RST_DMA]	= { 0x2c0, 6 },
//	[RST_DE]	= { 0x2c0, 5 },
//	[RST_MMC0]	= { 0x2c0, 8 },
//	[RST_MMC1]	= { 0x2c0, 9 },
//	[RST_MMC2]	= { 0x2c0, 10 },
	[RST_NAND]	= { 0x2c0, 13 },
	[RST_DRAM]	= { 0x2c0, 14 },
	[RST_EMAC]	= { 0x2c0, 17 },
	[RST_HSTIMER]	= { 0x2c0, 19 },
	[RST_SPI0]	= { 0x2c0, 20 },
	[RST_SPI1]	= { 0x2c0, 21 },
	[RST_USBDRD]	= { 0x2c0, 24 },
	[RST_EHCI0]	= { 0x2c0, 26 },
	[RST_EHCI1]	= { 0x2c0, 27 },
	[RST_OHCI0]	= { 0x2c0, 29 },

	[RST_VE]	= { 0x2c4, 0 },
//	[RST_TCON0]	= { 0x2c4, 4 },
//	[RST_TCON1]	= { 0x2c4, 5 },
	[RST_CSI]	= { 0x2c4, 8 },
	[RST_HDMI0]	= { 0x2c4, 10 },
	[RST_HDMI1]	= { 0x2c4, 11 },
	[RST_GPU]	= { 0x2c4, 20 },
	[RST_MSGBOX]	= { 0x2c4, 21 },
	[RST_SPINLOCK]	= { 0x2c4, 22 },

	[RST_LVDS]	= { 0x2c8, 0 },

	[RST_SPDIF]	= { 0x2d0, 1 },
	[RST_I2S0]	= { 0x2d0, 12 },
	[RST_I2S1]	= { 0x2d0, 13 },
	[RST_I2S2]	= { 0x2d0, 14 },
	[RST_TDM]	= { 0x2d0, 15 },

	[RST_I2C0]	= { 0x2d8, 0 },
	[RST_I2C1]	= { 0x2d8, 1 },
	[RST_I2C2]	= { 0x2d8, 2 },
	[RST_UART0]	= { 0x2d8, 16 },
	[RST_UART1]	= { 0x2d8, 17 },
	[RST_UART2]	= { 0x2d8, 18 },
	[RST_UART3]	= { 0x2d8, 19 },
	[RST_UART4]	= { 0x2d8, 20 },

	[RST_R_CIR]	= { 0x0b0, 1, 1 },
	[RST_R_TIMER]	= { 0x0b0, 2, 1 },
	[RST_R_RSB]	= { 0x0b0, 3, 1 },
	[RST_R_UART]	= { 0x0b0, 4, 1 },
	[RST_R_TWI]	= { 0x0b0, 6, 1 },
};

static void __init sun8i_a83t_ccu_setup(struct device_node *node)
{
	ccu_probe(node, sun8i_a83t_ccu_clks,
			ARRAY_SIZE(sun8i_a83t_ccu_clks),
			sun8i_a83t_ccu_resets,
			ARRAY_SIZE(sun8i_a83t_ccu_resets));
}
CLK_OF_DECLARE(sun8i_a83t_ccu, "allwinner,sun8i-a83t-ccu",
	       sun8i_a83t_ccu_setup);
