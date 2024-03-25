# BST-Amazon
The overall objective of the project will be to implement a service for storing key-value pairs (along the lines of the java.util.Map interface of the Java API) similar to that used by Amazon to support your web services.

# CONTENTS
    * Introduction
    * Usage
    * Contributors

# INTRODUCTION

The general objective will be to implement a peer storage project service
key-value (similar to the java.util.SortedMap interface of the Java API) similar to the one used
by Amazon to support its web services. The data structure used to store this information is a
binary search tree, used given its research efficiency.

# USAGE

In order to run the project you must use the command "make" and then use ./binary/tree_client argument after run ./binary/tree_server argument

arguments:
run server: port number = <port> 127.0.0.1:2181
run client: ip = 127.0.0.1:2181 

# LIMITATIONS

In case of error when using this program, it should be restarted until it works.
Servers may find difficult to connect to the next_server, in case it happens, restart.
Operations as put <key> <value> may not work, in case it happens, restart.

# CONTRIBUTORS

Group SD-050
 João Santos nº 57103
 Paulo Bolinhas nº 56300
 Rui Martins nº 56283

-------------------------------------------------------------------------------------------------
