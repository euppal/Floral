# Floral Projects

The Floral compiler is great for single, simple and short scripts and even 2, 3 or 4 files of code. However, once you are on the scale of refactoring that you have 5 separate files for your program, it's probably smart to start considering changing your setup from manual floralc invocation to a floral project.

#### Default Project Layout

With `floralc -new-project "My Project"` a default project is created under the directory `My Project`.

```
/src/floral/
/src/include/
/target/debug/
/target/release/
/internal/objects/
/internal/old/
/internal/filechange.txt
/project.toml
```

The `project.toml` file - or more simply - the project file, contains all the necessary build information to build your project, such as the location of the source code, what files to ignore and accept, libraries linked, etc.

Once done, navigate to the directory with the project file and type `floralc -project-build`. This will save the resulting executable to `/target/${debug or release}/${project name}`. All other intermediary files (such as `.nasm` and `.o` files) are placed in the `/internal/objects/` directory.

`floralc -project-run` is the same as `-project-build` except it subsequently runs the generated executable.

If a source file is changed, that change is marked in `/internal/filechange` and any changed files will be recompiled. 