/*************************************************************************/ /*
 * VSPM
 *
 * Copyright (C) 2026 Renesas Electronics Corporation
 *
 * License        Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * GPLv2:
 * If you wish to use this file under the terms of GPL, following terms are
 * effective.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */ /*************************************************************************/

#ifndef __VSPM_DEVICE_H__
#define __VSPM_DEVICE_H__

#include <linux/platform_device.h>
#include <linux/sys_soc.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regmap.h>

#define VSPM_CLKNUM_MAX			3
#define VSPM_RSTNUM_MAX			3

/* Indicator for require features on devices ops */
#define VSPM_DT_SYSCON_CPG		(BIT(0))
#define VSPM_DT_SYSCON_SYSC		(BIT(1))

/* Indicator for features on different SoC */
#define ISU_FM_INT_EDGE			(BIT(0))
#define ISU_RAW_FORMAT_EXTS		(BIT(1))

struct vsp_hw_prv_info {
	/* Reserved */
};

struct fdp_hw_prv_info {
	/* Reserved */
};

/* ISU parameter specific limit information */
struct isu_param_limit {
	unsigned short width;
	unsigned short height;
	unsigned short crop_w;
	unsigned short crop_h;
	unsigned int x_scale;
	unsigned int y_scale;
};

struct isu_hw_prv_info {
	unsigned short features;
	struct isu_param_limit min;
	struct isu_param_limit max;
};

struct vspm_device_ops {
	int (*power_on)(struct device *dev);
	int (*power_off)(struct device *dev);
	unsigned short require_features;
};

struct vspm_device_config {
	const char *clk_names[VSPM_CLKNUM_MAX];
	const char *rst_names[VSPM_CLKNUM_MAX];
	union {
		struct vsp_hw_prv_info vsp;
		struct fdp_hw_prv_info fdp;
		struct isu_hw_prv_info isu;
	} hw_prv_info;
	const struct vspm_device_ops *ops;
};

struct vspm_device {
	struct clk *clks[VSPM_CLKNUM_MAX];
	struct reset_control *rsts[VSPM_RSTNUM_MAX];
	int num_clks;
	int num_rsts;
	const struct vspm_device_config *config;
	struct regmap *syscon_cpg;
	struct regmap *syscon_sysc;
};

int vspm_device_get_clks(struct platform_device *pdev, struct vspm_device *vdev);
int vspm_device_get_rsts(struct platform_device *pdev, struct vspm_device *vdev);

extern const struct soc_device_attribute vsp_config_attrs[];
extern const struct soc_device_attribute fdp_config_attrs[];
extern const struct soc_device_attribute isu_config_attrs[];

extern const struct vspm_device_ops r9a09g047_vsp_ops;
extern const struct vspm_device_ops r9a09g047_fdp_ops;

#endif /* __VSPM_DEVICE_H__ */
