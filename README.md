# OS_Lab4
# 編譯模組
make

# 載入新的模組
sudo insmod osfs.ko

# 創建掛載點（如果不存在）
mkdir mnt

# 掛載檔案系統
sudo mount -t osfs none mnt/

# 進入掛載目錄
cd mnt/

#建立檔案
sudo touch test1.txt

ls

# 寫入檔案
sudo bash -c "echo 'I LOVE OSFS' > test1.txt"

# 讀取檔案內容
cat test1.txt

cd ..

# 卸載檔案系統
sudo umount mnt/

# 移除舊的模組
sudo rmmod osfs

# 刪除掛載點
rm -r mnt/
