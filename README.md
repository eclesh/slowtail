slowtail
========

slowtail prints any lines of a file that have appeared since the last
time slowtail ran. It is useful in cron jobs that need to keep track
of where they left off reading a file.

slowtail was inspired by logtail from the logcheck project[1]. For an
example use case, see Etsy's logster[2].

[1] http://logcheck.org
[2] https://github.com/etsy/logster

Quick start
===========

	$ make
	gcc -o slowtail slowtail.c
	$ ./slowtail access.log

License
=======

slowtail is licensed under the MIT license.
