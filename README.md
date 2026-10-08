# git_prompt

Fast "git prompt" generator for use in a command line prompt, written in C++.

Depends on [libgit2](https://libgit2.org/).

I used several git prompt generators which are written in shell (bash/zsh) and
call `git status` and parse the output with several calls to `sed`/whatever.
For me it was too slow, so I implemented this prompt generator which uses
libgit2 and runs noticeably faster.

