#  Tutorial 1.2: Variables

Right now our programs have been relatively simple: we've displayed some text and done some math. However, more complicated programs won't be as straightfoward as the example's shown here. We will need some way to keep track of program state, such as what inputs the user has entered, or files downloaded from a server. For this purpose, we use _variables_.

Variables allow us to store and manipulate data. They do this by _binding_ some particular data to a string normally called the variable's name. Variables can hold all sorts of data: numbers, booleans and text.

Here are some examples:

```swift
var number = 3;
var condition = false;
var language = "Floral";
```

A key aspect of variables is their _mutability_. That means that the value of a variable can change after definition. Let's try out an example.

Create a new file called `mutability.floral` and write the following code:

```swift
func main(): Int {
    var number = 41;
    number = 42;
    return number;
}
```

If we run the standard pipeline: compile, execute and then query the exit code, we should get `42`. This is because while the name `number` was bound to the value `41`, we _rebound_ it to a different number.  
