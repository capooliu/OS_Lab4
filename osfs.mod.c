#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const char ____versions[]
__used __section("__versions") =
	"\x1c\x00\x00\x00\x79\x43\x05\xde"
	"inode_init_owner\0\0\0\0"
	"\x1c\x00\x00\x00\x48\x9f\xdb\x88"
	"__check_object_size\0"
	"\x18\x00\x00\x00\xe2\x38\xf9\x9a"
	"d_instantiate\0\0\0"
	"\x18\x00\x00\x00\xc2\x9c\xc4\x13"
	"_copy_from_user\0"
	"\x14\x00\x00\x00\x2c\xb5\xaa\xd8"
	"new_inode\0\0\0"
	"\x20\x00\x00\x00\x1e\xdf\x4c\xa0"
	"unregister_filesystem\0\0\0"
	"\x18\x00\x00\x00\x68\x5e\xad\x90"
	"simple_statfs\0\0\0"
	"\x14\x00\x00\x00\x52\xa1\x91\x9b"
	"d_make_root\0"
	"\x24\x00\x00\x00\x6f\x6f\x23\x4c"
	"__x86_indirect_thunk_r15\0\0\0\0"
	"\x18\x00\x00\x00\x01\x07\x74\xe4"
	"d_splice_alias\0\0"
	"\x18\x00\x00\x00\x25\x4e\x86\x0c"
	"current_time\0\0\0\0"
	"\x10\x00\x00\x00\x67\x53\xa6\x60"
	"iput\0\0\0\0"
	"\x1c\x00\x00\x00\x27\x63\xca\x70"
	"register_filesystem\0"
	"\x14\x00\x00\x00\xbb\x6d\xfb\xbd"
	"__fentry__\0\0"
	"\x20\x00\x00\x00\x98\xbe\x62\xc2"
	"simple_inode_init_ts\0\0\0\0"
	"\x24\x00\x00\x00\x97\x70\x48\x65"
	"__x86_indirect_thunk_rax\0\0\0\0"
	"\x10\x00\x00\x00\xd8\x7e\x99\x92"
	"_printk\0"
	"\x14\x00\x00\x00\x32\x2c\x6e\xc2"
	"make_kuid\0\0\0"
	"\x10\x00\x00\x00\x94\xb6\x16\xa9"
	"strnlen\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x18\x00\x00\x00\xa9\xa1\x2f\x1b"
	"vmalloc_noprof\0\0"
	"\x14\x00\x00\x00\xd0\xff\xf5\x0d"
	"set_nlink\0\0\0"
	"\x10\x00\x00\x00\x11\x13\x92\x5a"
	"strncmp\0"
	"\x14\x00\x00\x00\x9a\x61\x3f\x4d"
	"from_kgid\0\0\0"
	"\x10\x00\x00\x00\xda\xfa\x66\x91"
	"strncpy\0"
	"\x18\x00\x00\x00\xb5\x79\xca\x75"
	"__fortify_panic\0"
	"\x18\x00\x00\x00\x02\xd4\xde\x7b"
	"default_llseek\0\0"
	"\x14\x00\x00\x00\x17\x9e\xcc\xeb"
	"from_kuid\0\0\0"
	"\x24\x00\x00\x00\x2a\x9b\x54\x31"
	"__x86_indirect_thunk_r10\0\0\0\0"
	"\x1c\x00\x00\x00\xd5\x71\x38\xc5"
	"__insert_inode_hash\0"
	"\x1c\x00\x00\x00\xca\x39\x82\x5b"
	"__x86_return_thunk\0\0"
	"\x18\x00\x00\x00\xe1\xbe\x10\x6b"
	"_copy_to_user\0\0\0"
	"\x14\x00\x00\x00\xca\x19\x45\xd7"
	"make_kgid\0\0\0"
	"\x14\x00\x00\x00\x02\xf0\xfe\xd7"
	"mount_nodev\0"
	"\x10\x00\x00\x00\x97\x82\x9e\x99"
	"vfree\0\0\0"
	"\x1c\x00\x00\x00\x65\x62\xf5\x2c"
	"__dynamic_pr_debug\0\0"
	"\x20\x00\x00\x00\xea\x1f\x17\x10"
	"generic_delete_inode\0\0\0\0"
	"\x1c\x00\x00\x00\x8f\x7c\x3a\x40"
	"generic_file_open\0\0\0"
	"\x18\x00\x00\x00\x14\x30\x0d\xa8"
	"d_parent_ino\0\0\0\0"
	"\x1c\x00\x00\x00\xf0\xeb\xfe\xbe"
	"__mark_inode_dirty\0\0"
	"\x1c\x00\x00\x00\x7d\x85\xae\xc9"
	"generic_file_llseek\0"
	"\x18\x00\x00\x00\x56\x41\x31\xbc"
	"nop_mnt_idmap\0\0\0"
	"\x18\x00\x00\x00\xfd\x1a\x4a\xa7"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "BEA96DC95222454641403F1");
