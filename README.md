Fuse File system prototype that is intended to delay file creation till file is actually being written. Greater use is for deployment on a distributed file system. 
More details will be updated soon...

Code Acknowledgements
------------------------
1. Code has been based on BBFS FS written by Joseph J. Pfeiffer, Emeritus Professor, New Mexico State University. It has been modified to suit our requirements of implementing a FUSE FS for replication in Lustre.2. The hash map used for our implementation is based on the open source hash table implementation for C by Troy D. Hanson. Thanks to the author for making the "uthash" implementation freely available.
