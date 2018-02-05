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
	{ 0x7ace506a, __VMLINUX_SYMBOL_STR(i2c_master_send) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xd9ead207, __VMLINUX_SYMBOL_STR(i2c_del_driver) },
	{ 0xabdbe515, __VMLINUX_SYMBOL_STR(dev_printk) },
	{ 0xf7802486, __VMLINUX_SYMBOL_STR(__aeabi_uidivmod) },
	{ 0x6b06fdce, __VMLINUX_SYMBOL_STR(delayed_work_timer_fn) },
	{ 0x70139129, __VMLINUX_SYMBOL_STR(snd_soc_add_codec_controls) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
	{ 0xb7051343, __VMLINUX_SYMBOL_STR(snd_soc_dapm_new_controls) },
	{ 0x8fdf772a, __VMLINUX_SYMBOL_STR(init_timer_key) },
	{ 0x82108f90, __VMLINUX_SYMBOL_STR(snd_soc_dapm_add_routes) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0x1b55d4fa, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x19d3b7d8, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x94680ad3, __VMLINUX_SYMBOL_STR(snd_soc_info_volsw) },
	{ 0xf204b815, __VMLINUX_SYMBOL_STR(i2c_register_driver) },
	{ 0x85d472aa, __VMLINUX_SYMBOL_STR(queue_delayed_work_on) },
	{ 0x31475f0c, __VMLINUX_SYMBOL_STR(i2c_master_recv) },
	{ 0xee9ce628, __VMLINUX_SYMBOL_STR(snd_soc_unregister_codec) },
	{ 0xb742fd7, __VMLINUX_SYMBOL_STR(simple_strtol) },
	{ 0xc5844d6f, __VMLINUX_SYMBOL_STR(of_property_read_variable_u32_array) },
	{ 0x103b85f7, __VMLINUX_SYMBOL_STR(snd_soc_register_codec) },
	{ 0x40c57909, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=snd-soc-core";

MODULE_ALIAS("i2c:ac108_0");
MODULE_ALIAS("i2c:ac108_1");
MODULE_ALIAS("i2c:ac108_2");
MODULE_ALIAS("i2c:ac108_3");
MODULE_ALIAS("of:N*T*Cx-power,ac108_0");
MODULE_ALIAS("of:N*T*Cx-power,ac108_0C*");
MODULE_ALIAS("of:N*T*Cx-power,ac108_1");
MODULE_ALIAS("of:N*T*Cx-power,ac108_1C*");
MODULE_ALIAS("of:N*T*Cx-power,ac108_2");
MODULE_ALIAS("of:N*T*Cx-power,ac108_2C*");
MODULE_ALIAS("of:N*T*Cx-power,ac108_3");
MODULE_ALIAS("of:N*T*Cx-power,ac108_3C*");

MODULE_INFO(srcversion, "8ED7730CCF1BF1674A70161");
