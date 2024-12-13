savedcmd_/home/angelliu/Lab4/osfs.mod := printf '%s\n'   super.o inode.o file.o dir.o osfs_init.o | awk '!x[$$0]++ { print("/home/angelliu/Lab4/"$$0) }' > /home/angelliu/Lab4/osfs.mod
