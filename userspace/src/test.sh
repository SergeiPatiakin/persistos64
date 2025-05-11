set -e

echo test_mkdir_basic
mkdir td

echo test_mkdir_subdir
mkdir td/subdir

echo test_mkdir_exists
set +e
mkdir td/subdir 2>td/mkdir_exists.err
assert_exit_code 2
set -e
echo 'mkdir: Error in mkdir' > td/mkdir_exists.err.exp
diff td/mkdir_exists.err td/mkdir_exists.err.exp

echo test_touch_basic
touch td/touch_basic.out
echo -n > td/touch_basic.out.exp
diff td/touch_basic.out td/touch_basic.out.exp

echo test_diff_length
echo -n a > td/diff_a
echo -n aa > td/diff_aa
set +e
diff td/diff_a td/diff_aa > td/diff_length.out
assert_exit_code 2
set -e
echo 'Files differ, they have different lengths' > td/diff_length.out.exp
diff td/diff_length.out td/diff_length.out.exp

echo test_diff_differs
echo -n a > td/diff_a
echo -n b > td/diff_b
set +e
diff td/diff_a td/diff_b > td/diff_differs.out
assert_exit_code 2
set -e
echo 'Files differ' > td/diff_differs.out.exp
diff td/diff_differs.out td/diff_differs.out.exp

echo test_echo_existing
echo aaa >td/echo_existing.out
echo aa >td/echo_existing.out
echo aa >td/echo_existing.out.exp
diff td/echo_existing.out td/echo_existing.out.exp

echo test_echo_new
echo aa > td/echo_new.out

echo test_cat_basic
echo -n aaa >td/cat_basic_a.txt
echo -n bb >td/cat_basic_b.txt
cat td/cat_basic_a.txt td/cat_basic_b.txt > td/cat_basic.out
echo -n aaabb > td/cat_basic.out.exp
diff td/cat_basic.out td/cat_basic.out.exp

echo test_shell_single_quotes
e'ch'o h'el'lo '  world' > td/shell_single_quotes.out
echo 'hello   world' > td/shell_single_quotes.out.exp
diff td/shell_single_quotes.out td/shell_single_quotes.out.exp

echo test_hd_basic
echo -n abcdefg > td/hd_basic.txt
hd td/hd_basic.txt > td/hd_basic.out
echo '00000000: 61626364 656667' > td/hd_basic.out.exp.1
echo '00000007:' > td/hd_basic.out.exp.2
cat td/hd_basic.out.exp.1 td/hd_basic.out.exp.2 > td/hd_basic.out.exp
diff td/hd_basic.out td/hd_basic.out.exp

echo test_ls_basic
mkdir td/ls_basic
touch td/ls_basic/a
touch td/ls_basic/b
ls td/ls_basic > td/ls_basic.out
echo a > td/ls_basic.out.exp.1
echo b > td/ls_basic.out.exp.2
cat td/ls_basic.out.exp.1 td/ls_basic.out.exp.2 > td/ls_basic.out.exp

echo test_truncate_basic
echo aaa > td/truncate_basic.out
truncate td/truncate_basic.out
echo -n > td/truncate_basic.out.exp
diff td/touch_basic.out td/truncate_basic.out.exp

echo test_sysfs_pciinfo
cat /sys/pciinfo > td/sysfs_pciinfo.out

echo test_sysfs_meminfo
cat /sys/meminfo > td/sysfs_meminfo.out

echo test_sysfs_nvme
cat /sys/nvme > td/sysfs_nvme.out

echo All tests successful
