#  Tutorial 1.0: Hello World

As is the tradition in programming, we will create a simple hello world program.

Open up a text editor and create a new file named `myhello.floral`. In your editor, enter the following code:

```swift
#include <io.fh>

func main(): Int {
    print("Hello, world!\n");
    return 0;
}
```

There might be a lot here you don't understand, but stick with it. By the end of these tutorials you will comfortably know all these elements.

Launch a shell, navigate to the directory the code file is in using `cd`, and run

```
$ floralc -o myhello myhello.floral
$ ./myhello
```

If all goes to plan, you should see the words "Hello, world!" appear on your screen. 
