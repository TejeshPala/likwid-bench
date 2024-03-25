# Code style

- Use 4 spaces instead of tabs
- All variables are initialized in the function header:
  - `int myvar = 0;`
  - `bstring mystr = NULL;`
- Exceptions to previous rule only if inside a code block`{ ... }`
- Functions get
  - prefixed with `_` if they are only used internally
  - a `static` keyword if only used inside the compilation unit (file)
- Function always return the error code as `int`. All outputs are pointers in the argument list:
  `int <function_name>(<type> inputarg1, <type> inputarg2, ..., <type>* outputarg1, <type>* outputarg2)`
- Functions, loops, etc. use this style (curly brackets in separate lines):
  ```
  for (int i = 0; i < end; i++)
  {
    <body>
  }
  ```

# Git workflow

For every new topic, create a branch, work in there and push the branch upsteam when done

```
$ git checkout -b <branchname>
$ <work>
$ git commit ...
$ <work>
$ git commit ...
$ git push -u origin <branchname>
```

When the topic is done, create a pull/merge request with
- Delete branch after merge
- Squash commits to a single message

Delete the branch `<branchname>` locally: `git branch -D <branchname>`.

