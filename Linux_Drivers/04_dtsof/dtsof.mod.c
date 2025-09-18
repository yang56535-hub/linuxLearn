#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x4b3323eb, "module_layout" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x37a0cba, "kfree" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0xf8fba7b7, "of_property_count_elems_of_size" },
	{ 0x97f5484e, "of_get_next_child" },
	{ 0xbdc03584, "of_property_read_variable_u32_array" },
	{ 0xf4d357f8, "of_property_read_string" },
	{ 0xc546631d, "of_find_property" },
	{ 0xc5850110, "printk" },
	{ 0x95b97b18, "of_find_node_opts_by_path" },
	{ 0x8f678b07, "__stack_chk_guard" },
};

MODULE_INFO(depends, "");

