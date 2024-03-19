#!/usr/bin/env python
#
# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import time
import argparse
import platform
import shutil
import subprocess
import re
import xml.etree.ElementTree as ET
import filecmp

sdk_arch = "arm"
sdk_root = os.getcwd()
sdk_root_parent = os.path.dirname(sdk_root)
#sdk_mcuboot_dir=os.path.join(sdk_root_parent, "bootloader", "mcuboot")
#sdk_mcuboot_imgtool = os.path.join(sdk_mcuboot_dir, "scripts", "imgtool.py")
#sdk_mcuboot_key= os.path.join(sdk_mcuboot_dir, "root-rsa-2048.pem")

#sdk_mcuboot_app_path=os.path.join(sdk_mcuboot_dir, "boot", "zephyr")
sdk_boards_dir = os.path.join(sdk_root, "boards", sdk_arch)
#sdk_application_dir = os.path.join(sdk_root_parent, "application")
sdk_application_dir=""
sdk_app_dir = os.path.join(sdk_root_parent, "application")
sdk_samples_dir = os.path.join(sdk_root, "samples")
sdk_script_dir = os.path.join(sdk_root, "tools")
sdk_script_prebuilt_dir = os.path.join(sdk_script_dir, "prebuilt")
build_config_file = os.path.join(sdk_root, ".build_config")



application = ""
sub_app = ""
board_conf = ""
app_conf = ""
config_conf = ""
zephyr_out = ""

def is_windows():
    sysstr = platform.system()
    if (sysstr.startswith('Windows') or \
       sysstr.startswith('MSYS') or     \
       sysstr.startswith('MINGW') or    \
       sysstr.startswith('CYGWIN')):
        return True
    else:
        return False

def to_int(str):
    try:
        int(str)
        return int(str)
    except ValueError:
        try:
            float(str)
            return int(float(str))
        except ValueError:
            return False

def read_choice(  tip, path):
    print("")
    print(" %s" %(tip))
    print("")
    dirs = next(os.walk(path))[1]
    i = 0

    for file in dirs:
        file_path = os.path.join(path, file)
        if os.path.isdir(file_path):
            i += 1
            print("    %d. %s" %(i, file))

    print("")
    input_prompt = "Which would you like? [" + dirs[0] + "] "
    str = ""
    try:
        str = input(input_prompt)
    except Exception as ex:
        print("")
    if to_int(str) != False:
        j = to_int(str)
    else:
        j = 1

    if j > i:
        j = i
    j -= 1
    return dirs[j]


def read_choice_file(  tip, path, file_suffix):
    print("")
    print(" %s" %(tip))
    print("")
    files = next(os.walk(path))[2]
    types_files = []
    i = 0

    for file in files:
        file_path = os.path.join(path, file)
        if os.path.splitext(file_path)[1].lower() == file_suffix:
            types_files.append(file)

    for file in types_files:
        i += 1
        print("    %d. %s" %(i, file))

    print("")
    input_prompt = "Which would you like? [" + types_files[0] + "] "
    str = ""
    try:
        str = input(input_prompt)
    except Exception as ex:
        print("")
    if to_int(str) != False:
        j = to_int(str)
    else:
        j = 1

    if j > i:
        j = i
    j -= 1
    return types_files[j]



def run_cmd(cmd):
    """Echo and run the given command.

    Args:
    cmd: the command represented as a list of strings.
    Returns:
    A tuple of the output and the exit code.
    """
#    print("Running: ", " ".join(cmd))
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
    #print("%s" % (output.rstrip()))
    return (output, p.returncode)

def build_nvram_bin(out_dir, board_dir, app_dir, app_cfg_dir):
    nvram_path = os.path.join(out_dir, "nvram.bin")
    app_cfg_path =  os.path.join(app_cfg_dir, "nvram.prop")
    if os.path.exists(app_cfg_path) == False:
        app_cfg_path =  os.path.join(app_dir, "nvram.prop")
    board_cnf_path =  os.path.join(board_dir, "nvram.prop")
    print("\n build_nvram \n")
    if not os.path.isfile(board_cnf_path):
        print("\n  board nvram %s not exists\n" %(board_cnf_path))
        return
    if not os.path.isfile(app_cfg_path):
        print("\n  app nvram %s not exists\n" %(app_cfg_path))
        return
    script_nvram_path = os.path.join(sdk_script_dir, 'build_nvram_bin.py')
    if (is_windows()):
        cmd = ['python', '-B', script_nvram_path,  '-o', nvram_path, app_cfg_path, board_cnf_path]
    else:
        cmd = ['python3', '-B', script_nvram_path,  '-o', nvram_path, app_cfg_path, board_cnf_path]

    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_nvram_bin error')
        print(outmsg)
        sys.exit(1)
    print("\n build_nvram  finshed\n\n")

def dir_cp(out_dir, in_dir):
    print(" cp dir %s  to dir %s"  %(in_dir, out_dir))
    if os.path.exists(in_dir) == True:
        if os.path.exists(out_dir) == True:
            for parent, dirnames, filenames in os.walk(in_dir):
                for filename in filenames:
                    pathfile = os.path.join(parent, filename)
                    shutil.copyfile(pathfile, os.path.join(out_dir, filename))
        else:
            shutil.copytree(in_dir, out_dir)

def dir_tree_cp(out_dir, in_dir):
    # keep directory structure
    print(" cp dir %s  to dir %s"  %(in_dir, out_dir))
    if os.path.exists(in_dir):
        if os.path.exists(out_dir):
            for parent, dirnames, filenames in os.walk(in_dir):
                for filename in filenames:
                    copy_to_dir = os.path.join(out_dir, os.path.relpath(parent, in_dir))
                    if not os.path.isdir(copy_to_dir):
                        os.mkdir(copy_to_dir)
                    pathfile = os.path.join(parent, filename)
                    shutil.copyfile(pathfile, os.path.join(copy_to_dir, filename))
        else:
            shutil.copytree(in_dir, out_dir)

def check_cp(out_dir, in_dir, name, outname=None):
    in_cp = os.path.join(in_dir, name)
    out_cp = os.path.join(out_dir, outname) if outname else os.path.join(out_dir, name)
    if os.path.exists(in_cp):
        if os.path.isfile(in_cp) == True:
            print(" cp file %s  to dir %s" %(in_cp,out_cp))
            shutil.copyfile(in_cp, out_cp)
        else :
            dir_tree_cp(out_dir, in_cp)

def build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir, soc, sdfs_name):
    board_sdfs_dir = os.path.join(board_dir, sdfs_name)
    app_sdfs_dir = os.path.join(app_cfg_dir, sdfs_name)
    if os.path.exists(app_sdfs_dir) == False:
        app_sdfs_dir = os.path.join(app_dir, "prebuild", soc, sdfs_name)
    #print("\n build_sdfs : sdfs_dir %s, board dfs %s\n\n" %(sdfs_dir, board_sdfs_dir))
    print("\n board sdfs %s, app_sdfs_dir %s\n\n" %(board_sdfs_dir, app_sdfs_dir))
    if (os.path.exists(board_sdfs_dir) == False) and (os.path.exists(app_sdfs_dir) == False):
        print("\n build_sdfs :  %s not exists\n\n" %(sdfs_name))
        return

    sdfs_dir = os.path.join(out_dir, sdfs_name)
    if os.path.exists(sdfs_dir) == True:
        shutil.rmtree(sdfs_dir)
        time.sleep(0.1)

    if os.path.exists(board_sdfs_dir) ==  True:
        dir_cp(sdfs_dir, board_sdfs_dir)

    print("\n app: %s\n\n" %(app_sdfs_dir))
    if os.path.exists(app_sdfs_dir) == True:
        dir_cp(sdfs_dir, app_sdfs_dir)

    # CONFIG_APP_TRANSMITTER
    if read_val_by_config(os.path.join(zephyr_out, ".config"), "CONFIG_APP_TRANSMITTER") == "y":
        tr_app_sdfs_dir = os.path.join(app_cfg_dir, "tr_" + sdfs_name)
        if os.path.exists(tr_app_sdfs_dir) == False:
            tr_app_sdfs_dir = os.path.join(app_dir, "prebuild", soc, "tr_" + sdfs_name)
        dir_cp(sdfs_dir, tr_app_sdfs_dir)

    if (sdfs_name == 'ksdfs') or (sdfs_name == 'sdfs'):
        ksym_bin = os.path.join(out_dir, "ksym.bin")
        if os.path.exists(ksym_bin):
            shutil.move(ksym_bin, os.path.join(sdfs_dir, "ksym.bin"))

    script_sdfs_path = os.path.join(sdk_script_dir, 'build_sdfs.py')
    sdfs_bin_path = os.path.join(out_dir, sdfs_name+".bin")
    if (is_windows()):
        cmd = ['python', '-B', script_sdfs_path,  '-o', sdfs_bin_path, '-d', sdfs_dir]
    else:
        cmd = ['python3', '-B', script_sdfs_path,  '-o', sdfs_bin_path, '-d', sdfs_dir]

    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_sdfs_bin error')
        print(outmsg)
        sys.exit(1)
    print("\n build_sdfs %s finshed\n\n" %(sdfs_name))

def build_sdfs_bin_bydir(out_dir, board_dir, app_dir, app_cfg_dir, soc):
    bpath = os.path.join(board_dir, "fs_sdfs");
    apath = os.path.join(app_cfg_dir, "fs_sdfs");
    if os.path.exists(apath) == False:
        apath = os.path.join(app_dir, "prebuild", soc, "fs_sdfs");
    if (os.path.exists(bpath) == False) and (os.path.exists(apath) == False):
        print('fs_sdfs not exists!')
        return
    all_dirs = {}
    if os.path.exists(bpath) == True:
        for file in os.listdir(bpath):
            print("board list %s ---\n" %(file))
            if os.path.isdir(os.path.join(bpath,file)):
                all_dirs[file] = file
    if os.path.exists(apath) == True:
        for file in os.listdir(apath):
            print("app list %s ---\n" %(file))
            if os.path.isdir(os.path.join(apath,file)):
                all_dirs[file] = file
    for dir in all_dirs.keys():
        print("--build_sdfs %s ---\n" %(dir))
        build_sdfs_bin_name(out_dir, bpath, apath, app_cfg_dir, soc, dir)


def build_sdfs_bin(out_dir, board_dir, app_dir, app_cfg_dir, soc):
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir, soc, "ksdfs")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir, soc, "sdfs")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir, soc, "fatfs")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir, soc, "res_sdfs")
    build_sdfs_bin_name(out_dir, board_dir, app_dir, app_cfg_dir, soc, "tone")
    build_sdfs_bin_bydir(out_dir, board_dir, app_dir, app_cfg_dir, soc)

def file_link(filename, file_add):
    if not os.path.exists(filename):
        return
    if not os.path.exists(file_add):
        return
    print("file %s, link %s \n" %(filename, file_add))
    with open(file_add, 'rb') as fa:
        file_data = fa.read()
        fa.close()
    fsize = os.path.getsize(filename)
    with open(filename, 'rb+') as f:
        f.seek(fsize, 0)
        f.write(file_data)
        f.close()

def is_config_KALLSYMS(cfg_path):
    bret = False
    print("\n cfg_path =%s\n" %cfg_path)
    fp = open(cfg_path,"r")
    lines = fp.readlines()
    for elem in lines:
        if elem[0] != '#':
            _c = elem.strip().split("=")
            if _c[0] == "CONFIG_KALLSYMS":
                print("CONFIG_KALLSYMS");
                bret = True
                break;
    fp.close
    print("\nCONFIG_KALLSYMS=%d\n" %bret )
    return bret


def build_datafs_bin(out_dir, app_dir, app_cfg_dir, fw_cfgfile):
    script_datafs_path = os.path.join(sdk_script_dir, 'build_datafs.py')

    tree = ET.ElementTree(file=fw_cfgfile)
    root = tree.getroot()
    if (root.tag != 'firmware'):
        sys.stderr.write('error: invalid firmware config file')
        sys.exit(1)

    part_list = root.find('partitions').findall('partition')
    for part in part_list:
        part_prop = {}
        for prop in part:
            part_prop[prop.tag] = prop.text.strip()
        if ('storage_id' in part_prop.keys()) and (part_prop['name'] == 'udisk'):
            app_datafs_dir = os.path.join(app_cfg_dir, part_prop['name'])
            if os.path.exists(app_datafs_dir) == False:
                app_datafs_dir = os.path.join(app_dir, part_prop['name'])
                if os.path.exists(app_datafs_dir) == False:
                    print("\n build_datafs : app config datafs %s not exists\n\n" %(app_datafs_dir))
                    continue
            datafs_cap = 0
            for parent, dirs, files in os.walk(app_datafs_dir):
                for file in files:
                    datafs_cap += os.path.getsize(os.path.join(parent, file))

            if datafs_cap == 0:
                print("\n build_datafs: no file in %s\n\n" %(app_datafs_dir))
                continue

            # reserved 0x80000 space for FAT filesystem region such as boot sector, FAT region, root directory region.
            datafs_cap += 0x80000

            datafs_cap = int((datafs_cap + 511) / 512) * 512

            if datafs_cap > int(part_prop['size'], 0):
                print("\n build_datafs: too large file:%d and max:%d\n\n" %(datafs_cap, int(part_prop['size'], 0)))
                continue

            datafs_bin_path = os.path.join(out_dir, part_prop['file_name'])
            print('DATAFS (%s) capacity => 0x%x' %(datafs_bin_path, datafs_cap))
            if (is_windows()):
                cmd = ['python', '-B', script_datafs_path,  '-o', datafs_bin_path, '-s', str(datafs_cap), '-d', app_datafs_dir]
            else:
                cmd = ['python3', '-B', script_datafs_path,  '-o', datafs_bin_path, '-s', str(datafs_cap), '-d', app_datafs_dir]

            (outmsg, exit_code) = run_cmd(cmd)
            if exit_code !=0:
                print('make build_datafs_bin %s error' %(datafs_bin_path))
                print(outmsg)
                sys.exit(1)

    print("\n build_datafs finshed\n\n")


def read_val_by_config(cfg_path, cfg_name):
    val = "0"
    fp = open(cfg_path,"r")
    lines = fp.readlines()
    for elem in lines:
        if elem[0] != '#':
            _c = elem.strip().split("=")
            if _c[0] == cfg_name:
                val = _c[1].strip('"')
                break;
    fp.close
    return val


def read_soc_by_config(cfg_path):
    soc = "lark"
    fp = open(cfg_path,"r")
    lines = fp.readlines()
    for elem in lines:
        if elem[0] != '#':
            _c = elem.strip().split("=")
            if _c[0] == "CONFIG_SOC_SERIES":
                soc = _c[1].strip('"')
                break;
    fp.close
    return soc

def build_firmware(board, out_dir, need_checkid, board_dir, app_dir, app_cfg_dir):
    print("\n build_firmware : out %s, board %s, app_dir %s cfg_dir %s check_id %s\n\n" %(out_dir, board_dir, app_dir, app_cfg_dir, need_checkid))
    FW_DIR = os.path.join(out_dir, "_firmware")
    OS_OUTDIR = os.path.join(out_dir, "zephyr")
    if os.path.exists(FW_DIR) == False:
        os.mkdir(FW_DIR)
    FW_DIR_BIN = os.path.join(FW_DIR, "bin")
    if os.path.exists(FW_DIR_BIN) == False:
        os.mkdir(FW_DIR_BIN)

    if is_config_KALLSYMS(os.path.join(OS_OUTDIR, ".config")):
        print("cpy ksym.bin- to fw dir-");
        check_cp(FW_DIR_BIN , OS_OUTDIR, "ksym.bin")


    build_nvram_bin(FW_DIR_BIN, board_dir, app_dir, app_cfg_dir)
    #build_sdfs_bin(FW_DIR_BIN, board_dir, app_dir, app_cfg_dir)
    soc = read_soc_by_config(os.path.join(OS_OUTDIR, ".config"))
    print("\n build_firmware : soc name= %s\n" %(soc))
    
    # CONFIG_APP_TRANSMITTER
    global zephyr_out
    zephyr_out = OS_OUTDIR
    
    build_sdfs_bin(FW_DIR_BIN, board_dir, app_dir, app_cfg_dir, soc)
    dir_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "common", "bin"))
    dir_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "bin"))
    dir_tree_cp(FW_DIR_BIN, os.path.join(sdk_script_prebuilt_dir, soc, "prebuild"))
    #dir_cp(os.path.join(FW_DIR_BIN, "usr_cfg"), os.path.join(app_cfg_dir, "usr_cfg"))
    usr_config_bin = os.path.join(FW_DIR_BIN, "usr_config.bin")
    if os.path.exists(usr_config_bin) == True:
        os.remove(usr_config_bin)
    check_cp(FW_DIR_BIN, app_cfg_dir, config_conf.split('.')[0] + "_usr_config.bin", "usr_config.bin")

    fw_cfgfile = os.path.join(FW_DIR, "firmware.xml")

    #check copy firmware.xml
    check_cp(FW_DIR, board_dir, "firmware.xml")
    check_cp(FW_DIR, app_dir, "firmware.xml")
    check_cp(FW_DIR, app_cfg_dir, "firmware.xml")
    if not os.path.exists(fw_cfgfile):
        print("failed to find out firmware.xml")
        sys.exit(1)

    # check override files
    check_cp(FW_DIR_BIN, board_dir, "mbrec.bin")
    check_cp(FW_DIR_BIN, board_dir, "afi.bin")
    check_cp(FW_DIR_BIN, board_dir, "bootloader.ini")
    check_cp(FW_DIR_BIN, board_dir, "nand_id.bin")
    check_cp(FW_DIR_BIN, board_dir, "recovery.bin")
    check_cp(FW_DIR_BIN, board_dir, "adfus.bin")
    #check_cp(FW_DIR_BIN, app_dir, os.path.join("prebuild", soc))
    # CONFIG_APP_TRANSMITTER
    check_cp(FW_DIR_BIN, os.path.join(app_dir, "prebuild", soc), "fw_product.cfg")
    check_cp(FW_DIR_BIN, os.path.join(app_dir, "prebuild", soc), "recovery.bin")
    check_cp(os.path.join(FW_DIR_BIN, "config"), os.path.join(app_dir, "prebuild", soc, "config"), "")
    check_cp(os.path.join(FW_DIR_BIN, "att"), os.path.join(app_dir, "prebuild", soc, "att"), "")
    check_cp(FW_DIR_BIN, app_cfg_dir, "prebuild")
    check_cp(FW_DIR_BIN, app_cfg_dir, "E_CHECK.fw")
    check_cp(FW_DIR_BIN, app_cfg_dir, "fwimage.cfg")
    check_cp(os.path.join(FW_DIR_BIN, "att"), app_cfg_dir, "res")

    # CONFIG_APP_TRANSMITTER
    if read_val_by_config(os.path.join(zephyr_out, ".config"), "CONFIG_APP_TRANSMITTER") == "y":
        check_cp(os.path.join(FW_DIR_BIN, "config"), os.path.join(app_dir, "prebuild", soc, "tr_config"), "")

    print("\nFW: Post build mbrec.bin\n")
    script_firmware_path = os.path.join(sdk_script_dir, 'build_boot_image.py')
    if (is_windows()):
        cmd = ['python', '-B', script_firmware_path, os.path.join(FW_DIR_BIN, "mbrec.bin"), \
            os.path.join(FW_DIR_BIN, "bootloader.ini"), \
            os.path.join(FW_DIR_BIN, "nand_id.bin")]
    else:
        cmd = ['python3', '-B', script_firmware_path, os.path.join(FW_DIR_BIN, "mbrec.bin"), \
            os.path.join(FW_DIR_BIN, "bootloader.ini"), \
            os.path.join(FW_DIR_BIN, "nand_id.bin")]

    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('make build_boot_image error')
        print(outmsg)
        sys.exit(1)


    print("\nFW: Post build recovery.bin\n")
    script_firmware_path = os.path.join(sdk_script_dir, 'pack_app_image.py')
    if (is_windows()):
        cmd = ['python', '-B', script_firmware_path, os.path.join(FW_DIR_BIN, "recovery.bin")]
    else:
        cmd = ['python3', '-B', script_firmware_path, os.path.join(FW_DIR_BIN, "recovery.bin")]

    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('pack recovery error')
        print(outmsg)
        sys.exit(1)


    build_datafs_bin(FW_DIR_BIN, app_dir, app_cfg_dir, fw_cfgfile)

    #copy zephyr.bin
    app_bin_src_path = os.path.join(OS_OUTDIR, "zephyr.bin")
    if os.path.exists(app_bin_src_path) == False:
        print("\n app.bin not eixt=%s\n\n"  %(app_bin_src_path))
        return
    #app_bin_dst_path=os.path.join(FW_DIR_BIN, "zephyr.bin")
    app_bin_dst_path=os.path.join(FW_DIR_BIN, "app.bin")
    shutil.copyfile(app_bin_src_path, app_bin_dst_path)


    # signed img
    """
    app_bin_sign_path=os.path.join(FW_DIR_BIN, "app.bin")
    cmd = ['python3', '-B', sdk_mcuboot_imgtool,  'sign', \
            '--key', sdk_mcuboot_key,\
            '--header-size', "0x200",\
            '--align', "8", \
            '--version', "1.2",\
            '--slot-size', "0x60000",\
            app_bin_dst_path,\
            app_bin_sign_path]
    print("\n signed cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    if exit_code !=0:
        print('signed img error')
        print(outmsg)
        sys.exit(1)

    #copy mcuboot.bin
    boot_bin_src_path = os.path.join(out_dir, "boot", "zephyr", "zephyr.bin")
    if os.path.exists(boot_bin_src_path) == False:
        print("\n mcuboot not eixt=%s\n\n"  %(boot_bin_src_path))
        return
    shutil.copyfile(boot_bin_src_path, os.path.join(FW_DIR_BIN, "mcuboot.bin"))
    """
    print("\n--board %s, soc %s ==--\n\n"  %(board, soc))
    script_firmware_path = os.path.join(sdk_script_dir, 'build_firmware.py')
    # CONFIG_APP_TRANSMITTER
    if read_val_by_config(os.path.join(zephyr_out, ".config"), "CONFIG_APP_TRANSMITTER") == "y":
        board = "tr_" + board
    fw_ver = read_val_by_config(os.path.join(zephyr_out, ".config"), "CONFIG_FW_VERSION_NUMBER")
    if (is_windows()):
        cmd = ['python', '-B', script_firmware_path,  '-c', fw_cfgfile, \
            '-e', os.path.join(FW_DIR_BIN, "encrypt.bin"),\
            '-ef', os.path.join(FW_DIR_BIN, "efuse.bin"),\
            '-s', soc,\
            '-b', board,\
            '-v', fw_ver,\
            '-d', need_checkid]
    else:
        cmd = ['python3', '-B', script_firmware_path,  '-c', fw_cfgfile, \
            '-e', os.path.join(FW_DIR_BIN, "encrypt.bin"),\
            '-ef', os.path.join(FW_DIR_BIN, "efuse.bin"),\
            '-s', soc,\
            '-b', board,\
            '-d', need_checkid]

    print("\n build cmd : %s\n\n" %(cmd))
    (outmsg, exit_code) = run_cmd(cmd)
    print(outmsg)
    if exit_code !=0:
        print('make build_firmware error')
        print(outmsg)
        sys.exit(1)

def build_zephyr_app_by_keil(board, out_dir, app_dir, app_cfg_p, libname, silent):
    if (is_windows()):
        os.chdir(sdk_root)
        build_args = "ninja2mdk.cmd " + board
        build_args += "  " + os.path.relpath(app_dir)
        build_args += "  " + app_cfg_p
        if libname is not None:
            build_args += "  " + libname
        print("\n bulid cmd:%s \n\n" %(build_args))
        ret = os.system(build_args)
        if ret != 0:
            print("\n bulid error\n")
            sys.exit(1)
        print("\n Warning: please build using Keil-MDK! \n\n")
        if silent == False:
            mdk_dir = os.path.join(out_dir, "mdk_clang")
            os.startfile(os.path.join(mdk_dir, "mdk_clang.uvprojx"))
        sys.exit(0)
    else:
        print("\n Error: please build on windows! \n\n")
        sys.exit(1)

def build_zephyr_app_by_gcc(board, out_dir, app_dir, app_cfg_p, libname, silent):

    build_args = "west build -p auto -b " + board
    build_args += " -d " + out_dir + "  " + app_dir
    cfg_file = os.path.join(app_cfg_p, "prj.conf")
    if os.path.exists(os.path.join(app_dir,cfg_file)) == True:
        if (is_windows()):
            cfg_file = cfg_file.replace('\\', '/')
        build_args += " -- " + " -DCONF_FILE=" + cfg_file

    os.chdir(sdk_root)
    print("\n bulid cmd:%s \n\n" %(build_args))
    ret = os.system(build_args)
    if ret != 0:
        print("\n bulid error\n")
        sys.exit(1)


def build_zephyr_menuconfig(out_dir):
    build_args = "west build -d " + out_dir
    build_args += " -t menuconfig"
    print("\n bulid cmd:%s \n\n" %(build_args))
    ret = os.system(build_args)
    if ret != 0:
        print("\n bulid error\n")
        sys.exit(1)

def main(argv):
    global config_conf

    parser = argparse.ArgumentParser()
    parser.add_argument('-n', dest="cfgdel", help="remove cur build_config", action="store_true", required=False)
    parser.add_argument('-c', dest="clear", help="remove board out dir and build_config", action="store_true", required=False)
    parser.add_argument('-m', dest='menuconfig', help="do sdk Configuration", action="store_true", default=False)
    parser.add_argument('-p', dest='pack', help="pack firmware only", action="store_true", default=False)
    parser.add_argument('-l', dest="libname", help="build lib", required=False)
    parser.add_argument('-s', dest="silent", help="silent mode", action="store_true", required=False)
    parser.add_argument('-t', dest="sample", help="build sample, default build applicaction", action="store_true", required=False)
    parser.add_argument('-k', dest="bkeil", help="build by keil, default by gcc ", action="store_true", required=False)
    parser.add_argument('-d', dest="disablecheckid", help="disable check chipid", action="store_true", required=False)

    print("build")
    args = parser.parse_args()

    if args.sample == True:
        sdk_application_dir = sdk_samples_dir
    else:
        sdk_application_dir = sdk_app_dir

    application = ""
    sub_app = ""
    board_conf = ""
    app_conf  = ""
    config_conf = ""

    if args.cfgdel == True:
        if os.path.exists(build_config_file) == True:
            os.remove(build_config_file)
            #sys.exit(0)

    if os.path.exists(build_config_file) == True:
        fp = open(build_config_file,"r")
        lines = fp.readlines()
        for elem in lines:
            if elem[0] != '#':
                _c = elem.strip().split("=")
                if _c[0] == "APPLICATION":
                    application = _c[1]
                elif _c[0] == "BOARD":
                    board_conf  = _c[1]
                elif _c[0] == "SUB_APP":
                    sub_app = _c[1]
                elif _c[0] == "APP_CONF":
                    app_conf = _c[1]
                elif _c[0] == "PRJ_CONF":
                    config_conf = _c[1]
        fp.close


    if os.path.exists(build_config_file) == False:
        board_conf = read_choice("Select boards configuration:", sdk_boards_dir)
        application = read_choice("Select application:", sdk_application_dir)
        sub_app = ""
        app_cfg_dir=os.path.join(sdk_application_dir, application)
        if os.path.exists(os.path.join(app_cfg_dir, "CMakeLists.txt")) == False:
            sub_app = read_choice("Select "+ application+" sub app:", os.path.join(sdk_application_dir, application))
            app_cfg_dir=os.path.join(app_cfg_dir, sub_app)
        if os.path.exists(os.path.join(app_cfg_dir, "CMakeLists.txt")) == False:
            print("application  %s not have CMakeLists.txt\n\n" %(app_cfg_dir))
            sys.exit(0)
        app_conf = ""
        if os.path.exists(os.path.join(app_cfg_dir, "app_conf")) == True:
            app_conf = read_choice("Select application conf:", os.path.join(app_cfg_dir, "app_conf"))
            app_cfg_dir = os.path.join(app_cfg_dir, "app_conf", app_conf);
        elif os.path.exists(os.path.join(app_cfg_dir, "prj.conf")) == False:
            print("application  %s not have prj.conf\n\n" %(app_cfg_dir))
            sys.exit(0)

        config_conf = read_choice_file("Select app prj configuration:", app_cfg_dir, ".conf")

        print(" app_cfg_dir  %s, \n\n" %(app_cfg_dir))

        fp = open(build_config_file,"w")
        fp.write("# Build config\n")
        fp.write("BOARD=" + board_conf + "\n")
        fp.write("APPLICATION=" + application + "\n")
        fp.write("SUB_APP=" + sub_app + "\n")
        fp.write("APP_CONF=" + app_conf + "\n")
        fp.write("PRJ_CONF=" + config_conf + "\n")
        fp.close()


    print("\n--== Build application %s (sub %s) (app_conf %s) (config_conf %s) board %s ==--\n\n"  %(application, sub_app, app_conf, config_conf, 
        board_conf))


    if os.path.exists(os.path.join(sdk_application_dir, application, "CMakeLists.txt")) == True:
        application_path = os.path.join(sdk_application_dir, application)
    else:
        application_path = os.path.join(sdk_application_dir, application, sub_app)

    if os.path.exists(application_path) == False:
        print("\nNo application at %s \n\n" %(sdk_application_dir))
        sys.exit(1)

    if os.path.exists(os.path.join(application_path,  "app_conf")) == True:
        application_cfg_reldir=os.path.join("app_conf", app_conf)
    elif os.path.exists(os.path.join(application_path,  "boards")) == True:
        application_cfg_reldir =os.path.join("boards", board_conf)
    else :
        application_cfg_reldir = "."
        
    #copy app.conf to prj.conf
    if os.path.exists(os.path.join(application_path,  "app_conf")) == True:    
        solution_dir=os.path.join(application_path, application_cfg_reldir)
        solution_conf = os.path.join(solution_dir, config_conf)
        if os.path.exists(solution_conf) == False:
            print("\n app.conf not eixt=%s\n\n"  %(solution_conf))
            sys.exit(1)    
        print(" app_conf  %s, \n\n" %(solution_conf))
        prj_conf = os.path.join(application_path, "prj.conf")
        if os.path.exists(prj_conf) == False:
            shutil.copyfile(solution_conf, prj_conf)		
        elif filecmp.cmp(solution_conf, prj_conf, shallow=False) == False:
            shutil.copyfile(solution_conf, prj_conf)

    print(" application cfg dir %s, \n\n" %(application_cfg_reldir))

    sdk_out = os.path.join(application_path, "outdir_" + config_conf.split('.')[0])
    sdk_build = os.path.join(sdk_out, board_conf)
    if args.clear == True:
        print("\n Clean build out=%s \n\n"  %(sdk_out))
        if os.path.exists(sdk_out) == True:
            shutil.rmtree(sdk_out, True)
            time.sleep(0.01)
        sys.exit(0)

    if args.menuconfig == True:
        if os.path.exists(sdk_build) == True:
            build_zephyr_menuconfig(sdk_build)
        else:
            print("please build")
        sys.exit(1)

    print("\n sdk_build=%s \n\n"  %(sdk_build))
    if os.path.exists(sdk_build) == False:
        os.makedirs(sdk_build)


    board_conf_path = os.path.join(sdk_boards_dir, board_conf)
    if os.path.exists(board_conf_path) == False:
        print("\nNo board at %s \n\n" %(board_conf_dir))
        sys.exit(1)

    #build_zephyr_app(board_conf, sdk_build_boot, sdk_mcuboot_app_path)
    if args.pack == False:
        if args.bkeil == True:
            build_zephyr_app_by_keil(board_conf, sdk_build, application_path, application_cfg_reldir, args.libname, args.silent)
        else:
            build_zephyr_app_by_gcc(board_conf, sdk_build, application_path, application_cfg_reldir, args.libname, args.silent)
            
    if args.disablecheckid == True:
        checkid_en = str(0)
    else:
        checkid_en = str(1)

    build_firmware(board_conf, sdk_build, checkid_en, board_conf_path, application_path, os.path.join(application_path, application_cfg_reldir))
    print("build finished")


if __name__ == "__main__":
    main(sys.argv)
