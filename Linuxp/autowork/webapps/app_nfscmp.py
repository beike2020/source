#!/usr/bin/env python

import os
import re
import sys
import shutil
import filecmp
import difflib

holderlist = []
 
text1 = """text1:
This module provides classes and functions for comparing sequences.
including HTML and context and unified diffs.
difflib document v7.4
add string
"""
text2 = """text2:
This module provides classes and functions for Comparing sequences.
including HTML and context and unified diffs.
difflib document v7.5"""


def diff_patch():
    d = difflib.Differ()
    diff = d.compare(text1_lines, text2_lines)
    print '\n'.join(list(diff))


def diff_htmls():
    d = difflib.HtmlDiff()
    print d.make_file(text1_lines, text2_lines)


def cmp_detail(dir1, dir2):
    dirobj = filecmp.dircmp(dir1, dir2, ['test.py'])
    dirobj.report()
    dirobj.report_partial_closure()
    dirobj.report_full_closure()
    print "left_list:" + str(dirobj.left_list)
    print "right_list:" + str(dirobj.right_list)
    print "common:" + str(dirobj.common)
    print "left_only:" + str(dirobj.left_only)
    print "right_only:" + str(dirobj.right_only)
    print "common_dirs:" + str(dirobj.common_dirs)
    print "common_files:" + str(dirobj.common_files)
    print "common_funny:" + str(dirobj.common_funny)
    print "same_file:" + str(dirobj.same_files)
    print "diff_files:" + str(dirobj.diff_files)
    print "funny_files:" + str(dirobj.funny_files)


def compareme(dir1, dir2):
    cmp_detail(dir1, dir2)
    dircomp = filecmp.dircmp(dir1, dir2)
    only_in_one = dircomp.left_only
    diff_in_one = dircomp.diff_files
    dirpath = os.path.abspath(dir1)
    [holderlist.append(os.path.abspath(os.path.join(dir1, x))) for x in only_in_one]
    [holderlist.append(os.path.abspath(os.path.join(dir1, x))) for x in diff_in_one]
    if len(dircomp.common_dirs) > 0:
        for item in dircomp.common_dirs:
            compareme(os.path.abspath(os.path.join(dir1, item)), \
            os.path.abspath(os.path.join(dir2, item)))
        return holderlist


def main():
    if len(sys.argv) > 2:
        dir1 = sys.argv[1]
        dir2 = sys.argv[2]
    else:
        print "Usage: ", sys.argv[0], " datadir backupdir"
        sys.exit()
 
    source_files = compareme(dir1,dir2)
    dir1 = os.path.abspath(dir1)

    if not dir2.endswith('/'): 
        dir2 = dir2+'/'
    dir2 = os.path.abspath(dir2)
    destination_files = []
    createdir_bool = False

    for item in source_files:
        destination_dir = re.sub(dir1, dir2, item)
        destination_files.append(destination_dir)
        if os.path.isdir(item):
            if not os.path.exists(destination_dir):
                os.makedirs(destination_dir)
                createdir_bool = True

    if createdir_bool:
        destination_files = []
        source_files = []
        source_files = compareme(dir1, dir2)
        for item in source_files:
            destination_dir = re.sub(dir1, dir2, item)
            destination_files.append(destination_dir)

    print "update item:"
    print source_files 

    copy_pair = zip(source_files,destination_files)
    for item in copy_pair:
        if os.path.isfile(item[0]):
            shutil.copyfile(item[0], item[1])
 

if __name__ == '__main__':
    main()
    text1_lines = text1.splitlines()
    text2_lines = text2.splitlines()
    diff_patch(text1_lines, text2_lines)
    diff_htmls(text1_lines, text2_lines)
