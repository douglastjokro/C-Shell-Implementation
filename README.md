# C Shell Implementation
This repository contains two projects implemented in C, both of which involve building a shell called "penn-shredder". The first project focuses on implementing a basic shell that will restrict the runtime of executed processes. The second project builds upon the first by adding additional features, including redirection and a two-stage pipeline.

## Project 1 - Basic Shell
In this project, we have implemented a basic shell called "penn-shredder" that reads input from users and executes it as a new process. However, the shell will be killed if the process exceeds a timeout. The files included for this project are:
- **penn-shredder.c**: the main source code file for the shell
- **makefile**: a makefile to build the penn-shredder binary

## Project 2 - Enhanced Shell
In this project, we have enhanced the penn-sh shell created in Project 1. The enhanced shell has two additional features: redirection and a two-stage pipeline. We no longer need to deal with the timer alarm. The files included for this project are:

- **token-shell.c**: the main source code file for the enhanced shell
- **tokenizer.c**: a source file that contains functions for tokenizing input strings
- **tokenizer.h**: a header file for the tokenizer
- **makefile**: a makefile to build the enhanced penn-sh binary
- **pipe.c**: a source file that contains functions for setting up and executing pipelines
- **pipe.h**: a header file for the pipeline functions

## Credits
This program was developed by Douglas Tjokrosetio.

## License
This project is licensed under the MIT License - see the LICENSE file for details.
