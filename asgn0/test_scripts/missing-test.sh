text="test_files/dogs.jpg"
output=$(./split $text 2>&1)

# Check if the output contains an error message about a missing delimiter
if [[ $output == *"Not enough arguments"* ]]; then
    exit 0
else
    exit 1
fi
