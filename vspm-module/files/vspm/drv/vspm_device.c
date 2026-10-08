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

#include "vspm_main.h"
#include "vspm_log.h"

#define VSPM_DEVICE_ISU_COMMON_CONFIG										\
	.clk_names = { "isu_aclk", "isu_pclk" },								\
	.rst_names = { "aresetn", "presetn" }

/* RZ/G2 Series */
const struct vspm_device_config r8a774_vsp_config =
{
	.clk_names = { "vsp", "fcp" },
	.rst_names = { "vsp", "fcp" },
};

const struct vspm_device_config r8a774_fdp_config =
{
	.clk_names = { "fdp", "fcp" },
	.rst_names = { "fdp", "fcp" },
};

/* RZ/G3E */
const struct vspm_device_config r9a09g047_vsp_config =
{
	.clk_names = { "vsp_mclk", "vsp_aclk", "vsp_pclk" },
	.rst_names = { "smreset_vspiss", "sreset_vsp1z", "spreset_fcpvi0" },
	.ops = &r9a09g047_vsp_ops,
};

const struct vspm_device_config r9a09g047_fdp_config =
{
	.clk_names = { "fdp_mclk", "fdp_aclk", "fdp_pclk" },
	.rst_names = { "brdg_resetn", "fdp1_resetn", "fcpf1_resetp" },
	.ops = &r9a09g047_fdp_ops,
};

/* RZ/G2L Series and RZ/V2L */
const struct vspm_device_config r9a07g044_isu_config =
{
	VSPM_DEVICE_ISU_COMMON_CONFIG,
	.hw_prv_info = {
		.isu = {
			.min = {
				.width   = 1,
				.height  = 1,
				.crop_w  = 1,
				.crop_h  = 1,
				.x_scale = 0x1000,
				.y_scale = 0x1000,
			},
			.max = {
				.width   = 2800,
				.height  = 2047,
				.crop_w  = 1920,
				.crop_h  = 1080,
				.x_scale = 0xFFFF,
				.y_scale = 0xFFFF,
			},
		},
	},
};

/* RZ/G3L */
const struct vspm_device_config r9a08g046_isu_config =
{
	VSPM_DEVICE_ISU_COMMON_CONFIG,
	.hw_prv_info = {
		.isu = {
			.features = ISU_FM_INT_EDGE | ISU_RAW_FORMAT_EXTS,
			.min = {
				.width   = 1,
				.height  = 1,
				.crop_w  = 1,
				.crop_h  = 1,
				.x_scale = 0x800,
				.y_scale = 0x800,
			},
			.max = {
				.width   = 2048,
				.height  = 2048,
				.crop_w  = 2048,
				.crop_h  = 2048,
				.x_scale = 0xFFFF,
				.y_scale = 0xFFFF,
			},
		},
	},
};

/* RZ/V2H and RZ/V2N */
const struct vspm_device_config r9a09g056_isu_config =
{
	VSPM_DEVICE_ISU_COMMON_CONFIG,
	.hw_prv_info = {
		.isu = {
			.features = ISU_FM_INT_EDGE | ISU_RAW_FORMAT_EXTS,
			.min = {
				.width   = 1,
				.height  = 1,
				.crop_w  = 1,
				.crop_h  = 1,
				.x_scale = 0x1000,
				.y_scale = 0x1000,
			},
			.max = {
				.width   = 4096,
				.height  = 4096,
				.crop_w  = 4096,
				.crop_h  = 4096,
				.x_scale = 0xFFFF,
				.y_scale = 0xFFFF,
			},
		},
	},
};

const struct soc_device_attribute vsp_config_attrs[] = {
	{ .soc_id = "r8a774*"  , .data = &r8a774_vsp_config },
	{ .soc_id = "r9a09g047", .data = &r9a09g047_vsp_config },
	{ /* sentinel */ },
};

const struct soc_device_attribute fdp_config_attrs[] = {
	{ .soc_id = "r8a774*"  , .data = &r8a774_fdp_config },
	{ .soc_id = "r9a09g047", .data = &r9a09g047_fdp_config },
	{ /* sentinel */ },
};

const struct soc_device_attribute isu_config_attrs[] = {
	{ .soc_id = "r9a07g044", .data = &r9a07g044_isu_config },
	{ .soc_id = "r9a07g054", .data = &r9a07g044_isu_config },
	{ .soc_id = "r9a08g046", .data = &r9a08g046_isu_config },
	{ .soc_id = "r9a09g056", .data = &r9a09g056_isu_config },
	{ .soc_id = "r9a09g057", .data = &r9a09g056_isu_config },
	{ /* sentinel */ },
};

/******************************************************************************
 * Function:		vspm_device_get_clks
 * Description:	Get clock array for device
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
int vspm_device_get_clks(struct platform_device *pdev, struct vspm_device *vdev)
{
	struct clk *clk = NULL;
	int i = 0;

	if (!vdev || !vdev->config)
		goto err_exit;

	for (i = 0; i < VSPM_CLKNUM_MAX; i++) {
		if (!vdev->config->clk_names[i])
			continue;

		clk = devm_clk_get(&pdev->dev, vdev->config->clk_names[i]);
		if (IS_ERR(clk)) {
			APRINT("Cannot get %s clock!!\n", vdev->config->clk_names[i]);
			goto err_exit;
		}

		vdev->clks[vdev->num_clks] = clk;
		vdev->num_clks++;
	}

	return R_VSPM_OK;

err_exit:
	for (i = vdev->num_clks; i > 0; i--)
		vdev->clks[i - 1] = NULL;

	vdev->num_clks = 0;

	return R_VSPM_NG;
}

/******************************************************************************
 * Function:		vspm_device_get_rsts
 * Description:	Get reset array for device
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
int vspm_device_get_rsts(struct platform_device *pdev, struct vspm_device *vdev)
{
	struct reset_control *rstc = NULL;
	int i = 0;

	if (!vdev || !vdev->config)
		goto err_exit;

	for (i = 0; i < VSPM_RSTNUM_MAX; i++) {
		if (!vdev->config->rst_names[i])
			continue;

		rstc = devm_reset_control_get(&pdev->dev, vdev->config->rst_names[i]);
		if (IS_ERR(rstc)) {
			APRINT("Failed to get cpg rsts (%s)!!\n", vdev->config->rst_names[i]);
			goto err_exit;
		}

		vdev->rsts[vdev->num_rsts] = rstc;
		vdev->num_rsts++;
	}

	return R_VSPM_OK;

err_exit:
	for (i = vdev->num_rsts; i > 0; i--)
		vdev->rsts[i - 1] = NULL;

	vdev->num_rsts = 0;

	return R_VSPM_NG;
}

