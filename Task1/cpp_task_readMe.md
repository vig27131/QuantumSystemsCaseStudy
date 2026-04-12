# cpp_task.cpp

## What it does?
It tries to start and run two threads:
    - one increments the loop counter every 2 seconds till timeout of 10 seconds occurs or the running flag becomes false
    - one increments the loop counter every second till timeout of 10 seconds or till loop has run 5 times. 

Thread 1 never aborts but runs till "timeout"
Thread 2 runs only for 5 iterations and then aborts.

## Problems seen:
- Since the two threads share a common running flag, as soon as one of the thread terminates, the other terminates automatically as well. 
If both loops need to run independent of each other, we need to use separate running flags for each of them. 

- Additionally, since "Process" and "timeout" are local to the startThread function, capturing them by reference leads to dangling reference within the lambda.

## Fix: 
- Capture Process and Timeout explicitly as a copy instead of a reference.
- Use separate running flags for each of the threads.

## Expectation: 
Both loops should have a counter value of 5 at the end of the program.

## Result:
C1: 5 C2: 5
