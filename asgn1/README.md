
# Assignment 1 Resources

This directory contains resources for assignment 1:

* tests/ A directory containing test scripts for evaluating your
  assignment.

* load_repo.sh a script that can be used to load your asgn1 for testing

* test_repo.sh a script that can be used to test your repository


## Using these Resources

1. Copy the tests to your repository using the supplied load_repo.sh
   script:

```
./load_repo.sh {path_to_asgn1_dir}
```

where {path_to_asgn1_dir} is the path to your asgn1 directory.

2. Go to your asgn1 directory:

```
    cd {path_to_asgn1_dir}
```

3. Make your memory binary:

```
    make
````

4. execute test_repo.sh:

```
    ./test_repo.sh
```

This command will print out each test and whether it passed or failed.
If the test passed, you will see a message saying "SUCCESS"; if it
fails you will see a message saying "FAILURE".

You can count the number of correct tests by executing:

```
    ./test_repo.sh | grep "SUCCESS" | wc -l
```

if the output is 25, then congrats!  You've passed all of our tests.
