echo test_mkdir_basic
mkdir td

echo test_mkdir_subdir
mkdir td/subdir

echo test_touch_basic
touch td/touch_basic.out
echo -n > td/touch_basic.expected
diff td/touch_basic.out td/touch_basic.expected

echo test_echo_existing
echo aaa >td/echo_existing.out
echo aa >td/echo_existing.out
echo aa >td/echo_existing.expected
diff td/echo_existing.out td/echo_existing.expected

echo test_echo_new
echo aa > td/echo_new.out

echo test_cat_basic
echo -n aaa >td/cat_basic_a.txt
echo -n bb >td/cat_basic_b.txt
cat td/cat_basic_a.txt td/cat_basic_b.txt > td/cat_basic.out
echo -n aaabb > td/cat_basic.expected
diff td/cat_basic.out td/cat_basic.expected

echo test_shell_single_quotes
e'ch'o h'el'lo '  world' > td/shell_single_quotes.out
echo 'hello   world' > td/shell_single_quotes.expected
diff td/shell_single_quotes.out td/shell_single_quotes.expected

echo test_hd_basic
echo -n abcdefg > td/hd_basic.txt
hd td/hd_basic.txt > td/hd_basic.out
echo '00000000: 61626364 656667' > td/hd_basic.expected.1
echo '00000007:' > td/hd_basic.expected.2
cat td/hd_basic.expected.1 td/hd_basic.expected.2 > td/hd_basic.expected
diff td/hd_basic.out td/hd_basic.expected

echo test_ls_basic
mkdir td/ls_basic
touch td/ls_basic/a
touch td/ls_basic/b
ls td/ls_basic > td/ls_basic.out
echo a > td/ls_basic.expected.1
echo b > td/ls_basic.expected.2
cat td/ls_basic.expected.1 td/ls_basic.expected.2 > td/ls_basic.expected

echo test_truncate_basic
echo aaa > td/truncate_basic.out
truncate td/truncate_basic.out
echo -n > td/truncate_basic.expected
diff td/touch_basic.out td/truncate_basic.expected

echo test_sysfs_pciinfo
cat /sys/pciinfo > td/sysfs_pciinfo.out

echo test_sysfs_meminfo
cat /sys/meminfo > td/sysfs_meminfo.out

echo test_sysfs_nvme
cat /sys/nvme > td/sysfs_nvme.out

echo All tests successful
