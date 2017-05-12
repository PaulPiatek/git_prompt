# git_prompt
Fast "git prompt" generator for use in command line prompt

Depends on gitlib2

I used several git prompt generators which are written in shell (bash/zsh) and call git status and parse the output with several calls to sed/whatever. For me it was to slow, so I implemented this prompt generator which uses gitlib2 and runs noticeably faster.
