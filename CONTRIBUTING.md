Contributing to KISO
--------------------

Welcome and thank you for considering contributing to KISO.
[GitHub issues](https://github.com/TheMonHub/kiso/issues/) is
your best friend when it comes to coordinating with each other.
Let's get started on it, assuming you know what you're doing.

Naming Convention
-----------------

| Type                 | Convention                 |
|----------------------|----------------------------|
| File and folder name | snake_case                 |
| Local variable       | snake_case                 |
| Function argument    | snake_case                 |
| Function name        | PascalCase                 |
| Constexpr variable   | PascalCase with "k" prefix |
| Enum entry           | PascalCase with "k" prefix |
| Macro                | SCREAMING_SNAKE_CASE       |
| Function-like macro  | PascalCase                 |
| Type definition      | PascalCase                 |

License Header
--------------

Every file that you made should have a license header containing the
following below:

```c++
// Copyright [YEAR] [YOUR NAME]
// SPDX-License-Identifier: Apache-2.0
```

> [!NOTE]
> Change [YEAR] to the year that the file was made in, and if the
> current year is no longer the same as the file created, you should
> add `-[CURRENT YEAR]` to it.

Header Guard
------------

Prefer traditional header guard with the name of the file along with
its file extension. Add a comment at `#endif` containing the name.

```c++
#ifndef FILE_NAME_H
#define FILE_NAME_H

// includes

// codes

#endif // FILE_NAME_H
```

clang-tidy
---------

You will need to run clang-tidy to lint your code. All clang-tidy
issues should be addressed

You can run the check through CMake directly by turning on
`RUN_CLANG_TIDY`.

If the linter complains about something intentional, you may disable
lint for that certain part of code with the comments below. Always
be explicit about the check you are disabling. If available, prefer
fixing the problem than disabling the check.

```c++
// NOLINT(check-to-disable)
// NOLINTNEXTLINE(check-to-disable)

// NOLINTBEGIN(check-to-disable)
// NOLINTEND(check-to-disable)
```

Code Quality
------------

> [!IMPORTANT]
> We don't accept any vibecoded or low-effort PRs.

The code should be easy to read, understand, and change. Prefer
having the code readable instead of having a document to explain.
Formatters are not required; only the linter, clang-tidy is needed in
order for the PR to get accepted. Any compiler warning should be
fixed when possible.

Comments are allowed and should be used to explain about unclear heap
object ownership, questionable part code that is intended, or used
to note things that the code didn't explain.

The PR should explain what has been changed and a summary on how it
is changed.

Any dependency that gets introduced in the PR should be added to
[NOTICE](NOTICE) and be discussed.

> [!TIP]
> Consider making a new issue in GitHub issues first before doing
> working on the PR or making the PR if it is regarding new
> features or additions.