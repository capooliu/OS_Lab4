#include <linux/fs.h>
#include <linux/uaccess.h>
#include "osfs.h"

/**
 * Function: osfs_get_osfs_inode
 * Description: Retrieves the osfs_inode structure for a given inode number.
 * Inputs:
 *   - sb: The superblock of the filesystem.
 *   - ino: The inode number to retrieve.
 * Returns:
 *   - A pointer to the osfs_inode structure if successful.
 *   - NULL if the inode number is invalid or out of bounds.
 */
struct osfs_inode *osfs_get_osfs_inode(struct super_block *sb, uint32_t ino)
{
    struct osfs_sb_info *sb_info = sb->s_fs_info;

    if (ino == 0 || ino >= sb_info->inode_count) // File system inode count upper bound
        return NULL;
    return &((struct osfs_inode *)(sb_info->inode_table))[ino];
}

/**
 * Function: osfs_get_free_inode
 * Description: Allocates a free inode number from the inode bitmap.
 * Inputs:
 *   - sb_info: The superblock information of the filesystem.
 * Returns:
 *   - The allocated inode number on success.
 *   - -ENOSPC if no free inode is available.
 */
int osfs_get_free_inode(struct osfs_sb_info *sb_info)
{
    uint32_t ino;

    for (ino = 1; ino < sb_info->inode_count; ino++) {
        if (!test_bit(ino, sb_info->inode_bitmap)) {
            set_bit(ino, sb_info->inode_bitmap);
            sb_info->nr_free_inodes--;
            return ino;
        }
    }
    pr_err("osfs_get_free_inode: No free inode available\n");
    return -ENOSPC;
}

/**
 * Function: osfs_iget
 * Description: Creates or retrieves a VFS inode from a given inode number.
 * Inputs:
 *   - sb: The superblock of the filesystem.
 *   - ino: The inode number to load.
 * Returns:
 *   - A pointer to the VFS inode on success.
 *   - ERR_PTR(-EFAULT) if the osfs_inode cannot be retrieved.
 *   - ERR_PTR(-ENOMEM) if memory allocation for the inode fails.
 */

/**
 * 新增：分配連續區塊的函數
 */
int osfs_alloc_extent(struct osfs_sb_info *sb_info, uint32_t needed_blocks, 
                      struct osfs_extent *extent) 
{
    uint32_t start = 0;
    uint32_t count = 0;
    uint32_t i;

    // 先檢查是否有足夠的可用空間
    if (needed_blocks > sb_info->nr_free_blocks) {
        pr_err("osfs: Not enough free blocks. Needed: %u, Available: %u\n",
               needed_blocks, sb_info->nr_free_blocks);
        return -ENOSPC;
    }

    // 檢查請求的區塊數是否合理
    if (needed_blocks == 0 || needed_blocks > sb_info->block_count) {
        pr_err("osfs: Invalid number of blocks requested: %u\n", needed_blocks);
        return -EINVAL;
    }

    // 尋找連續的空閒塊
    for (i = 0; i < sb_info->block_count; i++) {
        if (!test_bit(i, sb_info->block_bitmap)) {
            if (count == 0) {
                start = i;
            }
            count++;

            // 找到足夠的連續塊
            if (count == needed_blocks) {
                // 再次確認這些區塊還是可用的
                for (uint32_t j = start; j < start + count; j++) {
                    if (test_bit(j, sb_info->block_bitmap)) {
                        pr_err("osfs: Block %u became allocated during search\n", j);
                        return -ENOSPC;
                    }
                }

                // 設置 extent 資訊
                extent->start_block = start;
                extent->block_count = count;

                // 標記為已使用
                for (uint32_t j = start; j < start + count; j++) {
                    set_bit(j, sb_info->block_bitmap);
                }

                // 更新可用區塊數
                sb_info->nr_free_blocks -= count;

                pr_debug("osfs: Allocated extent: start=%u, count=%u\n", 
                        start, count);
                return 0;
            }
        } else {
            count = 0;  
        }
    }

    pr_err("osfs: Could not find %u contiguous free blocks\n", needed_blocks);
    return -ENOSPC;
}
/**
 * 新增：釋放連續數據塊的函數
 */
void osfs_free_extent(struct osfs_sb_info *sb_info, struct osfs_extent *extent)
{
    uint32_t i;
    
    for (i = extent->start_block; i < extent->start_block + extent->block_count; i++) {
        clear_bit(i, sb_info->block_bitmap);
    }
    sb_info->nr_free_blocks += extent->block_count;
}

struct inode *osfs_iget(struct super_block *sb, unsigned long ino)
{
    struct osfs_inode *osfs_inode;
    struct inode *inode;

    osfs_inode = osfs_get_osfs_inode(sb, ino);
    if (!osfs_inode)
        return ERR_PTR(-EFAULT);

    inode = new_inode(sb);
    if (!inode)
        return ERR_PTR(-ENOMEM);

    inode->i_ino = ino;
    inode->i_sb = sb;
    inode->i_mode = osfs_inode->i_mode;
    i_uid_write(inode, osfs_inode->i_uid);
    i_gid_write(inode, osfs_inode->i_gid);
    simple_inode_init_ts(inode);
    inode->i_size = osfs_inode->i_size;
    inode->i_blocks = osfs_inode->i_blocks;
    inode->i_private = osfs_inode;

    if (S_ISDIR(inode->i_mode)) {
        inode->i_op = &osfs_dir_inode_operations;
        inode->i_fop = &osfs_dir_operations;
    } else if (S_ISREG(inode->i_mode)) {
        inode->i_op = &osfs_file_inode_operations;
        inode->i_fop = &osfs_file_operations;
    }

    insert_inode_hash(inode);

    return inode;
}