/*
 * arch/arm/mach-msm/lge/device_lge.c
 *
 * Copyright (C) 2012,2013 LGE, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/memblock.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <asm/setup.h>
#include <asm/system_info.h>
#include <mach/board_lge.h>
#ifdef CONFIG_PSTORE_RAM
#include <linux/pstore_ram.h>
#endif
#ifdef CONFIG_LGE_HANDLE_PANIC
#include <mach/lge_handle_panic.h>
#endif

#ifdef CONFIG_PSTORE_RAM
#define LGE_RAM_CONSOLE_SIZE (128 * SZ_1K * 2)
#define LGE_PERSISTENT_RAM_SIZE (SZ_1M)
static char bootreason[128] = {0,};

int __init lge_boot_reason(char *s)
{
	snprintf(bootreason, sizeof(bootreason),
			"Boot info:\n"
			"Last boot reason: %s\n", s);
	return 1;
}
__setup("bootreason=", lge_boot_reason);

static struct ramoops_platform_data lge_ramoops_data = {
	.mem_size     = LGE_PERSISTENT_RAM_SIZE,
	.console_size = LGE_RAM_CONSOLE_SIZE,
};

static struct platform_device lge_ramoops_dev = {
	.name = "ramoops",
	.dev = {
		.platform_data = &lge_ramoops_data,
	}
};

static void __init lge_add_persist_ram_devices(void)
{
	int ret;
	phys_addr_t base;
	phys_addr_t size;

	size = lge_ramoops_data.mem_size;

	base = 0x10000000;

	pr_info("ramoops: reserved 1 MiB at 0x%08x\n", (int)base);

	lge_ramoops_data.mem_address = base;
	ret = memblock_reserve(lge_ramoops_data.mem_address,
			lge_ramoops_data.mem_size);

	if (ret)
		pr_err("%s: failed to initialize persistent ram\n", __func__);
}


void __init lge_reserve(void)
{
	lge_add_persist_ram_devices();
}

void __init lge_add_persistent_device(void)
{
	int ret;

	if (!lge_ramoops_data.mem_address) {
		pr_err("%s: not allocated memory for ramoops\n", __func__);
		return;
	}

	ret = platform_device_register(&lge_ramoops_dev);
	if (ret){
		pr_err("unable to register platform device\n");
		return;
	}
#ifdef CONFIG_LGE_HANDLE_PANIC
	/* write ram console addr to imem */
	lge_set_ram_console_addr(lge_ramoops_data.mem_address,
			lge_ramoops_data.console_size);
#endif
}
#endif

/* setting whether uart console is enalbed or disabled */
static unsigned int uart_console_mode = 1;  // Alway Off

unsigned int lge_get_uart_mode(void)
{
	return uart_console_mode;
}

void lge_set_uart_mode(unsigned int um)
{
	uart_console_mode = um;
}

static int __init lge_uart_mode(char *uart_mode)
{
	if (!strncmp("enable", uart_mode, 6)) {
		printk(KERN_INFO"UART CONSOLE : enable\n");
		lge_set_uart_mode((UART_MODE_ALWAYS_ON_BMSK | UART_MODE_EN_BMSK)
				& ~UART_MODE_ALWAYS_OFF_BMSK);
	} else {
		printk(KERN_INFO"UART CONSOLE : disable\n");
	}

	return 1;
}
__setup("uart_console=", lge_uart_mode);

/* get boot mode information from cmdline.
 * If any boot mode is not specified,
 * boot mode is normal type.
 */
static enum lge_boot_mode_type lge_boot_mode = LGE_BOOT_MODE_NORMAL;
int __init lge_boot_mode_init(char *s)
{
	if (!strcmp(s, "charger"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGER;
	else if (!strcmp(s, "chargerlogo"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGERLOGO;
	else if (!strcmp(s, "factory"))
		lge_boot_mode = LGE_BOOT_MODE_FACTORY;
	else if (!strcmp(s, "factory2"))
		lge_boot_mode = LGE_BOOT_MODE_FACTORY2;
	else if (!strcmp(s, "pifboot"))
		lge_boot_mode = LGE_BOOT_MODE_PIFBOOT;

	return 1;
}
__setup("androidboot.mode=", lge_boot_mode_init);

enum lge_boot_mode_type lge_get_boot_mode(void)
{
	return lge_boot_mode;
}

/* for board revision */
static hw_rev_type lge_bd_rev = HW_REV_B;

/* CAUTION: These strings are come from LK. */
char *rev_str[] = {"evb1", "evb2", "rev_a", "rev_b", "rev_c", "rev_d",
	"rev_e", "rev_f", "rev_g", "rev_h", "rev_10", "rev_11", "rev_12",
	"revserved"};

int __init board_revno_setup(char *rev_info)
{
	int i;

	for (i = 0; i < HW_REV_MAX; i++) {
		if (!strncmp(rev_info, rev_str[i], 6)) {
			lge_bd_rev = (hw_rev_type) i;
			/* it is defined externally in <asm/system_info.h> */
			system_rev = lge_bd_rev;
			break;
		}
	}

	printk(KERN_INFO "BOARD : LGE %s \n", rev_str[lge_bd_rev]);
	return 1;
}
__setup("lge.rev=", board_revno_setup);

hw_rev_type lge_get_board_revno(void)
{
    return lge_bd_rev;
}
