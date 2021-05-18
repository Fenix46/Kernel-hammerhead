/* Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c/i2c-qup.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/memory.h>
#include <linux/memblock.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/qpnp-regulator.h>
#include <linux/regulator/krait-regulator.h>
#include <linux/regulator/rpm-smd-regulator.h>
#include <linux/regulator/spm-regulator.h>
#include <linux/clk/msm-clk-provider.h>
#include <asm/mach/map.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <linux/msm-bus.h>
#include <mach/msm_iomap.h>
#include <mach/msm_memtypes.h>
#include <soc/qcom/rpm-smd.h>
#include <soc/qcom/socinfo.h>
#include <soc/qcom/smd.h>
#include <soc/qcom/smem.h>
#include <soc/qcom/spm.h>
#include <soc/qcom/pm.h>
#include "board-dt.h"
#include "clock.h"
#include "platsmp.h"
#include <mach/board_lge.h>

static unsigned long limit_mem;

static int __init limit_mem_setup(char *param)
{
	limit_mem = memparse(param, NULL);
	return 0;
}
early_param("limit_mem", limit_mem_setup);

static void __init limit_mem_reserve(void)
{
	unsigned long to_remove;
	unsigned long reserved_mem;
	unsigned long i;
	phys_addr_t base;

	if (!limit_mem)
		return;

	reserved_mem = ALIGN(memblock.reserved.total_size, PAGE_SIZE);

	to_remove = memblock.memory.total_size - reserved_mem - limit_mem;

	pr_info("Limiting memory from %lu KB to to %lu kB by removing %lu kB\n",
			(memblock.memory.total_size - reserved_mem) / 1024,
			limit_mem / 1024,
			to_remove / 1024);

	/* First find as many highmem pages as possible */
	for (i = 0; i < to_remove; i += PAGE_SIZE) {
		base = memblock_find_in_range(memblock.current_limit,
				MEMBLOCK_ALLOC_ANYWHERE, PAGE_SIZE, PAGE_SIZE);
		if (!base)
			break;
		memblock_remove(base, PAGE_SIZE);
	}
	/* Then find as many lowmem 1M sections as possible */
	for (; i < to_remove; i += SECTION_SIZE) {
		base = memblock_find_in_range(0, MEMBLOCK_ALLOC_ACCESSIBLE,
				SECTION_SIZE, SECTION_SIZE);
		if (!base)
			break;
		memblock_remove(base, SECTION_SIZE);
	}
}

void __init msm_8974_reserve(void)
{
	of_scan_flat_dt(dt_scan_for_memory_reserve, NULL);
#ifdef CONFIG_PSTORE_RAM	
	lge_reserve();
#endif
 	limit_mem_reserve();
}

/*
 * Used to satisfy dependencies for devices that need to be
 * run early or in a particular order. Most likely your device doesn't fall
 * into this category, and thus the driver should not be added here. The
 * EPROBE_DEFER can satisfy most dependency problems.
 */
void __init msm8974_add_drivers(void)
{
	msm_smd_init();
	msm_rpm_driver_init();
	msm_pm_sleep_status_init();
	spm_regulator_init();
	rpm_smd_regulator_driver_init();
	msm_spm_device_init();
	msm_bus_fabric_init_driver();
	qup_i2c_init_driver();
	krait_power_init();
#ifdef CONFIG_PSTORE_RAM	
	lge_add_persistent_device();
#endif	
}

static struct of_dev_auxdata msm_hsic_host_adata[] = {
	OF_DEV_AUXDATA("qcom,hsic-host", 0xF9A00000, "msm_hsic_host", NULL),
	{}
};

static struct of_dev_auxdata msm8974_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("qcom,hsusb-otg", 0xF9A55000, "msm_otg", NULL),
	OF_DEV_AUXDATA("qcom,ehci-host", 0xF9A55000, "msm_ehci_host", NULL),
	OF_DEV_AUXDATA("qcom,dwc-usb3-msm", 0xF9200000, "msm_dwc3", NULL),
	OF_DEV_AUXDATA("qcom,usb-bam-msm", 0xF9304000, "usb_bam", NULL),
	OF_DEV_AUXDATA("qcom,spi-qup-v2", 0xF9924000, \
			"spi_qsd.1", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF9824900, \
			"msm_sdcc.1", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF98A4900, \
			"msm_sdcc.2", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF9864900, \
			"msm_sdcc.3", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF98E4900, \
			"msm_sdcc.4", NULL),
	OF_DEV_AUXDATA("qcom,msm-rng", 0xF9BFF000, \
			"msm_rng", NULL),
	OF_DEV_AUXDATA("qcom,qseecom", 0xFE806000, \
			"qseecom", NULL),
	OF_DEV_AUXDATA("qcom,mdss_mdp", 0xFD900000, "mdp.0", NULL),
	OF_DEV_AUXDATA("qcom,msm-tsens", 0xFC4A8000, \
			"msm-tsens", NULL),
	OF_DEV_AUXDATA("qcom,qcedev", 0xFD440000, \
			"qcedev.0", NULL),
	OF_DEV_AUXDATA("qcom,qcrypto", 0xFD440000, \
			"qcrypto.0", NULL),
	OF_DEV_AUXDATA("qcom,hsic-host", 0xF9A00000, \
			"msm_hsic_host", NULL),
	OF_DEV_AUXDATA("qcom,hsic-smsc-hub", 0, "msm_smsc_hub",
			msm_hsic_host_adata),
	{}
};

static void __init msm8974_map_io(void)
{
	msm_map_8974_io();
}

void __init msm8974_init(void)
{
	struct of_dev_auxdata *adata = msm8974_auxdata_lookup;

	/*
	 * populate devices from DT first so smem probe will get called as part
	 * of msm_smem_init.  socinfo_init needs smem support so call
	 * msm_smem_init before it.  msm_8974_init_gpiomux needs socinfo so
	 * call socinfo_init before it.
	 */
	board_dt_populate(adata);

	msm_smem_init();

	if (socinfo_init() < 0)
		pr_err("%s: socinfo_init() failed\n", __func__);

	msm_8974_init_gpiomux();
	regulator_has_full_constraints();
	msm8974_add_drivers();
}

static const char *const msm8974_dt_match[] __initconst = {
	"qcom,msm8974",
	"qcom,apq8074",
	NULL
};

DT_MACHINE_START(MSM8974_DT, "Qualcomm MSM 8974 (Flattened Device Tree)")
	.map_io			= msm8974_map_io,
	.init_machine		= msm8974_init,
	.dt_compat		= msm8974_dt_match,
	.reserve		= msm_8974_reserve,
	.smp			= &msm8974_smp_ops,
MACHINE_END
