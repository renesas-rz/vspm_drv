/*************************************************************************/ /*
 * R9A09G047 Device Specification
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

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include "vspm_main.h"
#include "vspm_log.h"

#define STATUS_LOOP_TIME			(2)
#define STATUS_LOOP_CNT				(500)

/* define low-power-consumption sequence control register offset for FDP */
#define CPG_LP_FDP1_CTL				(0x0CA4)
#define CPG_LP_FDP1_PWRREQ			(BIT(16) | BIT(28))
#define CPG_LP_FDP1_PWRACC			(BIT(17) | BIT(29))
#define CPG_LP_FDP1_CLKREQ_S		(BIT(0) | BIT(8) | BIT(20))
#define CPG_LP_FDP1_CLKREQ_M		(BIT(4) | BIT(12) | BIT(24))
#define CPG_LP_FDP1_CLKREQ			(CPG_LP_FDP1_CLKREQ_S | CPG_LP_FDP1_CLKREQ_M)
#define CPG_LP_FDP1_CLKACC_S		(BIT(1) | BIT(9) | BIT(21))
#define CPG_LP_FDP1_CLKACC_M		(BIT(5) | BIT(13) | BIT(25))
#define CPG_LP_FDP1_CLKACC			(CPG_LP_FDP1_CLKACC_S | CPG_LP_FDP1_CLKACC_M)


/* define low-power-comsumption requence control register offset for VSP */
#define CPG_LP_VSP_CTL1				(0x0C9C)
#define CPG_LP_VSP_CTL2				(0x0CA0)
#define CPG_VSP_LPC1_CLKQREQ_S		(BIT(0) | BIT(8) | BIT(24))
#define CPG_VSP_LPC1_CLKQREQ_M		(BIT(4) | BIT(12) | BIT(20))
#define CPG_VSP_LPC1_PWRDQREQ		(BIT(16))
#define CPG_VSP_LPC1_CLKQREQ		(CPG_VSP_LPC1_CLKQREQ_S | CPG_VSP_LPC1_CLKQREQ_M)
#define CPG_VSP_LPC1_CLKACC_S		(BIT(1))
#define CPG_VSP_LPC1_CLKACC_M		(BIT(5))
#define CPG_VSP_LPC1_PWRDACC		(BIT(17))
#define CPG_VSP_LPC1_CLKACC			(CPG_VSP_LPC1_CLKACC_S | CPG_VSP_LPC1_CLKACC_M)
#define CPG_VSP_LPC2_PWRDQREQ		(BIT(0))
#define CPG_VSP_LPC2_PWRDACC		(BIT(1))

/* define SYSC offset */
#define SYS_VSPI_CLK				(0x0E00)
#define SYS_VSP_CLK_MSTP			(0x00000002)
#define SYS_VSP_CLK_MON				(0x00000001)
#define SYS_VSP_CLK_ALL				(SYS_VSP_CLK_MSTP | SYS_VSP_CLK_MON)


static int syscon_set_sync(struct regmap *syscon, unsigned int offset, unsigned int value, unsigned int check, int mask)
{
	unsigned int	reg_data	= 0;
	unsigned int	loopcnt		= 0;

	if (!syscon)
		return -EPIPE;

	if (regmap_write(syscon, offset, value)) {
		EPRINT("%s: failed to write to syscon\n", __func__);
		return -EAGAIN;
	}

	if (!mask)
		return 0;

	/* confirm Q-ch for all power supply control has transitioned */
	loopcnt = STATUS_LOOP_CNT;
	do {
		if (regmap_read(syscon, offset, &reg_data)) {
			EPRINT("%s: read error \n",__func__);
			return -EAGAIN;
		}
		msleep(STATUS_LOOP_TIME);
	} while (((reg_data & mask) != (check & mask)) &&
			(--loopcnt > 0));

	if (loopcnt == 0) {
		EPRINT("%s: timeout waiting for power supply control transition to 0x%x (cur:0x%x, set:0x%x, mask:0x%x)" \
				"\n", __func__, check, reg_data, value, mask);
		return -EAGAIN;
	}
	return 0;
}

static int clock_disable(struct vspm_device *vdev)
{
	int	i	= 0;

	if (!vdev || !vdev->config)
		return -EINVAL;

	for (i = (vdev->num_clks - 1); i >= 0 ; i--) {
		if (vdev->clks[i]) {
			clk_disable_unprepare(vdev->clks[i]);
		}
	}
	return 0;
}

static int clock_enable(struct vspm_device *vdev)
{
	int	i	= 0;
	int	ret	= -EINVAL;

	if (!vdev || !vdev->config)
		return ret;

	for (i = 0 ; i < vdev->num_clks ; i++) {
		if (vdev->clks[i]) {
			ret = clk_prepare_enable(vdev->clks[i]);
			if (ret) {
				EPRINT("%s: Fail to enable clock %d \n", __func__, i);
				return ret;
			}
		}
	}
	return ret;
}

static int reset_assert(struct vspm_device *vdev)
{
	int	i		= 0;
	int	loopcnt	= 0;

	if (!vdev || !vdev->config)
		return -EINVAL;

	for (i = (vdev->num_rsts - 1); i >= 0 ; i--) {
		if (reset_control_assert(vdev->rsts[i])) {
			EPRINT("%s: failed to assert resets!! \n", __func__);
			return -EAGAIN;
		}
		/* waiting assert process */
		loopcnt = STATUS_LOOP_CNT;
		do {
			/* sleep */
			msleep(STATUS_LOOP_TIME);
		} while ((!reset_control_status(vdev->rsts[i])) &&
				(--loopcnt > 0));

		if (loopcnt == 0) {
			EPRINT("%s: happen to timeout after assert resets!! \n", __func__);
			return -EAGAIN;
		}
	}
	return 0;
}

static int reset_deassert(struct vspm_device *vdev)
{
	int	i		= 0;
	int	loopcnt	= 0;

	if (!vdev || !vdev->config)
		return -EINVAL;

	for (i = 0; i < vdev->num_rsts ; i++) {
		if (reset_control_deassert(vdev->rsts[i])) {
			EPRINT("%s: failed to deassert resets!! \n", __func__);
			return -EAGAIN;
		}
		/* waiting deassert process */
		loopcnt = STATUS_LOOP_CNT;
		do {
			/* sleep */
			msleep(STATUS_LOOP_TIME);
		} while ((reset_control_status(vdev->rsts[i])) &&
				(--loopcnt > 0));

		if (loopcnt == 0) {
			EPRINT("%s: happen to timeout after deassert resets!! \n", __func__);
			return -EAGAIN;
		}
	}
	return 0;
}

static int r9a09g047_fdp_power_on(struct device *dev)
{
	struct platform_device	*pdev	= to_platform_device(dev);
	struct vspm_device		*vdev	= platform_get_drvdata(pdev);
	int						ercd 	= -EAGAIN;

	if (!vdev || !vdev->syscon_cpg)
		return -EINVAL;

	/* A2 - A3. Supply all clocks */
	ercd = clock_enable(vdev);
	if (ercd) {
		EPRINT("%s: Cannot enable clock: %d \n",__func__, ercd);
		return ercd;
	}

	/* W/A: Deassert resets should be done before supplying clock */
	/* A5 - A6 - A7 - A8. Deassert resets */
	ercd = reset_deassert(vdev);
	if (ercd) {
		EPRINT("%s: Cannot deassert resets \n",__func__);
		goto err1;
	}

	/* A4. Release lower power state */
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_FDP1_CTL,
							CPG_LP_FDP1_PWRREQ | CPG_LP_FDP1_CLKREQ,
							CPG_LP_FDP1_PWRREQ | CPG_LP_FDP1_CLKREQ,
							CPG_LP_FDP1_PWRREQ | CPG_LP_FDP1_CLKREQ /* W/A: Not recheck the acc bit */
							);
	if (ercd) {
		EPRINT("%s: Cannot release lower power state: %d\n",__func__, ercd);
		goto err2;
	}

	return ercd;
err2:
	reset_assert(vdev);
err1:
	clock_disable(vdev);
	return ercd;
}

static int r9a09g047_fdp_power_off(struct device *dev)
{
	struct platform_device	*pdev	= to_platform_device(dev);
	struct vspm_device		*vdev	= platform_get_drvdata(pdev);
	int						ercd	= -EAGAIN;

	if (!vdev || !vdev->syscon_cpg)
		return -EINVAL;

	/* B1 - B2. Make lower power state */
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_FDP1_CTL,
							(unsigned int)(~(CPG_LP_FDP1_PWRREQ | CPG_LP_FDP1_PWRACC)) & (CPG_LP_FDP1_CLKREQ | CPG_LP_FDP1_CLKACC),
							0, /* Only check the Power Request/Acc bit */
							CPG_LP_FDP1_PWRREQ | CPG_LP_FDP1_PWRACC	/* Check supply request/acc bit as document said */
							);
	if (ercd) {
		EPRINT("%s: Cannot make lower power state: %d \n", __func__, ercd);
		return ercd;
	}
	/* B3 - B4 - B5 - B6. Assert resets */
	ercd = reset_assert(vdev);
	if (ercd) {
		EPRINT("%s: Cannot assert reset: %d \n",__func__, ercd);
		goto err1;
	}
	/* Disable clock */
	ercd = clock_disable(vdev);
	if (ercd) {
		EPRINT("%s: Cannot disable clock: %d \n",__func__, ercd);
		goto err2;
	}
	return 0;
err2:
	reset_deassert(vdev);
err1:
	syscon_set_sync(vdev->syscon_cpg, CPG_LP_FDP1_CTL,
					CPG_LP_FDP1_PWRREQ | CPG_LP_FDP1_CLKREQ,
					CPG_LP_FDP1_PWRREQ, 0);
	return ercd;
}

static int vsp_enable_clk_divider(struct vspm_device *vdev)
{
	return syscon_set_sync(vdev->syscon_sysc, SYS_VSPI_CLK, SYS_VSP_CLK_MSTP,
							SYS_VSP_CLK_MON, SYS_VSP_CLK_MON);
}

static int vsp_disable_clk_divider(struct vspm_device *vdev)
{
	unsigned int	reg_data	= 0;

	regmap_read(vdev->syscon_sysc, SYS_VSPI_CLK, &reg_data);
	if (reg_data & SYS_VSP_CLK_ALL) {
		reg_data &= ~SYS_VSP_CLK_MSTP;
		return syscon_set_sync(vdev->syscon_sysc, SYS_VSPI_CLK, reg_data,
								SYS_VSP_CLK_MON, SYS_VSP_CLK_MON);
	}
	return 0;
}

static int vsp_reset_assert(struct vspm_device *vdev)
{
	int	i		= 0;
	int	loopcnt	= 0;

	if (!vdev)
		return -EINVAL;

	for (i = (vdev->num_rsts - 1); i > 0; i--) {
		if (reset_control_assert(vdev->rsts[i])) {
			EPRINT("%s: failed to assert resets!! \n", __func__);
			return -EAGAIN;
		}
		/* waiting assert process */
		loopcnt = STATUS_LOOP_CNT;
		do {
			/* sleep */
			msleep(STATUS_LOOP_TIME);
		} while ((!reset_control_status(vdev->rsts[i])) &&
				(--loopcnt > 0));

		if (loopcnt == 0) {
			EPRINT("%s: happen to timeout after assert resets!! \n", __func__);
			return -EAGAIN;
		}
		if (!strcmp(vdev->config->rst_names[i], "smreset_vspiss")) {
			/* disable clock divider after asserting vsp1z and fcpvi0 resets */
			if (vsp_disable_clk_divider(vdev))
				return -EAGAIN;
		}
	}
	return 0;
}

static int vsp_reset_deassert(struct vspm_device *vdev)
{
	int	i		= 0;
	int	loopcnt	= 0;

	if (!vdev)
		return -EINVAL;

	for (i = 0; i < vdev->num_rsts; i++) {
		if (reset_control_deassert(vdev->rsts[i])) {
			EPRINT("%s: failed to deassert resets!! \n", __func__);
			return -EAGAIN;
		}
		/* waiting deassert process */
		loopcnt = STATUS_LOOP_CNT;
		do {
			/* sleep */
			msleep(STATUS_LOOP_TIME);
		} while ((reset_control_status(vdev->rsts[i])) &&
				(--loopcnt > 0));
		if (loopcnt == 0) {
			EPRINT("%s: happen to timeout after deassert resets!! \n", __func__);
			return -EAGAIN;
		}
		if (!strcmp(vdev->config->rst_names[i], "smreset_vspiss")) {
			/* enable clock divider after deasserting vsp1z and fcpvi0 resets */
			if (vsp_enable_clk_divider(vdev))
				return -EAGAIN;
		}
	}
	return 0;
}

static int r9a09g047_vsp_power_on(struct device *dev)
{
	struct platform_device	*pdev		= to_platform_device(dev);
	struct vspm_device		*vdev		= platform_get_drvdata(pdev);
	int 					ercd		= -EAGAIN;
	unsigned int			reg_data	= 0;

	if (!vdev || !vdev->syscon_cpg || !vdev->syscon_sysc)
		return -EINVAL;

	/* A1. activate clocks for VSP from CPG */
	ercd = clock_enable(vdev);
	if (ercd) {
		EPRINT("%s: Cannot enable clock: %d\n", __func__, ercd);
		return ercd;
	}

	/* A2. De-assert the reset signal for clock divider from CPG
	 * A3-A4. De-assert module stop for clock divider (done in vsp_reset_deassert)
	 * A5. De-assert reset signals for VSPI and FCPVI */
	ercd = vsp_reset_deassert(vdev);
	if (ercd) {
		EPRINT("%s: Cannot deassert reset: %d\n", __func__, ercd);
		goto err1;
	}

	/* A6. Release the low-power state of ADB400
	 * A7. Confirm that ADB400 have leaved from the low-power state */
	regmap_read(vdev->syscon_cpg, CPG_LP_VSP_CTL1, &reg_data);
	reg_data |= CPG_VSP_LPC1_CLKQREQ; /* Keep others supply request bit */
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL1,
							reg_data,
							CPG_VSP_LPC1_CLKACC, CPG_VSP_LPC1_CLKACC);
	if (ercd) {
		goto err2;
	}

	/* A8. Release the low-power state of the APB-IF
	 * A9. Confirm that APB-IF have leaved from the low-power state */
	regmap_read(vdev->syscon_cpg, CPG_LP_VSP_CTL1, &reg_data);
	reg_data |= CPG_VSP_LPC1_PWRDQREQ; /* Keep others supply request bit */
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL1,
							reg_data,
							CPG_VSP_LPC1_PWRDACC, CPG_VSP_LPC1_PWRDACC);
	if (ercd) {
		goto err2;
	}

	regmap_read(vdev->syscon_cpg, CPG_LP_VSP_CTL2, &reg_data);
	reg_data |= CPG_VSP_LPC2_PWRDQREQ; /* Keep others supply request bit */
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL2,
							CPG_VSP_LPC2_PWRDQREQ,
							CPG_VSP_LPC2_PWRDACC, CPG_VSP_LPC2_PWRDACC);
	if (ercd) {
		goto err2;
	}
	return 0;
err2:
	EPRINT("%s: Cannot release lower power state: %d\n", __func__, ercd);
	syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL1,
					(unsigned int)~(CPG_VSP_LPC1_CLKQREQ | CPG_VSP_LPC1_PWRDQREQ),
					0, 0);
	syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL2,
					(unsigned int)(~CPG_VSP_LPC1_PWRDQREQ),
					0, 0);
	vsp_reset_assert(vdev);
err1:
	clock_disable(vdev);
	return ercd;
}

static int r9a09g047_vsp_power_off(struct device *dev)
{
	struct platform_device	*pdev		= to_platform_device(dev);
	struct vspm_device		*vdev		= platform_get_drvdata(pdev);
	unsigned int			reg_data	= 0;
	int						ercd		= -EAGAIN;

	if (!vdev || !vdev->syscon_cpg || !vdev->syscon_sysc)
		return -EINVAL;

	/* B1. Make the APB-IF to the low-power state
	 * B2. Confirm that APB-IF have been in the low-power state */
	regmap_read(vdev->syscon_cpg, CPG_LP_VSP_CTL2, &reg_data);
	reg_data &= ~CPG_VSP_LPC2_PWRDQREQ;
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL2,
							reg_data,
							0, CPG_VSP_LPC2_PWRDACC);
	if (ercd) {
		EPRINT("%s: Cannot make low power state: %d\n",__func__, ercd);
		return ercd;
	}

	regmap_read(vdev->syscon_cpg, CPG_LP_VSP_CTL1, &reg_data);
	reg_data &= ~CPG_VSP_LPC1_PWRDQREQ;
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL2,
							reg_data,
							0, CPG_VSP_LPC1_PWRDACC);
	if (ercd) {
		EPRINT("%s: Cannot make low power state: %d\n",__func__, ercd);
		goto err1;
	}

	/* B3. Make the ADB400 to the low-power state
	 * B4. Confirm that ADB400 have been in the low-power state */
	regmap_read(vdev->syscon_cpg, CPG_LP_VSP_CTL1, &reg_data);
	reg_data &= ~CPG_VSP_LPC1_CLKQREQ;
	ercd = syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL2,
							reg_data,
							0, 0);
	if (ercd) {
		EPRINT("%s: Cannot make low power state: %d\n",__func__, ercd);
		goto err1;
	}

	/* B5. Assert reset signals for VSPI and FCPVI *
	 * B6. Assert module stop signal for clock divider from SYS
	 * B7. Confirm that clock divider have stopped
	 * B8. Assert the reset signal for clock divider from CPG */
	ercd = reset_assert(vdev);
	if (ercd) {
		EPRINT("%s: Cannot assert reset: %d\n",__func__, ercd);
		goto err1;
	}

	/* B9. Stop clocks for VSP from CPG */
	ercd = clock_disable(vdev);
	if (ercd) {
		EPRINT("%s: Cannot disable clock: %d\n",__func__, ercd);
		goto err2;
	}
	return 0;
err2:
	reset_deassert(vdev);
err1:
	syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL1,
					CPG_VSP_LPC1_CLKQREQ,
					CPG_VSP_LPC1_CLKACC, CPG_VSP_LPC1_CLKACC);
	syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL1,
					CPG_VSP_LPC1_CLKQREQ | CPG_VSP_LPC1_PWRDQREQ,
					CPG_VSP_LPC1_CLKACC, CPG_VSP_LPC1_CLKACC);
	syscon_set_sync(vdev->syscon_cpg, CPG_LP_VSP_CTL2,
					CPG_VSP_LPC2_PWRDQREQ,
					CPG_VSP_LPC1_CLKACC, CPG_VSP_LPC1_CLKACC);
	return ercd;
}

const struct vspm_device_ops r9a09g047_fdp_ops = {
	.power_on			= r9a09g047_fdp_power_on,
	.power_off			= r9a09g047_fdp_power_off,
	.require_features	= VSPM_DT_SYSCON_CPG,
};

const struct vspm_device_ops r9a09g047_vsp_ops = {
	.power_on			= r9a09g047_vsp_power_on,
	.power_off			= r9a09g047_vsp_power_off,
	.require_features	= VSPM_DT_SYSCON_CPG | VSPM_DT_SYSCON_SYSC,
};
