.PHONY: all
EXTRA_CFLAGS := -I$(PWD)/../include
# obj-m is a target name of building kernel module
obj-m := huge.o
# {YOUR MODULE NAME HERE}-objs means needed object files
# we set obj-m as a km.o so set it as a km-objs
huge-objs := main.o
ccflags-y := -O0 -Wno-format -Wno-declaration-after-statement -Wno-unused-function -std=gnu99 -Wno-incompatible-pointer-types
KERNEL_DIR = /lib/modules/$(shell uname -r)/build
VERBOSE   := 0
BIN := main.o

all:
	make -C $(KERNEL_DIR) --include-dir=$(PWD)/../$(INCLUDES) SUBDIRS=$(PWD) KBUILD_VERBOSE=0  modules
	

clean:
	rm -rf  *.o *.ko *.mod.c *.symvers *.order .tmp_versions .read.* .cache.* .*.tmp *.o.ur-* .*.o.cmd user 1gb 2mb 4kb .huge.ko.cmd

clear: clean
	sudo rmmod huge


mount:
	sudo mount -t hugetlbfs none /mnt/hugetlbfs


user:
	gcc hugepage-mmap.c -z execstack -o user


1gb:
	gcc time_1gb.c -o 1gb


2mb:
	gcc time_2mb.c -o 2mb


4kb:
	gcc time_4kb.c -o 4kb

test: all mount user 1gb 2mb 4kb
	sudo insmod huge.ko
	sudo ./user
	sudo perf stat -e dTLB-loads,dTLB-load-misses,iTLB-loads,iTLB-load-misses sudo ./1gb
	sudo perf stat -e dTLB-loads,dTLB-load-misses,iTLB-loads,iTLB-load-misses sudo ./2mb
	sudo perf stat -e dTLB-loads,dTLB-load-misses,iTLB-loads,iTLB-load-misses sudo ./4kb
