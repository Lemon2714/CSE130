empty_file="test_files/empty_file.txt"
diff <(./split $'\n' "$text") <(./rsplit $'\n' "$text") > /dev/null
