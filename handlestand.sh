text="test_files/dogs.jpg"
diff <(./split a $text) <(./rsplit a $text) > /dev/null
