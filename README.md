# EXERCICE DESCRIPTION
Implement a server with the following characteristics:

- The server will listen on a port that it will receive as a parameter at startup.
- The clients requests will be strings in plain text with the format "get text n", where 'text' is a string without spaces and 'n' is a positive integer. See examples below.
- The answer for a certain 'text' will be its md5 hash.
- The function that calculates this value must include a sleep of 'n' millisecond to simulate a variable processing time between requests.
- The server will be able to attend several requests concurrently.
- The server will be able to to cache the last C results. This C value will be passed as a parameter at startup.
- When the cache is full and you need to enter new values, those that have been without access for a longer time will be eliminated.
- The server will empty the cache when receiving the SIGUSR1 signal.
- The server will do an orderly shutdown when receiving the SIGTERM signal and before finishing it will show the values cached by STDOUT.
- The usage of utilities from the standard libraries will be valued instead of adding dependencies from external libraries (boost, etc.)
- To compile the code, you can use the C ++ 17 standard or lower.

Examples:

# start the server on port 3456 with a cache of 10 items
$> ./server -p 3456 -C 10

# request examples
$> echo "get test1 3000" | nc localhost 3456
(... 3 seconds later ...)
5a105e8b9d40e1329780d62ea2265d8a

$> echo "get test2 5000" | nc localhost 3456
(... 5 seconds later ...)
ad0234829205b9033196ba818f7a872b

$> echo "get test1 6000" | nc localhost 3456
(immediately, because it is cached)
5a105e8b9d40e1329780d62ea2265d8a

$> kill -USR1 <server pid>
Done!

$> echo "get test1 3000" | nc localhost 3456
(... 3 seconds later ...)
5a105e8b9d40e1329780d62ea2265d8a

$> kill -TERM <server pid>
(test1, 5a105e8b9d40e1329780d62ea2265d8a)
Bye!




# EXERCICE RESOLUTION
The solution consists of a general-purpose library (liblocar), where I have added all software components that can be reused in other projects, a server binary (server) that meets the specifications requested, and a client binary (client) which I have used for testing purposes. To build the binaries, you just have to unzip and run make in the root directory:

make library => Build the library.

make server  => Build the library and the server binary.

make test    => Build the library and the client application for testing.

make all     => Build all binaries: the library, the server and the client.

make clean   => Clean the source code by deleting all the generated objects, the library, the server binary, the client binary and the bin and lib directories.


In addition to the proposed specifications, the solution contains some extra features such as:
- A multi-level trace system.
- A template cache with extra functionality to automatically discard entries based on a temporary age threshold.
- A multithreaded client binary, capable of sending multiple requests with random or parametric values to the server with a high degree of simultaneity.
- Additionally, both the server and client binary can display usage and example information with the -h or --help option.

