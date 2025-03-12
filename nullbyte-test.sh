text="test_files/dogs.jpg"
diff <(./split $'\0' $text) <(./rsplit $'\0' $text) > /dev/null
