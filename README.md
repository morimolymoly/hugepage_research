# hugepage test repository
for testing hugepage(forge 1gb,2mb,4kb sized page)

# add linux boot options to grub config
You have to disable nxbit and smep and to allocate gigantic pages.
```
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash nokaslr console=tty0 console=ttyS0,115200n8 nopti hugepagesz=1GB hugepages=4 default_hugepagesz=1GB noexec=off nosmep"
```

```
sudo vim /etc/default/grub
sudo update-grub2
sudo reboot
```

# build everything and test
```
make test
```

# files
* main.c - kernel module
* huge-page-mmap.c - allocate 1gb page from userland
* cmd.h - device driver include file
* time_(1gb,2mb,4kb).c - call ioctl to execute shellcode on specified size page
* time_walk.c - call ioctl to see what 1gb page is