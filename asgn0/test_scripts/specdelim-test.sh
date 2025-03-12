special_char_file="test_files/dogs.jpg"
delimiter='$'  # Assuming '$' is the special character you want to use as a delimiter

output=$(./split "$delimiter" "$special_char_file" 2>&1)

expected_output=$(./rsplit "$delimiter" "$special_char_file" 2>&1)

if [[ $output == "$expected_output" ]]; then
    exit 0
else
    exit 1
fi
