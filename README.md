# general-tools

A repository to store all software tools for the general systems used for the FOXSI mission that may not be suited to the individual detector system tool repositories.

Several languages might be used in this repository and so the top level will be to contain language specific packages.

For example, the `general-tools-py` folder will be a Python package containing all the necessary `.py` files. If other languages are to be included, like `IDL`, then a folder called `general-tools-idl` should be created and used to contain all the `IDL`-ness of the repository. This standard can be applied to the inclusion of other languages (e.g., C++ as `general-tools-cpp`, etc.).

Every `general-tools-<?>` folder should have an "examples" and a "tests" folder. The "examples" folder is a great place to include scripts that show how some of the code in the repository works and the "tests" folder is a fantastic place to put code that tests the tools that have been created.

Additionally, there is also an "examples" and "tests" folder in the top level of the repository so there is a place for anything that fits these folders that spans across code from multiple languages.

The `external` directory is a place for software external to the repository. There is no guarentee that it will follow any specific coding language and so it would perhaps not be ideal to place it in a specific coding language `tools` directory. Some may be [`git` submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules).
