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
	{ 0xb077e70a, __VMLINUX_SYMBOL_STR(clk_unprepare) },
	{ 0xe56a9336, __VMLINUX_SYMBOL_STR(snd_pcm_format_width) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0xeb711ae7, __VMLINUX_SYMBOL_STR(snd_soc_params_to_bclk) },
	{ 0x815588a6, __VMLINUX_SYMBOL_STR(clk_enable) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xd9ead207, __VMLINUX_SYMBOL_STR(i2c_del_driver) },
	{ 0xaa3b6018, __VMLINUX_SYMBOL_STR(snd_soc_dapm_get_volsw) },
	{ 0x52eb2dc5, __VMLINUX_SYMBOL_STR(regmap_update_bits_base) },
	{ 0xb6e6d99d, __VMLINUX_SYMBOL_STR(clk_disable) },
	{ 0xf7802486, __VMLINUX_SYMBOL_STR(__aeabi_uidivmod) },
	{ 0x70139129, __VMLINUX_SYMBOL_STR(snd_soc_add_codec_controls) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
	{ 0xb7051343, __VMLINUX_SYMBOL_STR(snd_soc_dapm_new_controls) },
	{ 0xcb5daba2, __VMLINUX_SYMBOL_STR(snd_soc_put_volsw) },
	{ 0x66da0eca, __VMLINUX_SYMBOL_STR(snd_soc_get_volsw) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x37dc1ecd, __VMLINUX_SYMBOL_STR(snd_soc_info_enum_double) },
	{ 0x82108f90, __VMLINUX_SYMBOL_STR(snd_soc_dapm_add_routes) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
	{ 0xe872d1ce, __VMLINUX_SYMBOL_STR(snd_soc_read) },
	{ 0x1b55d4fa, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xac92dc30, __VMLINUX_SYMBOL_STR(snd_soc_update_bits) },
	{ 0x3e9d3734, __VMLINUX_SYMBOL_STR(of_find_property) },
	{ 0x8b4dfb4c, __VMLINUX_SYMBOL_STR(snd_soc_dapm_put_volsw) },
	{ 0xa0195a8c, __VMLINUX_SYMBOL_STR(snd_ctl_boolean_mono_info) },
	{ 0x2196324, __VMLINUX_SYMBOL_STR(__aeabi_idiv) },
	{ 0x59e5070d, __VMLINUX_SYMBOL_STR(__do_div64) },
	{ 0x94680ad3, __VMLINUX_SYMBOL_STR(snd_soc_info_volsw) },
	{ 0xf204b815, __VMLINUX_SYMBOL_STR(i2c_register_driver) },
	{ 0x34cffba9, __VMLINUX_SYMBOL_STR(snd_soc_get_enum_double) },
	{ 0xff227eee, __VMLINUX_SYMBOL_STR(__devm_regmap_init_i2c) },
	{ 0x7c9a7371, __VMLINUX_SYMBOL_STR(clk_prepare) },
	{ 0x8c211eeb, __VMLINUX_SYMBOL_STR(devm_clk_get) },
	{ 0xee9ce628, __VMLINUX_SYMBOL_STR(snd_soc_unregister_codec) },
	{ 0xc98126c9, __VMLINUX_SYMBOL_STR(snd_soc_put_enum_double) },
	{ 0x103b85f7, __VMLINUX_SYMBOL_STR(snd_soc_register_codec) },
	{ 0x40c57909, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0xd79caef6, __VMLINUX_SYMBOL_STR(regmap_write) },
	{ 0xad83053c, __VMLINUX_SYMBOL_STR(regcache_sync) },
	{ 0x44c19f83, __VMLINUX_SYMBOL_STR(snd_soc_write) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=snd-pcm,snd-soc-core,snd";

MODULE_ALIAS("i2c:wm8960");
MODULE_ALIAS("of:N*T*Cwlf,wm8960");
MODULE_ALIAS("of:N*T*Cwlf,wm8960C*");

MODULE_INFO(srcversion, "2690F666DC1933DEF3E5373");
