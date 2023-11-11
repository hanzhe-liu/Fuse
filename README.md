This repository contains a basic implementation of a FUSE (Filesystem in Userspace) file system.

Overview:
The provided code defines a simple virtual file system where users can create, read, and write to directories and files. 
Additionally, a unique feature is implemented where files with the .revert extension will revert any changes made to the 
original file. This means that a rollback can be achieved via .revert. The logic of rollback is implemented in read. So 
essentially, when a file ending with .revert is read, the contents of the previous version of the file and the contents 
of the current version of the file are swapped. Another thing to note is that this simple file system can only save the 
contents of one past version of the file.

Example of how to use revert function:
Rolling back changes: If you decide to undo modifications to the example file and return to its previous state, simply read 
example.revert. This will trigger the rollback mechanism to restore the file contents to the last saved state.
For example, you can rollback by running cat:

cat example.txt.revert

Afterwards, the contents of example.txt will be restored to their previous state. And the contents of the current version of 
example.txt will be saved for rollback.

Compilation:
To compile the code: make / make all
To run the file system: make run
To clean up the compiled files: make clean