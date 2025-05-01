echo test_mkdir_basic
mkdir td
assert_exit_code

echo test_mkdir_subdir
mkdir td/subdir
assert_exit_code

echo test_touch_basic
touch td/touch_basic.out
assert_exit_code
echo -n > td/touch_basic.expected
assert_exit_code
diff td/touch_basic.out td/touch_basic.expected
assert_exit_code

echo test_echo_existing
echo aaa >td/echo_existing.out
assert_exit_code
echo aa >td/echo_existing.out
assert_exit_code
echo aa >td/echo_existing.expected
assert_exit_code
diff td/echo_existing.out td/echo_existing.expected
assert_exit_code

echo test_echo_new
echo aa > td/echo_new.out
assert_exit_code

echo test_cat_basic
echo -n aaa >td/cat_basic_a.txt
assert_exit_code
echo -n bb >td/cat_basic_b.txt
assert_exit_code
cat td/cat_basic_a.txt td/cat_basic_b.txt > td/cat_basic.out
assert_exit_code
echo -n aaabb > td/cat_basic.expected
assert_exit_code
diff td/cat_basic.out td/cat_basic.expected
assert_exit_code

echo test_shell_single_quotes
e'ch'o h'el'lo '  world' > td/shell_single_quotes.out
assert_exit_code
echo 'hello   world' > td/shell_single_quotes.expected
assert_exit_code
diff td/shell_single_quotes.out td/shell_single_quotes.expected
assert_exit_code

echo test_hd_basic
echo -n abcdefg > td/hd_basic.txt
assert_exit_code
hd td/hd_basic.txt > td/hd_basic.out
assert_exit_code
echo '00000000: 61626364 656667' > td/hd_basic.expected.1
assert_exit_code
echo '00000007:' > td/hd_basic.expected.2
assert_exit_code
cat td/hd_basic.expected.1 td/hd_basic.expected.2 > td/hd_basic.expected
assert_exit_code
diff td/hd_basic.out td/hd_basic.expected
assert_exit_code

echo test_ls_basic
mkdir td/ls_basic
assert_exit_code
touch td/ls_basic/a
assert_exit_code
touch td/ls_basic/b
assert_exit_code
ls td/ls_basic > td/ls_basic.out
assert_exit_code
echo a > td/ls_basic.expected.1
assert_exit_code
echo b > td/ls_basic.expected.2
assert_exit_code
cat td/ls_basic.expected.1 td/ls_basic.expected.2 > td/ls_basic.expected
assert_exit_code

echo test_truncate_basic
echo aaa > td/truncate_basic.out
assert_exit_code
truncate td/truncate_basic.out
assert_exit_code
echo -n > td/truncate_basic.expected
assert_exit_code
diff td/touch_basic.out td/truncate_basic.expected
assert_exit_code

echo test_sysfs_pciinfo
cat /sys/pciinfo > td/sysfs_pciinfo.out
assert_exit_code

echo test_sysfs_meminfo
cat /sys/meminfo > td/sysfs_meminfo.out
assert_exit_code

echo test_sysfs_nvme
cat /sys/nvme > td/sysfs_nvme.out
assert_exit_code

echo All tests successful
