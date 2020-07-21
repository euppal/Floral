# Floral

> Hello world in Floral.
```
func main(): Int {
    print("Hello, world!");
    return 0;
}
```

Floral is a strongly and statically typed compiled programming language inspired by C and Swift. The language is [open source](https://github.com/euppal/floral). It aims to draw on and expand on many existing features within the C family of languages while also providing slightly different syntax as to not lose familiarity but gain individuality. The wiki documents many aspects of the Floral language as well as how to get up and running with it, and as such is a very useful tool. You can find it [here]().

### Example Programs

The following are example Floral programs, each portraying a unique aspect of the language.

```
import IO;

func main(): Int {
    let file = try File.open("Path/To/File.txt") catch { return 1; };
    let data = "Hello, world!".data();
    File.write(data, file);
    File.close(file);
    return 0;
}
```
