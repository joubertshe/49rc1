/*
 * Copyright (C) 2016 Jean-Francois Moine <moinejf@free.fr>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _CCU_H_
#define _CCU_H_

struct device_node;

#define CCU_HW(_name, _parents, _ops)		\
	.name		= _name,		\
	.parent_names	= _parents,		\
	.num_parents	= ARRAY_SIZE(_parents),	\
	.ops 		= _ops

#define CCU_REG(_reg) .reg = _reg
#define CCU_REG1(_reg) .reg = _reg, .index = 1
#define CCU_RESET(_reg, _bit) .reset_reg = _reg, .reset_bit = _bit
#define CCU_BUS(_reg, _bit) .bus_reg = _reg, .bus_bit = _bit
#define CCU_GATE(_bit) .has_gate = 1, .gate_bit = _bit
#define CCU_LOCK(_reg, _bit) .lock_reg = _reg, .lock_bit = _bit
#define CCU_MUX(_shift, _width) .mux_shift = _shift, .mux_width = _width
#define CCU_N(_shift, _width) .n_shift = _shift, .n_width = _width
#define CCU_D1(_shift, _width) .d1_shift = _shift, .d1_width = _width
#define CCU_D2(_shift, _width) .p_shift = _shift, .p_width = _width
#define CCU_K(_shift, _width) .k_shift = _shift, .k_width = _width
#define CCU_M(_shift, _width) .m_shift = _shift, .m_width = _width
#define CCU_P(_shift, _width) .p_shift = _shift, .p_width = _width
#define CCU_UPD(_bit) .upd_bit = _bit
/* with ccu_fixed_factor_ops */
#define CCU_FIXED(_mul, _div) .n_width = _mul, .m_width = _div
/* with ccu_phase_ops */
#define CCU_PHASE(_shift, _width) .p_shift = _shift, .p_width = _width

#define CCU_FEATURE_MUX_VARIABLE_PREDIV	BIT(0)
#define CCU_FEATURE_MUX_FIXED_PREDIV	BIT(1)
#define CCU_FEATURE_FIXED_POSTDIV	BIT(2)
#define CCU_FEATURE_N0			BIT(3)
#define CCU_FEATURE_MODE_SELECT		BIT(4)
#define CCU_FEATURE_FLAT_FACTORS	BIT(5)
#define CCU_FEATURE_SET_RATE_GATE	BIT(6)
#define CCU_FEATURE_M_TABLE		BIT(7)
#define CCU_FEATURE_SET_RATE_PARENT	BIT(8)

/* extra */
#define CCU_EXTRA_FRAC(_frac) .frac = _frac, .num_frac = ARRAY_SIZE(_frac)
#define CCU_EXTRA_POST_DIV(_div) .fixed_div[0] = _div

/* fractional and Sigma-Delta items */
struct frac {
	unsigned long rate;
	u32 mask;
	u32 val;
	u32 sd_reg;
	u32 sd_val;
};

/* extra features */
struct ccu_extra {
	const struct frac *frac; /* array - last is the fractional mask/value */
	u8 num_frac;

	u16 fixed_div[4];		/* index = parent */

	struct {
		u8 index;
		u8 shift;
		u8 width;
	} variable_prediv;

	struct {
		unsigned long rate;
		u8 bit;
	} mode_select;

	u8 m_table[8];
};

struct ccu {
	struct clk_hw hw;

	u16 reg;

	u16 reset_reg, bus_reg, lock_reg;
	u8  reset_bit, bus_bit, lock_bit;
	u8 has_gate, gate_bit;
	u8 index;			/* CCU = 0, PRCM = 1 */

	u8 mux_shift, mux_width;
	u8 n_shift, n_width, n_min;
	u8 d1_shift, d1_width;
//	u8 d2_shift, d2_width;		/* d2 is p (never d2 + p) */
	u8 k_shift, k_width;
	u8 m_shift, m_width;
	u8 p_shift, p_width;

	u8 upd_bit;

	u16 features;

	const struct ccu_extra *extra;
};

struct ccu_reset_map {
	u16	reg;
	u8	bit;
	u8	index;			/* CCU = 0, PRCM = 1 */
};

extern const struct clk_ops ccu_fixed_factor_ops;
extern const struct clk_ops ccu_periph_ops;
extern const struct clk_ops ccu_pll_ops;
extern const struct clk_ops ccu_phase_ops;

int ccu_probe(struct device_node *node,
		const struct ccu * const ccu_clks[],
		int nr_clks,
		const struct ccu_reset_map *reset_map,
		int nr_resets);

#endif /* _CCU_H_ */
