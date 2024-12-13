#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include "osfs.h"

/**
 * Struct: osfs_super_ops
 * Description: Defines the superblock operations for the osfs filesystem.
 */
const struct super_operations osfs_super_ops = {
    .statfs = simple_statfs,            // Provides filesystem statistics
    .drop_inode = generic_delete_inode, // Generic inode deletion
    .destroy_inode = osfs_destroy_inode,

};

void osfs_destroy_inode(struct inode *inode)
{
    if (inode->i_private) {
        struct osfs_inode *osfs_inode = inode->i_private;
        struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
        
        // 釋放所有的 extents
        for (int i = 0; i < osfs_inode->i_extent_count; i++) {
            osfs_free_extent(sb_info, &osfs_inode->i_extents[i]);
        }
        inode->i_private = NULL;
    }
}


/**
 * Function: osfs_fill_super
 * Description: Initializes the superblock with filesystem-specific information during mount.
 * Inputs:
 *   - sb: The superblock to be filled.
 *   - data: Data passed during mount.
 *   - silent: If non-zero, suppress certain error messages.
 * Returns:
 *   - 0 on successful initialization.
 *   - A negative error code on failure.
 */
int osfs_fill_super(struct super_block *sb, void *data, int silent)
{
    pr_info("osfs: Filling super start\n");
    struct inode *root_inode;
    struct osfs_sb_info *sb_info;
    void *memory_region;
    size_t total_memory_size;
    int ret;

    // Calculate total memory size required
    total_memory_size = sizeof(struct osfs_sb_info) +
                       INODE_BITMAP_SIZE * sizeof(unsigned long) +
                       BLOCK_BITMAP_SIZE * sizeof(unsigned long) +
                       INODE_COUNT * sizeof(struct osfs_inode) +
                       DATA_BLOCK_COUNT * BLOCK_SIZE;

    // Allocate memory for superblock information and related structures
    memory_region = vmalloc(total_memory_size);
    if (!memory_region)
        return -ENOMEM;

    memset(memory_region, 0, total_memory_size);

    // Initialize superblock information
    sb_info = (struct osfs_sb_info *)memory_region;
    sb_info->magic = OSFS_MAGIC;
    sb_info->block_size = BLOCK_SIZE;
    sb_info->inode_count = INODE_COUNT;
    sb_info->block_count = DATA_BLOCK_COUNT;
    sb_info->nr_free_inodes = INODE_COUNT - 1;
    sb_info->nr_free_blocks = DATA_BLOCK_COUNT;

    // Partition the memory region into respective components
    sb_info->inode_bitmap = (unsigned long *)(sb_info + 1);
    sb_info->block_bitmap = sb_info->inode_bitmap + INODE_BITMAP_SIZE;
    sb_info->inode_table = (void *)(sb_info->block_bitmap + BLOCK_BITMAP_SIZE);
    sb_info->data_blocks = (void *)((char *)sb_info->inode_table +
                                   INODE_COUNT * sizeof(struct osfs_inode));

    // Initialize bitmaps
    memset(sb_info->inode_bitmap, 0, INODE_BITMAP_SIZE * sizeof(unsigned long));
    memset(sb_info->block_bitmap, 0, BLOCK_BITMAP_SIZE * sizeof(unsigned long));

    // Set superblock fields
    sb->s_magic = sb_info->magic;
    sb->s_fs_info = sb_info;
    sb->s_op = &osfs_super_ops;

    // Create root directory inode
    root_inode = new_inode(sb);
    if (!root_inode) {
        ret = -ENOMEM;
        goto out_free;
    }

    root_inode->i_ino = ROOT_INODE;
    root_inode->i_sb = sb;
    root_inode->i_op = &osfs_dir_inode_operations;
    root_inode->i_fop = &osfs_dir_operations;
    root_inode->i_mode = S_IFDIR | 0755;
    set_nlink(root_inode, 2);
    simple_inode_init_ts(root_inode);

    // Initialize root directory's osfs_inode
    struct osfs_inode *root_osfs_inode = osfs_get_osfs_inode(sb, ROOT_INODE);
    if (!root_osfs_inode) {
        ret = -EIO;
        goto out_iput;
    }
    memset(root_osfs_inode, 0, sizeof(*root_osfs_inode));

    root_osfs_inode->i_ino = ROOT_INODE;
    root_osfs_inode->i_mode = root_inode->i_mode;
    root_osfs_inode->i_links_count = 2;
    root_osfs_inode->i_size = 0;
    root_osfs_inode->i_extent_count = 0;  // 初始化 extent 計數
    simple_inode_init_ts(root_inode);

    // Initialize root directory's osfs_inode
    // 改成extent結構
    struct osfs_extent *root_extent = &root_osfs_inode->i_extents[0];
    ret = osfs_alloc_extent(sb_info, 1, root_extent);
    if (ret < 0) {
        goto out_iput;
    }
    root_osfs_inode->i_extent_count = 1;
    root_osfs_inode->i_blocks = 1;
    root_inode->i_private = root_osfs_inode;

    // Mark root directory inode as used
    set_bit(ROOT_INODE, sb_info->inode_bitmap);

    // Update root directory size
    root_inode->i_size = 0;
    inode_init_owner(&nop_mnt_idmap, root_inode, NULL, root_inode->i_mode);

    // Set the root directory
    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto out_free_extent;
    }
    pr_info("osfs: Superblock filled successfully\n");
    return 0;

out_free_extent:
    osfs_free_extent(sb_info, root_extent);
out_iput:
    iput(root_inode);
out_free:
    vfree(memory_region);
    return ret;
}