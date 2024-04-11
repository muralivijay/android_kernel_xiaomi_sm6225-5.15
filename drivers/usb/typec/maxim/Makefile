#
# Makefile for ccic devices
#
subdir-ccflags-y := -Wformat
obj-$(CONFIG_CCIC_MAX77729)		+= pdic_max77729.o
pdic_max77729-y		:= max77729_cc.o max77729_pd.o max77729_usbc.o max77729_alternate.o
pdic_max77729-$(CONFIG_CCIC_MAX77729_DEBUG) += max77729_debug.o
pdic_max77729-$(CONFIG_MUIC_MAX77729) += max77729-muic.o
