#  Tutorial 1.3: Constants - Local and Global

So far we have learned one way of storing and manipulating data: variables. However, there is another way. Suppose you are creating a game with physics. You have a character and you want to declare the gravity of the world so the whole program knows what it is. You might try something like:

```swift
var gravity = 9.8;

func game() { ... }
```

However, if you try and build this, you will get a compile-time error. This is floral hinting you that maybe you should consider another option. Why? Think about this: a global variable can be modified by any of your code at any time.
