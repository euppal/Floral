#  Tutorial 1.1: The Main Function

At the heart of every floral program is the `main()` function. You might remember it from the previous tutorial, it was that thing with the word `func` (short for "function" and the curcly braces).

```swift
func main(): Int {
    return 0;
}
```

You might also notice the `: Int` after the `main()`. This is called the return type of the function. `Int` is short for "integer", meaning this function returns a positive or negative number. In the case of `main()`, it is the _exit code_ that your program terminates with. To view the exit code of a program simply run `echo $?` after executing a program.

0 is usually an exit code indicating success, and 1 usually indicates failure. However, those are just conventions, and the return value can essentially be any number. To try this out, let's perform some arithmetic.

In a new file `return-value.floral` enter in the following code:

```swift
func main(): Int {
    return 1 + 2 * 3;
}
```

Now, compile and execute.

```
$ floralc -o return-value return-value.floral
$ ./return-value
```

Nothing will occur. However, if we run

```
$ echo $?
```

We should see the result of `7` appear on the screen.
