'\"
'\" This file contains documentation for the prefix daemon
'\"
'\" $Id: prefix.ms,v 1.1 89/07/10 03:46:17 adam Exp $
'\"
'\"
'\" xH is a macro to provide numbered headers that are automatically stuffed
'\" into a table-of-contents, properly indented, etc. If the first argument
'\" is numeric, it is taken as the depth for numbering (as for .NH), else
'\" the default (1) is assumed.
'\"
'\" @P The initial paragraph distance.
'\" @Q The piece of section number to increment (or 0 if none given)
'\" @R Section header.
'\" @S Indent for toc entry
'\" @T Argument to NH (can't use @Q b/c giving 0 to NH resets the counter)
.de xH
.nr @Q 0
.ds @T
'\" This stuff exercises a bug in nroff. It used to read
'\" .ie \\$1, but if $1 was non-numeric, nroff would process the
'\" commands after the first in the true body, as well as the
'\" false body. Why, I don't know. The bit with @U is a kludge, and
'\" the initial assignment of 0 is necessary
.nr @U 0
.nr @U \\$1
.ie \\n(@U>0  \{\
.	nr @Q \\$1
.	ds @T \\$1
.	ds @R \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
'\}
.el .ds @R \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
.nr @S (\\n(@Q-1)*5
.nr @P \\n(PD
.ie \\n(@S==-5 .nr @S 0
.el .nr PD 0
.NH \\*(@T
\\*(@R
.XS \\n(PN \\n(@S
\\*(SN \\*(@R
.XE
.nr PD \\n(@P
..
'\" CW is used to place a string in fixed-width or switch to a
'\" fixed-width font.
'\" C is a typewriter font for a laserwriter. Use something else if
'\" you don't have one...
.de CW
.ie !\\n(.$ .ft C
.el \&\\$3\fC\\$1\fP\\$2
..
'\" Anything I put in a display I want to be in fixed-width
.am DS
.CW
..
.de Bp
.ie !\\n(.$ .IP \(bu 2
.el .IP "\&" 2
..
.po +.3i
.RP
.TL
Prefix \*- Painless Filesystem Management
.AU
Adam de Boor
.AI
Berkeley Softworks
2150 Shattuck Ave, Penthouse
Berkeley, CA 94704
adam@bsw.uu.net
\&...!uunet!bsw!adam
.AB
.FS "\&
\(co Copyright Adam de Boor and Berkeley Softworks, 1989.
Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appears in all copies.
Neither Berkeley Softworks, nor Adam de Boor makes any
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.
.FE
This paper describes the installation, use and theory of operation of
.CW prefix ,
a simple daemon to implement a ``prefix table'' for
.UX
using NFS\(tm. The daemon provides for a less painful filesystem
management scheme than the current static configuration file and mount
table scheme, allowing for the dynamic mounting, location and unmounting of
filesystems according to user demand. The intent of the daemon is to
provide a globally consistent filesystem for use with PMake and Customs.
.AE
.nr PD .1i
'\"
'\"		INTRODUCTION
'\"
.xH Introduction
.LP
The ``prefix'' program implements a ``prefix table'' for
.UX
running NFS. ``What's a prefix table?'' you ask. A prefix table is a
less administration-intensive way of maintaining network filesystems,
relying on the network to provide the location of a particular
filesystem, rather than a static configuration file (/etc/fstab) as is
done now. In addition, the set of mounted systems is dynamic, with
unneeded systems automatically unmounted and mounted again when
necessary. This dynamic configuration helps to reduce the chances of a
client hanging because the server of some unneeded filesystem has gone down.
.LP
The implementation revolves around the notion of a ``prefix'' \*- a
unique name for a particular filesystem that is common across all
systems on the local network. The ``prefix'' daemon controls access to
these prefixes. When a client wishes to access a file in a prefixed
filesystem, the daemon broadcasts to the local network to find out
what machine is currently serving the prefix, accepting the first
responder as the machine whose proferred filesystem it will use. This
allows a prefix to be served by more than one host, with the one most
able to serve the prefix (i.e. the one that can respond the fastest)
being the one chosen. After determining the server of the prefix, the
daemon mounts the appropriate filesystem from the server and the
client's request passes on to the server to be handled.
.LP
It is important to note that this daemon does
.B not
bypass the normal access controls provided by NFS. While anyone can
tell the local prefix daemon that a particular directory is to be
exported under a given prefix, this doesn't preclude the local mount
daemon from refusing access based on the /etc/exports file.
.LP
While there were several goals considered in the writing of this
daemon, the primary one was the providing of a globally consistent
filesystem across all client machines to allow PMake and Customs to
operate without concern. That it is also helpful for users and
administrators in general is a pleasant by-product. Because of this
goal, and the fairly trusting atmosphere already assumed by Customs,
the service is not as elaborate as it might be, though I doubt not
that it will evolve over time as it is used in more diverse environments.
.LP
``prefix'' was inspired by the
.CW automount
program that comes with the latest Sun OS 4.0 release (4.0.3). It
operates rather differently, however, in that automounted filesystems
are mounted \fIin place\fP, rather than under some temporary mount
directory. This is required by PMake's need for a globally consistent
filesystem. The problem lies in the
.CW getwd
library function. If the directory
.CW /n/promethium
were mounted on
.CW /tmp_mnt/n/promethium ,
as automount would like, getwd would return a path within the /tmp_mnt
directory. This is ok, as far as local work goes, since /tmp_mnt is in
fact where the files are to be found. Unfortunately, if PMake exports
a job somewhere, it will pass along this path to the remote job and
Customs will attempt to change to that directory, an attempt that will
fail unless someone on the remote machine has also caused the
file system to be automounted. With ``prefix,'' however, the file
system will always be in the same place, or will be automounted if it
is not, and getwd will return the proper value.
'\"
'\"		OPERATION
'\"
.xH Operation
.LP
The prefix daemon has two jobs to perform:
.RS
.Bp
Export prefixes to other machines, providing the local directory name
in response to a request from a remote prefix daemon.
.Bp
Import prefixes from other machines, locating their servers and
mounting them for normal NFS access on demand, unmounting them when
they are no longer required.
.RE
.LP
The first task is simple to perform and quickly explained. Any
directory on the system may be declared to be the server for a prefix.
When a request arrives for the given prefix, the daemon will respond
with the directory. To ensure against ``operator error,'' prefix will consult
.CW /etc/exports
to make sure the directory is actually exported, warning you if it
can't locate the directory. Note that no attention is paid to the
export restrictions that may be placed in the file. This is arguably a
bug, as it could cause a remote system to fail in mounting a prefix
that it could actually get from somewhere else simply because the
restricted host responded first.
.LP
Imported prefixes are divided into two categories: ``root'' and
``imported'' prefixes.
.LP
An ``imported'' prefix is a real prefix available on the network. As
such, prefix will broadcast for the prefix's server whenever you try
to lookup anything in that directory.
.LP
A ``root'' prefix, on the other hand, is internal to the machine,
being the root of a tree of prefixes. Anything sought by the kernel
immediately below one is assumed to be an imported prefix, to be sought
on the network should the kernel attempt to locate anything inside it.
For example, if you have a network of machines with local disks and
you want the main partition of each to be accessible as
.CW /n/ \fImachine\fP,
declaring
.CW /n
to be a root prefix will cause the proper thing to happen.
.LP
Why this difference? It's more flexible, since prefix must be informed
of each prefix it should import (not being party to the name lookup
performed by the kernel). It is much easier to have a few, well-known
root prefixes under which needed directories can be exported than to
have each prefix entered in a configuration file on each client
system. One of the purposes of this daemon is, after all, to get away
from a static configuration file.
'\"
'\"		OPTIONS
'\"
.xH Options
.LP
``prefix'' accepts several command-line options that dictate its actions:
.IP "\-D[D]                  "
Enter daemon mode. This includes detaching from its parent. If the
second D is given, prefix will enter its debugging mode.
.IP "\-d \fIprefix\fP	    "
Causes the imported or exported prefix to be deleted. If the prefix is
imported and mounted, it will be unmounted first. If the prefix is a
root prefix, all mounted subprefixes will be unmounted first. If any
unmounting fails, the prefix will not be deleted.
.IP "\-f \fIfile\fP          "
Specifies a configuration file to be read. If the file is read
successfully, this automatically implies \-D.
.IP "\-i \fIprefix\fP        "
Import the given prefix, using the default mounting options (rw).
.IP "\-p                     "
Print the current state of imported and exported prefixes.
.IP "\-q                     "
Used only when invoking prefix as a daemon, causes standard status messages
not to be sent to the console. Error messages are still written there, however.
.IP "\-r \fIprefix\fP        "
Set the given prefix as a root prefix.
.IP "\-x \fIdirectory\fP [\fIprefix\fP]"
Export the given directory under the given prefix, or as itself if no
prefix given. If \fIdirectory\fP doesn't appear in
.CW /etc/exports ,
a warning message is printed to the console.
'\"
'\"		INSTALLATION
'\"
.xH Installation
.LP
The installation of ``prefix'' is relatively simple, since it is a
more-or-less self-contained system. The program operates in either
daemon mode or as a user's agent when conversing with the local
daemon, so it should probably be installed in /etc with a link to it
from a user-accessible directory. 
.ul
It should not be setuid to root.
As it must be started at boot time, when it will be executed by root,
there is no need to make it setuid.
.LP
Examine the makefile in the prefix directory to set the installation
point and the directory in which the symbolic link is to be placed, as
well as to set the names of certain system files, if you've moved
them. You should then be able, as root, to type ``make install''.
.LP
Once you've installed the daemon, you must arrange for it to start at
boot time and be given the proper set of initial prefixes, both imported
and exported. The best way to tell it this information is via a
(short) configuration file. I use
.CW /etc/prefix.conf .
The configuration file consists of a series of command lines, with
optional comments interspersed. There are five commands understood:
import, export, root, quiet and debug.
.IP "import \fIprefix\fP [\fImount options\fP]
Declares \fIprefix\fP to be an imported prefix. When it is mounted,
the optional \fImount options\fP are used. These options are exactly
as used in /etc/fstab and default to
.CW "rw"
if you don't give any.
.IP "export \fIdirectory\fP [\fIprefix\fP]
Declares \fIdirectory\fP to be exported under \fIprefix\fP, if given,
or as itself if no prefix is specified.
.IP "root \fIprefix\fP [\fImount options\fP]
Declares \fIprefix\fP to be a root prefix. Any prefix mounted under it
will be mounted with the given \fImount options\fP, as described for the
.CW import
command.
.IP "quiet            "
Normally, prefix will write to /dev/console when it is broadcasting
for the server of a prefix, as well as what server/directory pair it is
actually mounting and any problems with the mounting it encounters. If the
.CW quiet
command is given, the notices about the broadcasting for and the
server of a prefix will not be printed. Error messages will still be
written to the console, however.
.IP "debug             "
Turns on debugging for the daemon, including extensive heap checking
and very verbose progress messages that were needed during development
because the thing very frequently caused the machine to hang (thank
you ``disk wait,'' may you rot in Hell). Not very useful...
.LP
As an example, here is the configuration file I've got on my workstation:
.DS
.ta \w'export    'u +\w'/old_staff    'u
#
# Prefix configuration file for promethium
#
#
# Standard prefix roots
#
root	/n	rw,intr
root	/rn	rw,intr
#
# Standard imports
#
import	/old_staff	rw,intr
#
# Machine-specific imports/roots
#
import	/usr/src	rw,intr
#
# Standard exports
#
export	/	/rn/pm
export	/n/pm
export	/n/pm	/n/promethium
.DE
.LP
The mount options are optional, even though I've got them for all
imported prefixes. Note also that the same disk partition (/n/pm) is
exported under two names. This is perfectly acceptable.
.LP
One other question arises in as far as in which /etc/rc.foo file the
daemon should be started. I start it in /etc/rc itself, with the command:
.DS
if [ -f /etc/prefix -a -f /etc/prefix.conf ]; then
    (echo starting prefix daemon)				>/dev/console
    /etc/prefix /etc/prefix.conf 
fi
.DE
This goes immediately before the mounting of the remaining local (4.2
or 4.3-type) partitions from /etc/fstab because I've a partition that
wants mounting on
.CW /n/pm .
Because of the way the daemon works, and the fact that /n is a (root)
prefix, the daemon must be started before the local partition is
mounted, or I wouldn't be able to access the local partition (dire
things would happen, likely). It is permissible to explicitly mount a local or
remote filesystem immediately under a root prefix, as the prefix is
only sought should you attempt to look anything up in it (mounting
a filesystem doesn't qualify), nor will the kernel try and look
anything up there so long as the filesystem remains mounted.
.LP
An alternative, albeit a strange one, to the configuration file is to
specify all the prefixes using command line options.
'\"
'\"		INTERNALS
'\"
.xH Internals
.LP
This section is intended to give you a working understanding of how
the prefix daemon does its job, so you'll have some idea of what's
going on should the daemon have a bug in it (gasp! not \fImy\fP software!).
.LP
The main aspect of the daemon to know is that it appears to the kernel
as just another NFS server. The kernel doesn't care that its address
is on the local machine, as long as the daemon obeys the protocol.
Prefix sets up one mount point (with itself as the server) for each
imported and root prefix, with one other mount point at
.CW /.prefix
whose purpose I'll explain in a bit. These mount points are made
\fIsoft\fP with timeout and retransmission parameters tailored to the
operation of the daemon. They are soft so operations can just timeout
should the daemon die (remember, this is fairly young software).
.LP
As a directory, each mount point should be considered (and is actually
mounted) \fIread-only\fP \*- you cannot create symbolic links,
subdirectories or files in one.  The only NFS operations supported are
the fetching of attributes, the lookup of a file, the fetching of
filesystem attributes and the reading of directory entries (always
just . and .., except for a root prefix, from which a client will also
obtain the names of previously encountered subprefixes).
.LP
For all of these mount points, save that for /.prefix, only the lookup
of a file will cause the daemon to do anything special \*- fetching
the attributes of an unmounted prefix will return the attributes of
the underlying directory; reading it as a directory will only return
precalculated information, potentially making someone erroneously
think the prefix is mounted and empty, or that the daemon has died.
The reason prefix waits for a lookup is to prevent the search for, and
mounting of, a prefix until it is absolutely necessary \*- you
wouldn't want to have the prefix mounted if the user just did an ls of
its containing directory, would you? So it is when you lookup
something in the prefix that the fun begins.
'\"
'\"			MOUNTING
'\"
.xH 2 Mounting \*- The Fun Begins
.LP
For a root prefix, the fun is short-lived: all the daemon does is
create a new prefix for its own use and return a handle and the root's
attributes (though with a different file number, so the kernel won't
get confused).
.LP
For an imported prefix, however, the lookup of a file is a momentous
occasion. The importance, however, is lost on the daemon, as it will
always return the same answer: no matter what is being sought, prefix
will always say the target is a symbolic link to something in
the /.prefix directory. This subterfuge is necessary to cause the
kernel to unlock the data structure by which it found the prefix
daemon, allowing the daemon to unmount itself and mount the real filesystem
instead.  This bait and switch is performed when the kernel, using the
contents of the symbolic link the daemon returned, talks to the daemon
again, asking it to look something up in its /.prefix directory.
.LP
The format of the link contents returned by the previous lookup
operation is simply
.DS
/.prefix/\fIprefix\fP/\fIcomponent\fP
.DE
where \fIprefix\fP is the address of the daemon's internal prefix
descriptor (in ascii) and \fIcomponent\fP is the component the kernel
was attempting to find within the prefix. The daemon delays its search
for the server of the prefix until the kernel actually attempts to
read the symbolic link whose handle it returned, at which point it
assumes the kernel must be serious in its desire for the prefix, so it
broadcasts to the network to find the server of the prefix, recording
the results until the kernel acts on the link contents.
.LP
When the daemon gets this second lookup request, it can be fairly
certain that the prefix's mount point is free and attempts to unmount
itself from it. Should this fail, the lookup request will return a
general system error to the kernel, which the kernel will ignore until
it's been told enough times (this is why the broadcast for the
prefix's server is done when the link is read, not here, since the
link read will never fail). Should the daemon not be able to
disentangle itself from the prefix, the user will get back a message
about a remote system error from the kernel. This rarely happens,
however, so you needn't worry too much about it.
.LP
Once the daemon has unmounted itself, it goes through the process of
mounting the remote system in its place. Again, if this fails, the
user will see a message about a remote system error and the daemon
will re-mount itself on the prefix so the user can try again once
s/he's (or you've) straightened things out.
.LP
If the remote mount succeeds, the kernel is told that the component it
is seeking (which will be the ascii representation of the prefix's
address) is also a symbolic link, but this one's back to the prefix
itself. The kernel performs the same lookup it did before, since the
daemon tacked the component the kernel was seeking onto the end of the link
contents it returned, but this time the request goes to the real
server of the filesystem.
.LP
A mounted prefix is displayed in /etc/mtab in a slightly different
format than usual. Rather than giving the server as
``\fIserver\fP:\fIdirectory\fP'', I have chosen to display it as
``\fIserver\fP[\fIdirectory\fP]'' in an attempt to distinguish between
permanent systems people are used to and the transient systems that
are prefixes.
'\"
'\"			UNMOUNTING
'\"
.xH 2 Unmounting
.LP
Once the prefix is mounted on a remote filesystem, the question arises
of when to unmount it. The goal of reducing a machine's vulnerability
to server crashes would certainly not be met if all prefixes ever
referenced by a machine were to remain mounted, nor would
administrators be able to switch the serving of a prefix to another
machine very easily. To avoid this, prefix will attempt to unmount a
mounted prefix every ten minutes or so. If the attempt is successful,
the entry is removed from /etc/mtab and the daemon once again takes
over the mount point.
.LP
This solution isn't ideal of course, as one would much prefer to
unmount a prefix only ten minutes after it was last accessed, for
instance, rather than whenever it's possible at a ten minute interval.
Even if you've got a program referencing a prefix every five seconds,
it could still be unmounted if the program is merely stat'ing a file,
for example, rather than keeping a file open. To allow the daemon
access to the last-access time of a prefix would either require
special kernel support (stat'ing the root of the filesystem doesn't
help, since the access time of the directory isn't modified when a
directory search is performed; only when the directory is actually
read), or it would force the daemon to act as an intermediary between
the kernel and the remote server, an alternative I deemed too costly,
given the extra copies required on reads and writes (for a write, the
data would flow from the client to the kernel to the daemon to the
kernel to the remote system, causing an extra two copies, usually of
8K each or more).
'\"
'\"			MISCELLANEOUS
'\"
.xH 2 Miscellaneous
.LP
The daemon actually runs as two processes, one of which services the
various mount points and is generally in charge of everything. The
other process performs all the mount and unmount system calls at the
behest of the first process (actually, the one with the higher PID is
the one that's in charge, but who's counting?).
.LP
The daemon needs to be in two pieces so the kernel can check out
prefixes during mounting or unmounting, especially for the children of
a root prefix (in order to mount the remote system on a subprefix, the
kernel must request a file handle from the root prefix. Since the
daemon can't run to field this request while it's in the kernel
performing the mount, having only a single process would cause instant,
unending deadlock).
'\"
'\"		QUIRKS
'\"
.xH Quirks
.LP
There are a few non-obvious pieces of behaviour exhibited by a system
running this daemon. One already mentioned is the tendency to generate
strange error messages should a prefix be unmountable. As an example,
the command
.DS
cd /n/fishnet/biscuit
.DE
will yield the following two enlightening messages:
.DS
NFS readlink failed for server prefix: RPC: Remote system error
/n/fishnet/biscuit: Unknown error
.DE
.LP
Another quirk is exhibited by doing and ``ls -l'' of anything under an
as-yet unmounted prefix. While the prefix will indeed be mounted by
this operation, the nature of ls -l will cause output like this
.DS L
lrwxrwxrwx  1 root  23 Dec 31  1969 /usr/src/public -> /.prefix/164480/public
.DE
to be displayed, to the edification of no one.
'\"
'\"		EXTENSIONS
'\"
.xH Extensions
.LP
This service is by no means complete. The understanding of the access
controls in /etc/exports for exported prefixes is an example of a
worthwhile extension, as would an additional call to force all active
prefixes to unmount a prefix, if possible, to allow an administrator
to switch the server of a prefix. Unfortunately, I have little time to
implement such extensions (nor do I need them at the moment).
Ideas, however, are always welcome, especially if accompanied by code.
.TC
