#include <linux/fs.h>
#include <linux/uaccess.h>
#include "osfs.h"

/**
 * Function: osfs_read
 * Description: Reads data from a file.
 * Inputs:
 *   - filp: The file pointer representing the file to read from.
 *   - buf: The user-space buffer to copy the data into.
 *   - len: The number of bytes to read.
 *   - ppos: The file position pointer.
 * Returns:
 *   - The number of bytes read on success.
 *   - 0 if the end of the file is reached.
 *   - -EFAULT if copying data to user space fails.
 */
static ssize_t osfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct osfs_inode *osfs_inode = inode->i_private;
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    void *data_block;
    ssize_t bytes_read = 0;
    uint32_t current_pos = *ppos;

    // If the file has not been allocated a data block, it indicates the file is empty
    if (osfs_inode->i_blocks == 0 || osfs_inode->i_extent_count == 0) {
        pr_debug("osfs_read: File has no data blocks\n");
        return 0;
    }

    if (current_pos >= osfs_inode->i_size)
        return 0;

    if (current_pos + len > osfs_inode->i_size)
        len = osfs_inode->i_size - current_pos;

    while (len > 0) {
        uint32_t accumulated_size = 0;
        struct osfs_extent *current_extent = NULL;
        uint32_t offset_in_extent = 0;
        uint32_t bytes_to_read;
        int i;

        // 在多個 extent 中讀取數據
        for (i = 0; i < osfs_inode->i_extent_count; i++) {
            uint32_t extent_size = osfs_inode->i_extents[i].block_count * BLOCK_SIZE;//第i個extent
            // currentpos在這個 extent 內
            if (current_pos < accumulated_size + extent_size) {
                current_extent = &osfs_inode->i_extents[i];
                offset_in_extent = current_pos - accumulated_size;
                break;
            }
            accumulated_size += extent_size;
        }

        if (!current_extent) {
            pr_err("osfs_read: Could not find extent for position %u\n", current_pos);
            break;
        }

        // 確保數據區塊位置合法
        if (current_extent->start_block >= sb_info->block_count) {
            pr_err("osfs_read: Invalid block number: %u\n", current_extent->start_block);
            return -EIO;
        }

        // 計算這次要讀取的大小
        bytes_to_read = min_t(uint32_t, len,
                            (current_extent->block_count * BLOCK_SIZE) - offset_in_extent);//extent中剩下的空間 or 剩下要讀取的量

        // 計算實際的數據位置
        data_block = (char *)sb_info->data_blocks + 
                    (current_extent->start_block * BLOCK_SIZE) + 
                    offset_in_extent;

        // 驗證記憶體範圍
        if ((void *)data_block + bytes_to_read > 
            (void *)sb_info->data_blocks + (sb_info->block_count * BLOCK_SIZE)) {
            pr_err("osfs_read: Memory access out of bounds\n");
            return -EIO;
        }

        pr_debug("osfs_read: Reading %u bytes from block %u at offset %u\n",
                bytes_to_read, current_extent->start_block, offset_in_extent);

        if (copy_to_user(buf + bytes_read, data_block, bytes_to_read)) {
            pr_err("osfs_read: copy_to_user failed\n");
            return bytes_read > 0 ? bytes_read : -EFAULT;
        }

        bytes_read += bytes_to_read;
        len -= bytes_to_read;
        current_pos += bytes_to_read;
    }

    *ppos = current_pos;
    pr_debug("osfs_read: Read complete. Total bytes read: %zd\n", bytes_read);
    return bytes_read;
}

/**
 * Function: osfs_write
 * Description: Writes data to a file.
 * Inputs:
 *   - filp: The file pointer representing the file to write to.
 *   - buf: The user-space buffer containing the data to write.
 *   - len: The number of bytes to write.
 *   - ppos: The file position pointer.
 * Returns:
 *   - The number of bytes written on success.
 *   - -EFAULT if copying data from user space fails.
 *   - Adjusted length if the write exceeds the block size.
 */
static ssize_t osfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{   
    //Step1: Retrieve the inode and filesystem information
    struct inode *inode = file_inode(filp);
    struct osfs_inode *osfs_inode = inode->i_private;
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    void *data_block;
    ssize_t bytes_written = 0;
    int ret;
    uint32_t current_pos = *ppos; 

    // Step2: Check if a data block has been allocated; if not, allocate one
    if (osfs_inode->i_extent_count == 0) { //如果還沒有extent則分配
        struct osfs_extent *new_extent = &osfs_inode->i_extents[0];
        uint32_t blocks_needed = (len + BLOCK_SIZE - 1) / BLOCK_SIZE; //向上取整數
        
        ret = osfs_alloc_extent(sb_info, blocks_needed, new_extent);
        if (ret) {
            pr_err("osfs_write: Failed to allocate extent\n");
            return ret;
        }
        osfs_inode->i_extent_count = 1;
        osfs_inode->i_blocks += blocks_needed;
    }

    // Step3: Limit the write length to fit within one data block
    // 寫入循環
    while (len > 0) {
        struct osfs_extent *current_extent = NULL;
        uint32_t i;
        uint32_t block_offset;
        uint32_t bytes_to_write;

        // 看現在的寫入位置是否在某一個extent內
        for (i = 0; i < osfs_inode->i_extent_count; i++) {
            struct osfs_extent *extent = &osfs_inode->i_extents[i];
            uint32_t extent_start = extent->start_block * BLOCK_SIZE;
            uint32_t extent_size = extent->block_count * BLOCK_SIZE;
            
            if (current_pos >= extent_start && 
                current_pos < extent_start + extent_size) {
                current_extent = extent;
                break;
            }
        }
// loop device
        // 如果需要分配新的 extent
        if (!current_extent && osfs_inode->i_extent_count < MAX_EXTENT_COUNT) {
            struct osfs_extent *new_extent = &osfs_inode->i_extents[osfs_inode->i_extent_count];
            uint32_t blocks_needed = (len + BLOCK_SIZE - 1) / BLOCK_SIZE;
            
            ret = osfs_alloc_extent(sb_info, blocks_needed, new_extent);
            if (ret) {
                if (bytes_written > 0)
                    break;
                return ret;
            }
            current_extent = new_extent;
            osfs_inode->i_extent_count++;
            osfs_inode->i_blocks += blocks_needed;
        }
        //超過最大連續數
        if (!current_extent) { 
            if (bytes_written > 0)
                break;
            return -ENOSPC;
        }

        // 計算寫入位置和大小
        block_offset = current_pos % BLOCK_SIZE;
        bytes_to_write = min(len, BLOCK_SIZE - block_offset);

        data_block = sb_info->data_blocks + 
                    current_extent->start_block * BLOCK_SIZE + 
                    block_offset;

        // Step4: Write data from user space to the data block
        if (copy_from_user(data_block, buf + bytes_written, bytes_to_write))
            return bytes_written > 0 ? bytes_written : -EFAULT;

        bytes_written += bytes_to_write;
        len -= bytes_to_write;
        current_pos += bytes_to_write;
    }
    

    // Step5: Update inode & osfs_inode attribute
    *ppos = current_pos;
    
    //寫入位置超過file大小則更新file system 和 VFS的inode大小
    if (current_pos > osfs_inode->i_size) {
        osfs_inode->i_size = current_pos;
        inode->i_size = current_pos;
    }
  
    simple_inode_init_ts(inode);//更新修改時間
    mark_inode_dirty(inode); //告訴 VFS 這個 inode 被修改過

    // Step6: Return the number of bytes written
    return bytes_written;
}

/**
 * Struct: osfs_file_operations
 * Description: Defines the file operations for regular files in osfs.
 */
const struct file_operations osfs_file_operations = {
    .open = generic_file_open, // Use generic open or implement osfs_open if needed
    .read = osfs_read,
    .write = osfs_write,
    .llseek = default_llseek,
    // Add other operations as needed
};

/**
 * Struct: osfs_file_inode_operations
 * Description: Defines the inode operations for regular files in osfs.
 * Note: Add additional operations such as getattr as needed.
 */
const struct inode_operations osfs_file_inode_operations = {
    // Add inode operations here, e.g., .getattr = osfs_getattr,
};