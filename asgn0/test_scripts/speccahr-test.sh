special_char_file = "test_files/-rickrolled.txt" 
diff <(./split a "$special_char_file") <(./rsplit a "$special_char_file") > /dev/null
