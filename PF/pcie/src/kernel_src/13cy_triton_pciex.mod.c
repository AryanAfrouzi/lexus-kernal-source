#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x1e95b2f, "module_layout" },
	{ 0x7942259, "cdev_del" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x69ded9e3, "cdev_init" },
	{ 0xd0d8621b, "strlen" },
	{ 0x77aa43e3, "boot_cpu_data" },
	{ 0x6b184f4c, "pci_disable_device" },
	{ 0x27927ec0, "pci_release_regions" },
	{ 0xd1b4bf6a, "mutex_unlock" },
	{ 0x462f0359, "__init_waitqueue_head" },
	{ 0xaad67878, "pci_set_master" },
	{ 0xb72397d5, "printk" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0xb4390f9a, "mcount" },
	{ 0x859c6dc7, "request_threaded_irq" },
	{ 0xa1aee336, "cdev_add" },
	{ 0x42c8de35, "ioremap_nocache" },
	{ 0x10fda20, "pci_unregister_driver" },
	{ 0xe733ec19, "__wake_up" },
	{ 0xabf0e47b, "mutex_lock_nested" },
	{ 0x37a0cba, "kfree" },
	{ 0x4eb13d16, "remap_pfn_range" },
	{ 0x8066401c, "pci_request_regions" },
	{ 0x251cb21e, "pci_disable_msi" },
	{ 0xedc03953, "iounmap" },
	{ 0xd64b85c9, "__pci_register_driver" },
	{ 0xc424ff3b, "complete" },
	{ 0xaafdb9a0, "pci_enable_msi_block" },
	{ 0xd933d774, "pci_enable_device" },
	{ 0xcfb74cc6, "wait_for_completion_timeout" },
	{ 0x33d169c9, "_copy_from_user" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

