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
	{ 0x8fc5dfb7, __VMLINUX_SYMBOL_STR(devm_snd_soc_register_card) },
	{ 0x815588a6, __VMLINUX_SYMBOL_STR(clk_enable) },
	{ 0xbcfd140b, __VMLINUX_SYMBOL_STR(asoc_simple_card_canonicalize_dailink) },
	{ 0x2733a62b, __VMLINUX_SYMBOL_STR(snd_soc_of_parse_audio_simple_widgets) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xc3f4c05, __VMLINUX_SYMBOL_STR(asoc_simple_card_canonicalize_cpu) },
	{ 0x3dd3f751, __VMLINUX_SYMBOL_STR(of_parse_phandle) },
	{ 0xb9e26d37, __VMLINUX_SYMBOL_STR(asoc_simple_card_clean_reference) },
	{ 0x870fc8b2, __VMLINUX_SYMBOL_STR(snd_soc_pm_ops) },
	{ 0xb6e6d99d, __VMLINUX_SYMBOL_STR(clk_disable) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
	{ 0x88dc7e6f, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0x1b55d4fa, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x3199c87d, __VMLINUX_SYMBOL_STR(of_device_is_available) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x3e9d3734, __VMLINUX_SYMBOL_STR(of_find_property) },
	{ 0x378c26fe, __VMLINUX_SYMBOL_STR(asoc_simple_card_set_dailink_name) },
	{ 0xb9dc6bef, __VMLINUX_SYMBOL_STR(of_get_child_by_name) },
	{ 0x848b1a5b, __VMLINUX_SYMBOL_STR(asoc_simple_card_init_dai) },
	{ 0x42a0de39, __VMLINUX_SYMBOL_STR(asoc_simple_card_parse_clk) },
	{ 0x94c40b85, __VMLINUX_SYMBOL_STR(asoc_simple_card_parse_daifmt) },
	{ 0xe912cd9a, __VMLINUX_SYMBOL_STR(of_get_next_child) },
	{ 0x7c9a7371, __VMLINUX_SYMBOL_STR(clk_prepare) },
	{ 0x149259a, __VMLINUX_SYMBOL_STR(of_get_named_gpio_flags) },
	{ 0xa6d8aaef, __VMLINUX_SYMBOL_STR(snd_soc_dai_set_sysclk) },
	{ 0x4cce7249, __VMLINUX_SYMBOL_STR(snd_soc_of_parse_audio_routing) },
	{ 0x48b6c9b2, __VMLINUX_SYMBOL_STR(snd_soc_card_jack_new) },
	{ 0xd1b73339, __VMLINUX_SYMBOL_STR(snd_soc_jack_add_gpios) },
	{ 0x5a371847, __VMLINUX_SYMBOL_STR(asoc_simple_card_parse_card_name) },
	{ 0xb81960ca, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x63fbc1b9, __VMLINUX_SYMBOL_STR(snd_soc_jack_free_gpios) },
	{ 0xfc01da1c, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0xc5844d6f, __VMLINUX_SYMBOL_STR(of_property_read_variable_u32_array) },
	{ 0x973c1054, __VMLINUX_SYMBOL_STR(of_node_put) },
	{ 0x40c57909, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0x4716d13d, __VMLINUX_SYMBOL_STR(snd_soc_of_parse_tdm_slot) },
	{ 0xee2335c2, __VMLINUX_SYMBOL_STR(asoc_simple_card_parse_dai) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=snd-soc-core,snd-soc-simple-card-utils";

MODULE_ALIAS("of:N*T*Csimple-audio-card");
MODULE_ALIAS("of:N*T*Csimple-audio-cardC*");

MODULE_INFO(srcversion, "4AD728D0E5FB89C88D04665");
