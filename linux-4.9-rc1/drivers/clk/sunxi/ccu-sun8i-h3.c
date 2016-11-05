/*
 * Copyright (c) 2016 Jean-Francois Moine <moinejf@free.fr>
 * Based on 'sunxi-ng' from
 * Copyright (c) 2016 Maxime Ripard. All rights reserved.
 * and sun8i SDK
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk-provider.h>
#include <linux/reset-controller.h>

#include <dt-bindings/clock/sun8i-h3.h>
#include <dt-bindings/reset/sun8i-h3.h>

#include "ccu.h"

static const char losc[] __initconst = "osc32k";
static const char hosc[] __initconst = "osc24M";
static const char * const child_hosc[] __initconst = { hosc };

/* ---- CCU ---- */

/* cpux */
/*	rate = 24MHz * (n - 1) * (k - 1) / (m - 1) >> p */
static const char pll_cpux[] __initconst = "pll-cpux";
static const struct clk_init_data pll_cpux_init __initconst = {
	CCU_HW(pll_cpux, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_cpux_clk __initconst = {
	CCU_REG(0x000),
	.hw.init = &pll_cpux_init,
	CCU_GATE(31),
	CCU_LOCK(0x000, 28),
	CCU_N(8, 5),
	CCU_K(4, 2),
	CCU_M(0, 2),
	CCU_P(16, 2),		/* only if rate < 288MHz */
	.features = CCU_FEATURE_FLAT_FACTORS,
};

/* audio */
/*	rate = 22579200Hz or 24576000Hz with sigma-delta */
/*   or	rate = 24Hz * (n + 1) / (m + 1) / (p + 1) */
static const char pll_audio[] __initconst = "pll-audio";
#define NMP_MASK (GENMASK(14, 8) | GENMASK(4, 0) | GENMASK(19, 16))
#define SDM_BIT BIT(24)
static const struct frac audio_fracs[] = {
	{
		.rate = 22579200,
		.mask = SDM_BIT | NMP_MASK,
		.val = SDM_BIT | (6 << 8) | (7 << 16), /* n=6 p=7 */
		.sd_reg = 0x284,
		.sd_val = 0xc0010d84,
	},
	{
		.rate = 24576000,
		.mask =  SDM_BIT | NMP_MASK,
		.val = SDM_BIT | (13 << 8) | (13 << 16), /* n=13 p=13 */
		.sd_reg = 0x284,
		.sd_val = 0xc000ac02,
	},
	{
		.rate = 0,
		.mask = SDM_BIT | NMP_MASK,
		.val = 3 << 16,				/* p=4 */
	},
};
static const struct ccu_extra audio_extra = {
	CCU_EXTRA_FRAC(audio_fracs),
	CCU_EXTRA_POST_DIV(4),		/* not used when sigma-delta */
};
static const struct clk_init_data pll_audio_init __initconst = {
	CCU_HW(pll_audio, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_audio_clk __initconst = {
	CCU_REG(0x008),
	.hw.init = &pll_audio_init,
	CCU_GATE(31),
	CCU_LOCK(0x008, 28),
	CCU_N(8, 7),
	CCU_M(0, 5),
	/* p set by fractional table */
	.features = CCU_FEATURE_FIXED_POSTDIV,
	.extra = &audio_extra,
};
static const char * const child_pll_audio[] __initconst = { pll_audio };

static const char pll_audio_2x[] __initconst = "pll-audio-2x";
static const struct clk_init_data pll_audio_2x_init __initconst = {
	CCU_HW(pll_audio_2x, child_pll_audio, &ccu_fixed_factor_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu pll_audio_2x_clk __initconst = {
	.hw.init = &pll_audio_2x_init,
	CCU_FIXED(2, 1),
};

static const char pll_audio_4x[] __initconst = "pll-audio-4x";
static const struct clk_init_data pll_audio_4x_init __initconst = {
	CCU_HW(pll_audio_4x, child_pll_audio, &ccu_fixed_factor_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu pll_audio_4x_clk __initconst = {
	.hw.init = &pll_audio_4x_init,
	CCU_FIXED(4, 1),
};

static const char pll_audio_8x[] __initconst = "pll-audio-8x";
static const struct clk_init_data pll_audio_8x_init __initconst = {
	CCU_HW(pll_audio_8x, child_pll_audio, &ccu_fixed_factor_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu pll_audio_8x_clk __initconst = {
	.hw.init = &pll_audio_8x_init,
	CCU_FIXED(8, 1),
};

/* video */
/*	rate = 24MHz * (n + 1) / (m + 1) */
static const char pll_video[] __initconst = "pll-video";
#define M_MASK_0_3 0x0f
static const struct frac video_fracs[] = {
/*	   rate		mask			    val */
	{270000000, M_MASK_0_3 | BIT(24) | BIT(25), 0},
	{297000000, M_MASK_0_3 | BIT(24) | BIT(25), BIT(25)},
	{0,			 BIT(24) | BIT(25), BIT(24)},
};
static const struct ccu_extra video_extra = {
	CCU_EXTRA_FRAC(video_fracs),
};
static const struct clk_init_data pll_video_init __initconst = {
	CCU_HW(pll_video, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_video_clk __initconst = {
	CCU_REG(0x010),
	.hw.init = &pll_video_init,
	CCU_GATE(31),
	CCU_LOCK(0x010, 28),
	CCU_N(8, 7),
	CCU_M(0, 4),
	.extra = &video_extra,
};
static const char * const child_pll_video[] __initconst = { pll_video };

/* video engine */
/*	rate = 24MHz * (n + 1) / (m + 1) */
static const char pll_ve[] __initconst = "pll-ve";
static const struct clk_init_data pll_ve_init __initconst = {
	CCU_HW(pll_ve, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_ve_clk __initconst = {
	CCU_REG(0x018),
	.hw.init = &pll_ve_init,
	CCU_GATE(31),
	CCU_LOCK(0x018, 28),
	CCU_N(8, 7),
	CCU_M(0, 4),
	.extra = &video_extra,
};
static const char * const child_pll_ve[] __initconst = { pll_ve };

/* ddr */
/*	rate = 24MHz * (n + 1) * (k + 1) / (m + 1)
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
	CCU_LOCK(0x020, 28),
	CCU_N(8, 5),
	CCU_K(4, 2),
	CCU_M(0, 2),
	CCU_UPD(20),
};
static const char * const child_pll_ddr[] __initconst = { pll_ddr };

/* periph0 */
/*	rate = 24MHz * (n + 1) * (k + 1) / 2 */
static const char pll_periph0[] __initconst = "pll-periph0";
static const struct ccu_extra periph_extra = {
	CCU_EXTRA_POST_DIV(2),
};
static const struct clk_init_data pll_periph0_init __initconst = {
	CCU_HW(pll_periph0, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_periph0_clk __initconst = {
	CCU_REG(0x028),
	.hw.init = &pll_periph0_init,
	CCU_GATE(31),
	CCU_LOCK(0x028, 28),
	CCU_N(8, 5),
	CCU_K(4, 2),
	.features = CCU_FEATURE_FIXED_POSTDIV,
	.extra = &periph_extra,
};
static const char * const child_pll_periph0[] = { pll_periph0 };

static const char pll_periph0_2x[] __initconst = "pll-periph0-2x";
static const struct clk_init_data pll_periph0_2x_init __initconst = {
	CCU_HW(pll_periph0_2x, child_pll_periph0, &ccu_fixed_factor_ops),
};
static const struct ccu pll_periph0_2x_clk __initconst = {
	.hw.init = &pll_periph0_2x_init,
	CCU_FIXED(2, 1),
};

/* gpu */
/*	rate = 24MHz * (n + 1) / (m + 1) */
static const char pll_gpu[] __initconst = "pll-gpu";
static const struct clk_init_data pll_gpu_init __initconst = {
	CCU_HW(pll_gpu, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_gpu_clk __initconst = {
	CCU_REG(0x038),
	.hw.init = &pll_gpu_init,
	CCU_GATE(31),
	CCU_LOCK(0x038, 28),
	CCU_N(8, 7),
	CCU_M(0, 4),
	.extra = &video_extra,
};
static const char * const child_pll_gpu[] __initconst = { pll_gpu };

/* periph1 */
/*	rate = 24MHz * (n + 1) * (k + 1) / 2 */
static const char pll_periph1[] __initconst = "pll-periph1";
static const struct clk_init_data pll_periph1_init __initconst = {
	CCU_HW(pll_periph1, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_periph1_clk __initconst = {
	CCU_REG(0x044),
	.hw.init = &pll_periph1_init,
	CCU_GATE(31),
	CCU_LOCK(0x044, 28),
	CCU_N(8, 5),
	CCU_K(4, 2),
	.features = CCU_FEATURE_FIXED_POSTDIV,
	.extra = &periph_extra,
};

/* display engine */
/*	rate = 24MHz * (n + 1) / (m + 1) */
static const char pll_de[] __initconst = "pll-de";
static const struct clk_init_data pll_de_init __initconst = {
	CCU_HW(pll_de, child_hosc, &ccu_pll_ops),
};
static const struct ccu pll_de_clk __initconst = {
	CCU_REG(0x048),
	.hw.init = &pll_de_init,
	CCU_GATE(31),
	CCU_LOCK(0x048, 28),
	CCU_N(8, 7),
	CCU_M(0, 4),
	.extra = &video_extra,
};

static const char * const cpux_parents[] __initconst = {
				losc, hosc, pll_cpux, pll_cpux
};
static const char cpux[] __initconst = "cpux";
static const struct clk_init_data cpux_init __initconst = {
	CCU_HW(cpux, cpux_parents, &ccu_periph_ops),
	.flags = CLK_IS_CRITICAL,
};
static const struct ccu cpux_clk __initconst = {
	CCU_REG(0x050),
	.hw.init = &cpux_init,
	CCU_MUX(16, 2),
};
static const char * const child_cpux[] __initconst = { cpux };

static const char axi[] __initconst = "axi";
static const struct clk_init_data axi_init __initconst = {
	CCU_HW(axi, child_cpux, &ccu_periph_ops),
};
static const struct ccu axi_clk __initconst = {
	CCU_REG(0x050),
	.hw.init = &axi_init,
	CCU_M(0, 2),
};

static const char ahb1[] __initconst = "ahb1";
static const char * const ahb1_parents[] __initconst = {
				losc, hosc, axi, pll_periph0
};
static const struct ccu_extra ahb1_extra = {
	.variable_prediv = { .index = 3, .shift = 6, .width = 2 },
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
static const struct ccu_extra apb1_extra = {
	.m_table = { 2, 2, 4, 8 },
};
static const struct clk_init_data apb1_init __initconst = {
	CCU_HW(apb1, child_ahb1, &ccu_periph_ops),
};
static const struct ccu apb1_clk __initconst = {
	CCU_REG(0x054),
	.hw.init = &apb1_init,
	CCU_M(8, 2),
	.features = CCU_FEATURE_M_TABLE,
	.extra = &apb1_extra,
};
static const char * const child_apb1[] __initconst = { apb1 };

static const char apb2[] __initconst = "apb2";
static const char * const apb2_parents[] __initconst = {
				losc, hosc, pll_periph0, pll_periph0
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
static const char * const child_apb2[] __initconst = { apb2 };

static const char ahb2[] __initconst = "ahb2";
static const char * const ahb2_parents[] __initconst = {
				ahb1, pll_periph0
};
static const struct ccu_extra ahb2_extra = {
	.fixed_div = { 1, 2 },
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
static const char * const child_ahb2[] __initconst = { ahb2 };

static const char bus_ce[] __initconst = "bus-ce";
static const struct clk_init_data bus_ce_init __initconst = {
	CCU_HW(bus_ce, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_ce_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ce_init,
	CCU_GATE(5),
};
static const char bus_dma[] __initconst = "bus-dma";
static const struct clk_init_data bus_dma_init __initconst = {
	CCU_HW(bus_dma, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_dma_clk __initconst = {
	CCU_REG(0x060),
//	CCU_HW(bus_dma", "ahb1", &ccu_periph_ops, 0),
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
	CCU_HW(bus_emac, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_emac_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_emac_init,
	CCU_GATE(17),
};
static const char bus_ts[] __initconst = "bus-ts";
static const struct clk_init_data bus_ts_init __initconst = {
	CCU_HW(bus_ts, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_ts_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ts_init,
	CCU_GATE(18),
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
	.hw.init = &bus_spi0_init,
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
static const char bus_otg[] __initconst = "bus-otg";
static const struct clk_init_data bus_otg_init __initconst = {
	CCU_HW(bus_otg, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_otg_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_otg_init,
	CCU_GATE(23),
};
static const char bus_ehci0[] __initconst = "bus-ehci0";
static const struct clk_init_data bus_ehci0_init __initconst = {
	CCU_HW(bus_ehci0, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_ehci0_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ehci0_init,
	CCU_GATE(24),
};
static const char bus_ehci1[] __initconst = "bus-ehci1";
static const struct clk_init_data bus_ehci1_init __initconst = {
	CCU_HW(bus_ehci1, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ehci1_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ehci1_init,
	CCU_GATE(25),
};
static const char bus_ehci2[] __initconst = "bus-ehci2";
static const struct clk_init_data bus_ehci2_init __initconst = {
	CCU_HW(bus_ehci2, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ehci2_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ehci2_init,
	CCU_GATE(26),
};
static const char bus_ehci3[] __initconst = "bus-ehci3";
static const struct clk_init_data bus_ehci3_init __initconst = {
	CCU_HW(bus_ehci3, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ehci3_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ehci3_init,
	CCU_GATE(27),
};
static const char bus_ohci0[] __initconst = "bus-ohci0";
static const struct clk_init_data bus_ohci0_init __initconst = {
	CCU_HW(bus_ohci0, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_ohci0_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ohci0_init,
	CCU_GATE(28),
};
static const char bus_ohci1[] __initconst = "bus-ohci1";
static const struct clk_init_data bus_ohci1_init __initconst = {
	CCU_HW(bus_ohci1, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ohci1_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ohci1_init,
	CCU_GATE(29),
};
static const char bus_ohci2[] __initconst = "bus-ohci2";
static const struct clk_init_data bus_ohci2_init __initconst = {
	CCU_HW(bus_ohci2, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ohci2_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ohci2_init,
	CCU_GATE(30),
};
static const char bus_ohci3[] __initconst = "bus-ohci3";
static const struct clk_init_data bus_ohci3_init __initconst = {
	CCU_HW(bus_ohci3, child_ahb2, &ccu_periph_ops),
};
static const struct ccu bus_ohci3_clk __initconst = {
	CCU_REG(0x060),
	.hw.init = &bus_ohci3_init,
	CCU_GATE(31),
};

static const char bus_ve[] __initconst = "bus-ve";
static const struct clk_init_data bus_ve_init __initconst = {
	CCU_HW(bus_ve, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_ve_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_ve_init,
	CCU_GATE(0),
};
static const char bus_deinterlace[] __initconst = "bus-deinterlace";
static const struct clk_init_data bus_deinterlace_init __initconst = {
	CCU_HW(bus_deinterlace, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_deinterlace_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_deinterlace_init,
	CCU_GATE(5),
};
static const char bus_csi[] __initconst = "bus-csi";
static const struct clk_init_data bus_csi_init __initconst = {
	CCU_HW(bus_csi, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_csi_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_csi_init,
	CCU_GATE(8),
};
static const char bus_tve[] __initconst = "bus-tve";
static const struct clk_init_data bus_tve_init __initconst = {
	CCU_HW(bus_tve, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_tve_clk __initconst = {
	CCU_REG(0x064),
	.hw.init = &bus_tve_init,
	CCU_GATE(9),
};
static const char bus_gpu[] __initconst = "bus-gpu";
static const struct clk_init_data bus_gpu_init __initconst = {
	CCU_HW(bus_gpu, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_gpu_clk __initconst = {
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

static const char bus_codec[] __initconst = "bus-codec";
static const struct clk_init_data bus_codec_init __initconst = {
	CCU_HW(bus_codec, child_apb1, &ccu_periph_ops),
};
static const struct ccu bus_codec_clk __initconst = {
	CCU_REG(0x068),
	.hw.init = &bus_codec_init,
	CCU_GATE(0),
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
static const char bus_pio[] __initconst = "bus-pio";
static const struct clk_init_data bus_pio_init __initconst = {
	CCU_HW(bus_pio, child_apb1, &ccu_periph_ops),
};
static const struct ccu bus_pio_clk __initconst = {
	CCU_REG(0x068),
	.hw.init = &bus_pio_init,
	CCU_GATE(5),
};
//static const char bus_ths[] __initconst = "bus-ths";
//static const struct clk_init_data bus_ths_init __initconst = {
//	CCU_HW(bus_ths, child_apb1, &ccu_periph_ops),
//};
//static const struct ccu bus_ths_clk __initconst = {
//	CCU_REG(0x068),
//	.hw.init = &bus_ths_init,
//	CCU_GATE(8),
//};

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
static const char bus_scr[] __initconst = "bus-scr";
static const struct clk_init_data bus_scr_init __initconst = {
	CCU_HW(bus_scr, child_apb2, &ccu_periph_ops),
};
static const struct ccu bus_scr_clk __initconst = {
	CCU_REG(0x06c),
	.hw.init = &bus_scr_init,
	CCU_GATE(20),
};

static const char bus_ephy[] __initconst = "bus-ephy";
static const struct clk_init_data bus_ephy_init __initconst = {
	CCU_HW(bus_ephy, child_apb1, &ccu_periph_ops),
};
static const struct ccu bus_ephy_clk __initconst = {
	CCU_REG(0x070),
	.hw.init = &bus_ephy_init,
	CCU_GATE(0),
};
static const char bus_dbg[] __initconst = "bus-dbg";
static const struct clk_init_data bus_dbg_init __initconst = {
	CCU_HW(bus_dbg, child_ahb1, &ccu_periph_ops),
};
static const struct ccu bus_dbg_clk __initconst = {
	CCU_REG(0x070),
	.hw.init = &bus_dbg_init,
	CCU_GATE(7),
};

static const char ths[] __initconst = "ths";
static const struct ccu_extra ths_extra = {
	.m_table = { 1, 2, 4, 6 },
};
static const struct clk_init_data ths_init __initconst = {
	CCU_HW(ths, child_hosc, &ccu_periph_ops),
};
static const struct ccu ths_clk __initconst = {
	CCU_REG(0x074),
	.hw.init = &ths_init,
	CCU_MUX(24, 2),
	CCU_BUS(0x068, 8),
	CCU_GATE(31),
	CCU_M(0, 2),
	.features = CCU_FEATURE_M_TABLE,
	.extra = &ths_extra,
};

static const char nand[] __initconst = "nand";
static const char * const mmc_parents[] __initconst = {
				hosc, pll_periph0, pll_periph1
};
static const struct clk_init_data nand_init __initconst = {
	CCU_HW(nand, mmc_parents, &ccu_periph_ops),
};
static const struct ccu nand_clk __initconst = {
	CCU_REG(0x080),
	.hw.init = &nand_init,
	CCU_MUX(24, 2),
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
static const struct ccu_extra mmc_extra = {
	.mode_select.rate = 50000000,
	.mode_select.bit = 30,
};
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
	.features = CCU_FEATURE_SET_RATE_GATE |
			CCU_FEATURE_MODE_SELECT,
	.extra = &mmc_extra,
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
	.features = CCU_FEATURE_MODE_SELECT,
	.extra = &mmc_extra,
};
static const char mmc1_output[] __initconst = "mmc1-output";
static const struct clk_init_data mmc1_output_init __initconst = {
	CCU_HW(mmc1_output, child_mmc1, &ccu_phase_ops),
};
static const struct ccu mmc1_output_clk __initconst = {
	CCU_REG(0x08c),
	.hw.init = &mmc1_output_init,
	CCU_PHASE(8, 3),
	.features = CCU_FEATURE_MODE_SELECT,
	.extra = &mmc_extra,
};

static const char mmc2[] __initconst = "mmc2";
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
static const char * const child_mmc2[] __initconst = { "mmc2" };
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

static const char ts[] __initconst = "ts";
static const char * const ts_parents[] __initconst = { hosc, pll_periph0 };
static const struct clk_init_data ts_init __initconst = {
	CCU_HW(ts, ts_parents, &ccu_periph_ops),
};
static const struct ccu ts_clk __initconst = {
	CCU_REG(0x098),
	.hw.init = &ts_init,
	CCU_MUX(24, 2),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
};

static const char ce[] __initconst = "ce";
static const struct clk_init_data ce_init __initconst = {
	CCU_HW(ce, mmc_parents, &ccu_periph_ops),
};
static const struct ccu ce_clk __initconst = {
	CCU_REG(0x09c),
	.hw.init = &ce_init,
	CCU_MUX(24, 2),
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
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
};

static const char i2s0[] __initconst = "i2s0";
static const char * const i2s_parents[] __initconst = {
		pll_audio_8x, pll_audio_4x, pll_audio_2x, pll_audio
};
static const struct clk_init_data i2s0_init __initconst = {
	CCU_HW(i2s0, i2s_parents, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu i2s0_clk __initconst = {
	CCU_REG(0x0b0),
	.hw.init = &i2s0_init,
	CCU_MUX(16, 2),
	CCU_BUS(0x068, 12),
	CCU_GATE(31),
};
static const char i2s1[] __initconst = "i2s1";
static const struct clk_init_data i2s1_init __initconst = {
	CCU_HW(i2s1, i2s_parents, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu i2s1_clk __initconst = {
	CCU_REG(0x0b4),
	.hw.init = &i2s1_init,
	CCU_MUX(16, 2),
	CCU_BUS(0x068, 13),
	CCU_GATE(31),
};

static const char i2s2[] __initconst = "i2s2";
static const struct clk_init_data i2s2_init __initconst = {
	CCU_HW(i2s2, i2s_parents, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu i2s2_clk __initconst = {
	CCU_REG(0x0b8),
	.hw.init = &i2s2_init,
	CCU_MUX(16, 2),
	CCU_BUS(0x068, 14),
	CCU_GATE(31),
};

static const char spdif[] __initconst = "spdif";
static const struct clk_init_data spdif_init __initconst = {
	CCU_HW(spdif, child_pll_audio, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu spdif_clk __initconst = {
	CCU_REG(0x0c0),
	.hw.init = &spdif_init,
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

static const char usb_phy2[] __initconst = "usb-phy2";
static const struct clk_init_data usb_phy2_init __initconst = {
	CCU_HW(usb_phy2, child_hosc, &ccu_periph_ops),
};
static const struct ccu usb_phy2_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &usb_phy2_init,
	CCU_GATE(10),
};

static const char usb_phy3[] __initconst = "usb-phy3";
static const struct clk_init_data usb_phy3_init __initconst = {
	CCU_HW(usb_phy3, child_hosc, &ccu_periph_ops),
};
static const struct ccu usb_phy3_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &usb_phy3_init,
	CCU_GATE(11),
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

static const char ohci1[] __initconst = "ohci1";
static const struct clk_init_data ohci1_init __initconst = {
	CCU_HW(ohci1, child_hosc, &ccu_periph_ops),
};
static const struct ccu ohci1_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &ohci1_init,
	CCU_GATE(17),
};

static const char ohci2[] __initconst = "ohci2";
static const struct clk_init_data ohci2_init __initconst = {
	CCU_HW(ohci2, child_hosc, &ccu_periph_ops),
};
static const struct ccu ohci2_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &ohci2_init,
	CCU_GATE(18),
};

static const char ohci3[] __initconst = "ohci3";
static const struct clk_init_data ohci3_init __initconst = {
	CCU_HW(ohci3, child_hosc, &ccu_periph_ops),
};
static const struct ccu ohci3_clk __initconst = {
	CCU_REG(0x0cc),
	.hw.init = &ohci3_init,
	CCU_GATE(19),
};

static const char dram[] __initconst = "dram";
static const struct clk_init_data dram_init __initconst = {
	CCU_HW(dram, child_pll_ddr, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu dram_clk __initconst = {
	CCU_REG(0x0f4),
	.hw.init = &dram_init,
	CCU_MUX(20, 2),
	CCU_M(0, 4),
	CCU_UPD(16),
};
static const char * const child_dram[] __initconst = { dram };

static const char dram_ve[] __initconst = "dram-ve";
static const struct clk_init_data dram_ve_init __initconst = {
	CCU_HW(dram_ve, child_dram, &ccu_periph_ops),
};
static const struct ccu dram_ve_clk __initconst = {
	CCU_REG(0x0100),
	.hw.init = &dram_ve_init,
	CCU_GATE(0),
};

static const char dram_csi[] __initconst = "dram-csi";
static const struct clk_init_data dram_csi_init __initconst = {
	CCU_HW(dram_csi, child_dram, &ccu_periph_ops),
};
static const struct ccu dram_csi_clk __initconst = {
	CCU_REG(0x100),
	.hw.init = &dram_csi_init,
	CCU_GATE(1),
};

static const char dram_deinterlace[] __initconst = "dram-deinterlace";
static const struct clk_init_data dram_deinterlace_init __initconst = {
	CCU_HW(dram_deinterlace, child_dram, &ccu_periph_ops),
};
static const struct ccu dram_deinterlace_clk __initconst = {
	CCU_REG(0x0100),
	.hw.init = &dram_deinterlace_init,
	CCU_GATE(2),
};

static const char dram_ts[] __initconst = "dram-ts";
static const struct clk_init_data dram_ts_init __initconst = {
	CCU_HW(dram_ts, child_dram, &ccu_periph_ops),
};
static const struct ccu dram_ts_clk __initconst = {
	CCU_REG(0x0100),
	.hw.init = &dram_ts_init,
	CCU_GATE(3),
};

static const char * const de_parents[] __initconst = {
				pll_periph0_2x, pll_de
};
static const char de[] __initconst = "de";
static const struct clk_init_data de_init __initconst = {
	CCU_HW(de, de_parents, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu de_clk __initconst = {
	CCU_REG(0x104),
	.hw.init = &de_init,
	CCU_MUX(24, 3),
	CCU_RESET(0x2c4, 12),
	CCU_BUS(0x064, 12),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char tcon0[] __initconst = "tcon0";
static const struct clk_init_data tcon0_init __initconst = {
	CCU_HW(tcon0, child_pll_video, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu tcon0_clk __initconst = {
	CCU_REG(0x118),
	.hw.init = &tcon0_init,
	CCU_MUX(24, 3),
	CCU_RESET(0x2c4, 3),
	CCU_BUS(0x064, 3),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char tve[] __initconst = "tve";
static const char * const tve_parents[] __initconst = {
				pll_de, pll_periph1
};
static const struct clk_init_data tve_init __initconst = {
	CCU_HW(tve, tve_parents, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu tve_clk __initconst = {
	CCU_REG(0x120),
	.hw.init = &tve_init,
	CCU_MUX(24, 3),
	CCU_GATE(31),
	CCU_M(0, 4),
};

static const char deinterlace[] __initconst = "deinterlace";
static const char * const ph0_ph1[] __initconst = { pll_periph0, pll_periph1 };
static const struct clk_init_data deinterlace_init __initconst = {
	CCU_HW(deinterlace, ph0_ph1, &ccu_periph_ops),
};
static const struct ccu deinterlace_clk __initconst = {
	CCU_REG(0x124),
	.hw.init = &deinterlace_init,
	CCU_MUX(24, 3),
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

static const char csi_sclk[] __initconst = "csi-sclk";
static const struct clk_init_data csi_sclk_init __initconst = {
	CCU_HW(csi_sclk, ph0_ph1, &ccu_periph_ops),
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
				hosc, pll_video, pll_periph0
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

static const char ac_dig[] __initconst = "ac-dig";
static const struct clk_init_data ac_dig_init __initconst = {
	CCU_HW(ac_dig, child_pll_audio, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu ac_dig_clk __initconst = {
	CCU_REG(0x0140),
	.hw.init = &ac_dig_init,
	CCU_GATE(31),
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
	CCU_HW(hdmi, child_pll_video, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu hdmi_clk __initconst = {
	CCU_REG(0x150),
	.hw.init = &hdmi_init,
	CCU_MUX(24, 2),
	CCU_GATE(31),
	CCU_BUS(0x064, 11),
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
				hosc, pll_periph0_2x, pll_ddr
};
static const struct clk_init_data mbus_init __initconst = {
	CCU_HW(mbus, mbus_parents, &ccu_periph_ops),
	.flags = CLK_IS_CRITICAL,
};
static const struct ccu mbus_clk __initconst = {
	CCU_REG(0x15c),
	.hw.init = &mbus_init,
	CCU_GATE(31),
	CCU_MUX(24, 2),
	CCU_M(0, 3),
};

static const char gpu[] __initconst = "gpu";
static const struct clk_init_data gpu_init __initconst = {
	CCU_HW(gpu, child_pll_gpu, &ccu_periph_ops),
	.flags = CLK_SET_RATE_PARENT,
};
static const struct ccu gpu_clk __initconst = {
	CCU_REG(0x1a0),
	.hw.init = &gpu_init,
	CCU_GATE(31),
	CCU_M(0, 3),
};

/* ---- PRCM ---- */
static const char * const cpus_parents[] __initconst = {
			losc, hosc, pll_periph0, ""	/* ?? */
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

static const char * const r_cir_parents[] __initconst = { losc, hosc };
static const char r_cir[] __initconst = "r_cir";
static const struct clk_init_data r_cir_init __initconst = {
	CCU_HW(r_cir, r_cir_parents, &ccu_periph_ops),
};
static const struct ccu r_cir_clk __initconst = {
	CCU_REG1(0x054),
	.hw.init = &r_cir_init,
	CCU_MUX(24, 2),
	CCU_GATE(31),
	CCU_M(0, 4),
	CCU_P(16, 2),
};

/* system clocks and resets */
static const struct ccu * const sun8i_h3_ccu_clks[] __initconst = {
	[CLK_BUS_DMA]		= &bus_dma_clk,
	[CLK_BUS_EHCI0]		= &bus_ehci0_clk,
	[CLK_BUS_EHCI1]		= &bus_ehci1_clk,
	[CLK_BUS_EHCI2]		= &bus_ehci2_clk,
	[CLK_BUS_EHCI3]		= &bus_ehci3_clk,
	[CLK_BUS_MMC0]		= &bus_mmc0_clk,
	[CLK_BUS_MMC1]		= &bus_mmc1_clk,
	[CLK_BUS_MMC2]		= &bus_mmc2_clk,
	[CLK_BUS_OHCI0]		= &bus_ohci0_clk,
	[CLK_BUS_OHCI1]		= &bus_ohci1_clk,
	[CLK_BUS_OHCI2]		= &bus_ohci2_clk,
	[CLK_BUS_OHCI3]		= &bus_ohci3_clk,
	[CLK_BUS_PIO]		= &bus_pio_clk,
	[CLK_BUS_UART0]		= &bus_uart0_clk,
	[CLK_BUS_UART1]		= &bus_uart1_clk,
	[CLK_BUS_UART2]		= &bus_uart2_clk,
	[CLK_BUS_UART3]		= &bus_uart3_clk,
	[CLK_DE]		= &de_clk,
	[CLK_HDMI]		= &hdmi_clk,
	[CLK_HDMI_DDC]		= &hdmi_ddc_clk,
	[CLK_I2S0]		= &i2s0_clk,
	[CLK_I2S1]		= &i2s1_clk,
	[CLK_I2S2]		= &i2s2_clk,
	[CLK_MMC0]		= &mmc0_clk,
	[CLK_MMC0_SAMPLE]	= &mmc0_sample_clk,
	[CLK_MMC0_OUTPUT]	= &mmc0_output_clk,
	[CLK_MMC1]		= &mmc1_clk,
	[CLK_MMC1_SAMPLE]	= &mmc1_sample_clk,
	[CLK_MMC1_OUTPUT]	= &mmc1_output_clk,
	[CLK_MMC2]		= &mmc2_clk,
	[CLK_MMC2_SAMPLE]	= &mmc2_sample_clk,
	[CLK_MMC2_OUTPUT]	= &mmc2_output_clk,
	[CLK_PLL_AUDIO]		= &pll_audio_clk,
	[CLK_PLL_DE]		= &pll_de_clk,
	[CLK_PLL_GPU]		= &pll_gpu_clk,
	[CLK_PLL_PERIPH0]	= &pll_periph0_clk,
	[CLK_PLL_PERIPH1]	= &pll_periph1_clk,
	[CLK_PLL_VE]		= &pll_ve_clk,
	[CLK_PLL_VIDEO]		= &pll_video_clk,
	[CLK_SPDIF]		= &spdif_clk,
	[CLK_SPI0]		= &spi0_clk,
	[CLK_SPI1]		= &spi1_clk,
	[CLK_TCON0]		= &tcon0_clk,
	[CLK_THS]		= &ths_clk,
	[CLK_TVE]		= &tve_clk,
	[CLK_USB_OHCI0]		= &ohci0_clk,
	[CLK_USB_OHCI1]		= &ohci1_clk,
	[CLK_USB_OHCI2]		= &ohci2_clk,
	[CLK_USB_OHCI3]		= &ohci3_clk,
	[CLK_USB_PHY0]		= &usb_phy0_clk,
	[CLK_USB_PHY1]		= &usb_phy1_clk,
	[CLK_USB_PHY2]		= &usb_phy2_clk,
	[CLK_USB_PHY3]		= &usb_phy3_clk,
	[CLK_VE]		= &ve_clk,
	[CLK_BUS_R_PIO]		= &bus_r_pio_clk,
	[CLK_BUS_R_CIR]		= &bus_r_cir_clk,
	[CLK_R_CIR]		= &r_cir_clk,
	[CLK_PLL_CPUX]		= &pll_cpux_clk,
	[CLK_BUS_EMAC]		= &bus_emac_clk,
	[CLK_BUS_EPHY]		= &bus_ephy_clk,
	[CLK_BUS_I2C0]		= &bus_i2c0_clk,
	[CLK_BUS_I2C1]		= &bus_i2c1_clk,
	[CLK_BUS_I2C2]		= &bus_i2c2_clk,
	&pll_audio_2x_clk,
	&pll_audio_4x_clk,
	&pll_audio_8x_clk,
	&pll_periph0_2x_clk,
	&pll_ddr_clk,
	&cpux_clk,
	&axi_clk,
	&ahb1_clk,
	&apb1_clk,
	&apb2_clk,
	&ahb2_clk,
	&bus_ce_clk,
	&bus_nand_clk,
	&bus_dram_clk,
	&bus_ts_clk,
	&bus_hstimer_clk,
	&bus_spi0_clk,
	&bus_spi1_clk,
	&bus_otg_clk,
	&bus_ve_clk,
	&bus_deinterlace_clk,
	&bus_csi_clk,
	&bus_tve_clk,
	&bus_gpu_clk,
	&bus_msgbox_clk,
	&bus_spinlock_clk,
	&bus_codec_clk,
	&bus_spdif_clk,
	&bus_scr_clk,
	&bus_dbg_clk,
	&nand_clk,
	&ts_clk,
	&ce_clk,
	&dram_clk,
	&dram_ve_clk,
	&dram_csi_clk,
	&dram_deinterlace_clk,
	&dram_ts_clk,
	&deinterlace_clk,
	&csi_misc_clk,
	&csi_sclk_clk,
	&csi_mclk_clk,
	&ac_dig_clk,
	&avs_clk,
	&mbus_clk,
	&gpu_clk,
	&cpus_clk,
	&apb0_clk,
};

static const struct ccu_reset_map sun8i_h3_ccu_resets[] __initconst = {
	[RST_USB_PHY0]	= { 0x0cc, 0 },
	[RST_USB_PHY1]	= { 0x0cc, 1 },
	[RST_USB_PHY2]	= { 0x0cc, 2 },
	[RST_USB_PHY3]	= { 0x0cc, 3 },

	[RST_MBUS]	= { 0x0fc, 31 },

	[RST_BUS_CE]	= { 0x2c0, 5 },
	[RST_BUS_DMA]	= { 0x2c0, 6 },
//	[RST_BUS_MMC0]	= { 0x2c0, 8 },
//	[RST_BUS_MMC1]	= { 0x2c0, 9 },
//	[RST_BUS_MMC2]	= { 0x2c0, 10 },
//	[RST_BUS_DE]	= { 0x2c0, 12 },
	[RST_BUS_NAND]	= { 0x2c0, 13 },
	[RST_BUS_DRAM]	= { 0x2c0, 14 },
	[RST_BUS_EMAC]	= { 0x2c0, 17 },
	[RST_BUS_TS]	= { 0x2c0, 18 },
	[RST_BUS_HSTIMER]	= { 0x2c0, 19 },
	[RST_BUS_SPI0]	= { 0x2c0, 20 },
	[RST_BUS_SPI1]	= { 0x2c0, 21 },
	[RST_BUS_OTG]	= { 0x2c0, 23 },
	[RST_BUS_EHCI0]	= { 0x2c0, 24 },
	[RST_BUS_EHCI1]	= { 0x2c0, 25 },
	[RST_BUS_EHCI2]	= { 0x2c0, 26 },
	[RST_BUS_EHCI3]	= { 0x2c0, 27 },
	[RST_BUS_OHCI0]	= { 0x2c0, 28 },
	[RST_BUS_OHCI1]	= { 0x2c0, 29 },
	[RST_BUS_OHCI2]	= { 0x2c0, 30 },
	[RST_BUS_OHCI3]	= { 0x2c0, 31 },

	[RST_BUS_VE]	= { 0x2c4, 0 },
//	[RST_BUS_TCON0]	= { 0x2c4, 3 },
//	[RST_BUS_TCON1]	= { 0x2c4, 4 },
	[RST_BUS_DEINTERLACE] = { 0x2c4, 5 },
	[RST_BUS_CSI]	= { 0x2c4, 8 },
	[RST_BUS_TVE]	= { 0x2c4, 9 },
	[RST_BUS_HDMI0]	= { 0x2c4, 10 },
	[RST_BUS_HDMI1]	= { 0x2c4, 11 },
	[RST_BUS_GPU]	= { 0x2c4, 20 },
	[RST_BUS_MSGBOX]	= { 0x2c4, 21 },
	[RST_BUS_SPINLOCK]	= { 0x2c4, 22 },
	[RST_BUS_DBG]	= { 0x2c4, 31 },

	[RST_BUS_EPHY]	= { 0x2c8, 2 },

	[RST_BUS_CODEC]	= { 0x2d0, 0 },
	[RST_BUS_SPDIF]	= { 0x2d0, 1 },
	[RST_BUS_THS]	= { 0x2d0, 8 },
	[RST_BUS_I2S0]	= { 0x2d0, 12 },
	[RST_BUS_I2S1]	= { 0x2d0, 13 },
	[RST_BUS_I2S2]	= { 0x2d0, 14 },

	[RST_BUS_I2C0]	= { 0x2d8, 0 },
	[RST_BUS_I2C1]	= { 0x2d8, 1 },
	[RST_BUS_I2C2]	= { 0x2d8, 2 },
	[RST_BUS_UART0]	= { 0x2d8, 16 },
	[RST_BUS_UART1]	= { 0x2d8, 17 },
	[RST_BUS_UART2]	= { 0x2d8, 18 },
	[RST_BUS_UART3]	= { 0x2d8, 19 },
	[RST_BUS_SCR]	= { 0x2d8, 20 },

	[RST_R_PIO]	= { 0x0b0, 0, 1 },
	[RST_R_CIR]	= { 0x0b0, 1, 1 },
};

static void __init sun8i_h3_ccu_setup(struct device_node *node)
{
	ccu_probe(node, sun8i_h3_ccu_clks,
			ARRAY_SIZE(sun8i_h3_ccu_clks),
			sun8i_h3_ccu_resets,
			ARRAY_SIZE(sun8i_h3_ccu_resets));
}
CLK_OF_DECLARE(sun8i_h3_ccu, "allwinner,sun8i-h3-ccu",
	       sun8i_h3_ccu_setup);
