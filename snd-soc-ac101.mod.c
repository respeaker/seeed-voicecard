#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0xdb0d8ebc, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x2d3385d3, __VMLINUX_SYMBOL_STR(system_wq) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xd9ead207, __VMLINUX_SYMBOL_STR(i2c_del_driver) },
	{ 0x1c9ec9e6, __VMLINUX_SYMBOL_STR(regulator_disable) },
	{ 0x6b06fdce, __VMLINUX_SYMBOL_STR(delayed_work_timer_fn) },
	{ 0x70139129, __VMLINUX_SYMBOL_STR(snd_soc_add_codec_controls) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
	{ 0x8fdf772a, __VMLINUX_SYMBOL_STR(init_timer_key) },
	{ 0x2f1f8f20, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0xcb5daba2, __VMLINUX_SYMBOL_STR(snd_soc_put_volsw) },
	{ 0xe1250b50, __VMLINUX_SYMBOL_STR(regmap_read) },
	{ 0x66da0eca, __VMLINUX_SYMBOL_STR(snd_soc_get_volsw) },
	{ 0xb1ea493e, __VMLINUX_SYMBOL_STR(sysfs_remove_group) },
	{ 0x84e40310, __VMLINUX_SYMBOL_STR(register_start_clock) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
	{ 0xe872d1ce, __VMLINUX_SYMBOL_STR(snd_soc_read) },
	{ 0x1b55d4fa, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0xf090eadf, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x19d3b7d8, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0xac92dc30, __VMLINUX_SYMBOL_STR(snd_soc_update_bits) },
	{ 0xb729ab40, __VMLINUX_SYMBOL_STR(regulator_bulk_get) },
	{ 0x9138e27f, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x94680ad3, __VMLINUX_SYMBOL_STR(snd_soc_info_volsw) },
	{ 0xf204b815, __VMLINUX_SYMBOL_STR(i2c_register_driver) },
	{ 0x85d472aa, __VMLINUX_SYMBOL_STR(queue_delayed_work_on) },
	{ 0xff227eee, __VMLINUX_SYMBOL_STR(__devm_regmap_init_i2c) },
	{ 0xee9ce628, __VMLINUX_SYMBOL_STR(snd_soc_unregister_codec) },
	{ 0xbeea083, __VMLINUX_SYMBOL_STR(devm_gpiod_get_optional) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x3b045959, __VMLINUX_SYMBOL_STR(regulator_put) },
	{ 0xb742fd7, __VMLINUX_SYMBOL_STR(simple_strtol) },
	{ 0xb2d48a2e, __VMLINUX_SYMBOL_STR(queue_work_on) },
	{ 0x103b85f7, __VMLINUX_SYMBOL_STR(snd_soc_register_codec) },
	{ 0x5445472c, __VMLINUX_SYMBOL_STR(gpiod_set_value) },
	{ 0x40c57909, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0xad83053c, __VMLINUX_SYMBOL_STR(regcache_sync) },
	{ 0x99b90480, __VMLINUX_SYMBOL_STR(regcache_cache_only) },
	{ 0x3f14810b, __VMLINUX_SYMBOL_STR(regulator_enable) },
	{ 0x44c19f83, __VMLINUX_SYMBOL_STR(snd_soc_write) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=snd-soc-core,snd-soc-simple-card";

MODULE_ALIAS("of:N*T*Cx-power,ac101");
MODULE_ALIAS("of:N*T*Cx-power,ac101C*");

MODULE_INFO(srcversion, "D993030BC9B6E1FB4EEB3FC");
