.PHONY: loader   lib  group1  make_cfg atf

include ./scripts/config.mk

all: lib loader group1   atf

BOARD ?= cuckoo_dvb

lib:
	cd ./lib && make || exit "$$?"

loader:
	cd ./loader && make || exit "$$?"

group1:
	cd ./group1 && make || exit "$$?"

atf:
	python ./scripts/build_atf.py -c ./../att_bin/atf_make.cfg  -o ./../../../prebuild/$(IC_TYPE)/att/atttest.bin

clean:
	cd ./lib && make clean
	cd ./loader && make clean
	cd ./group1 && make clean
	rm -rf ./scripts/*.out
