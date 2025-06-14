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
diff td/mkdir_exists.err test/mkdir_exists.err.exp

echo test_touch_basic
touch td/touch_basic.out
diff td/touch_basic.out test/touch_basic.out.exp

echo test_diff_length
set +e
diff test/diff_a.txt test/diff_aa.txt > td/diff_length.out
assert_exit_code 2
set -e
diff td/diff_length.out test/diff_length.out.exp

echo test_diff_differs
set +e
diff test/diff_a.txt test/diff_b.txt > td/diff_differs.out
assert_exit_code 2
set -e
diff td/diff_differs.out test/diff_differs.out.exp

echo test_echo_existing
echo aaa >td/echo_existing.out
echo aa >td/echo_existing.out
diff td/echo_existing.out test/echo_existing.out.exp

echo test_echo_new
echo aa > td/echo_new.out

echo test_cat_basic
cat test/cat_basic_a.txt test/cat_basic_b.txt > td/cat_basic.out
diff td/cat_basic.out test/cat_basic.out.exp

echo test_shell_single_quotes
e'ch'o h'el'lo '  world' > td/shell_single_quotes.out
diff td/shell_single_quotes.out test/shell_single_quotes.out.exp

echo test_shell_script
shell test/hello.sh > td/shell_script.out
diff td/shell_script.out test/shell_script.out.exp

echo test_hd_basic
hd test/hd_basic.txt > td/hd_basic.out
diff td/hd_basic.out test/hd_basic.out.exp

echo test_ls_basic
mkdir td/ls_basic
touch td/ls_basic/a
touch td/ls_basic/b
ls td/ls_basic > td/ls_basic.out
diff td/ls_basic.out test/ls_basic.out.exp

echo test_truncate_basic
echo aaa > td/truncate_basic.out
truncate td/truncate_basic.out
diff td/touch_basic.out test/truncate_basic.out.exp

echo test_sysfs_pciinfo
cat /sys/pciinfo > td/sysfs_pciinfo.out

echo test_sysfs_meminfo
cat /sys/meminfo > td/sysfs_meminfo.out

echo test_sysfs_nvme
cat /sys/nvme > td/sysfs_nvme.out

echo test_dd_basic
dd if=test/dd_basic.txt of=td/dd_basic.out bs=2 skip=3 count=4 seek=5
diff td/dd_basic.out test/dd_basic.out.exp

echo test_ps_basic
ps > td/ps_basic.out
dd if=td/ps_basic.out of=td/ps_basic_trunc.out bs=32 count=1
diff td/ps_basic_trunc.out test/ps_basic_trunc.out.exp

echo test_fault_div_zero
set +e
fault div-zero
assert_exit_code 136
set -e

echo test_fault_priv_instr
set +e
fault priv-instr
assert_exit_code 132
set -e

echo test_fault_bad_read
set +e
fault bad-read
assert_exit_code 139
set -e

echo test_fault_bad_write
set +e
fault bad-write
assert_exit_code 139
set -e

echo test_sleep_smoke
sleep 10

echo All tests successful
