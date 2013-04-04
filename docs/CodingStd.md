# Coding Standards

[Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml) is adopted, with the following changes:

* Declaration Order: accessors for private data members should be placed before other methods
* C++11: all features [available](http://wiki.apache.org/stdcxx/C++0xCompilerSupport) in both gcc 4.6 and clang 3.1 are allowed
* Enumerator Names: named like constants, and use enum class declaration so that the names are scoped
* File Comments: not really necessary
* Line Length: no hard limit, but avoid more than 80 chars in general

Other rules are:

* in C++ code, use nullptr to indicate null pointer, not NULL or 0;
  in C code, use NULL, not 0
* call instance methods of current class with `this->Method(..)` instead of `Method(..)`,
  use instance variables of current class with `this->varname_` instead of `varname_`,
  so it's clear that's an instance member
* call static methods with `ClassName::StaticMethod(..)` instead of `instance->StaticMethod(..)` or `StaticMethod(..)`,
  so it's clear that's a static member
* prefer a switch statement to if - else if - else statements that tests on the same variable

## Commit Requirements

To commit to the master branch, the code must compile (without errors or warnings) on major supported platforms, and must not break any existing test cases (no test case can go from passing to failure because of this commit).

Repo is not a method to sync code between machines. Use rsync or [Dropbox](http://bit.ly/dboxapp) for this purpose.

If you need another member to review your code, use git diff command to create a patch from an existing pushed commit, and email that patch.


