r"""Wrapper for whisper.h

Generated with:
ctypesgen whisper.h -l whisper -o whisper_cpp_wrapper.py

Do not modify this file.
"""

__docformat__ = "restructuredtext"

# Begin preamble for Python

import ctypes
import sys
from ctypes import *  # noqa: F401, F403

_int_types = (ctypes.c_int16, ctypes.c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have ctypes.c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (ctypes.c_int64,)
for t in _int_types:
    if ctypes.sizeof(t) == ctypes.sizeof(ctypes.c_size_t):
        c_ptrdiff_t = t
del t
del _int_types



class UserString:
    def __init__(self, seq):
        if isinstance(seq, bytes):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq).encode()

    def __bytes__(self):
        return self.data

    def __str__(self):
        return self.data.decode()

    def __repr__(self):
        return repr(self.data)

    def __int__(self):
        return int(self.data.decode())

    def __long__(self):
        return int(self.data.decode())

    def __float__(self):
        return float(self.data.decode())

    def __complex__(self):
        return complex(self.data.decode())

    def __hash__(self):
        return hash(self.data)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        else:
            return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        else:
            return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        else:
            return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        else:
            return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        else:
            return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
        else:
            return self.data != string

    def __contains__(self, char):
        return char in self.data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        return self.__class__(self.data[index])

    def __getslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, bytes):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other).encode() + self.data)

    def __mul__(self, n):
        return self.__class__(self.data * n)

    __rmul__ = __mul__

    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self):
        return self.__class__(self.data.capitalize())

    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))

    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)

    def decode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())

    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)

    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))

    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)

    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)

    def isalpha(self):
        return self.data.isalpha()

    def isalnum(self):
        return self.data.isalnum()

    def isdecimal(self):
        return self.data.isdecimal()

    def isdigit(self):
        return self.data.isdigit()

    def islower(self):
        return self.data.islower()

    def isnumeric(self):
        return self.data.isnumeric()

    def isspace(self):
        return self.data.isspace()

    def istitle(self):
        return self.data.istitle()

    def isupper(self):
        return self.data.isupper()

    def join(self, seq):
        return self.data.join(seq)

    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))

    def lower(self):
        return self.__class__(self.data.lower())

    def lstrip(self, chars=None):
        return self.__class__(self.data.lstrip(chars))

    def partition(self, sep):
        return self.data.partition(sep)

    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))

    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)

    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)

    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))

    def rpartition(self, sep):
        return self.data.rpartition(sep)

    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))

    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)

    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)

    def splitlines(self, keepends=0):
        return self.data.splitlines(keepends)

    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)

    def strip(self, chars=None):
        return self.__class__(self.data.strip(chars))

    def swapcase(self):
        return self.__class__(self.data.swapcase())

    def title(self):
        return self.__class__(self.data.title())

    def translate(self, *args):
        return self.__class__(self.data.translate(*args))

    def upper(self):
        return self.__class__(self.data.upper())

    def zfill(self, width):
        return self.__class__(self.data.zfill(width))


class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""

    def __init__(self, string=""):
        self.data = string

    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")

    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + sub + self.data[index + 1 :]

    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + self.data[index + 1 :]

    def __setslice__(self, start, end, sub):
        start = max(start, 0)
        end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start] + sub.data + self.data[end:]
        elif isinstance(sub, bytes):
            self.data = self.data[:start] + sub + self.data[end:]
        else:
            self.data = self.data[:start] + str(sub).encode() + self.data[end:]

    def __delslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]

    def immutable(self):
        return UserString(self.data)

    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, bytes):
            self.data += other
        else:
            self.data += str(other).encode()
        return self

    def __imul__(self, n):
        self.data *= n
        return self


class String(MutableString, ctypes.Union):

    _fields_ = [("raw", ctypes.POINTER(ctypes.c_char)), ("data", ctypes.c_char_p)]

    def __init__(self, obj=b""):
        if isinstance(obj, (bytes, UserString)):
            self.data = bytes(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(ctypes.POINTER(ctypes.c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from bytes
        elif isinstance(obj, bytes):
            return cls(obj)

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj.encode())

        # Convert from c_char_p
        elif isinstance(obj, ctypes.c_char_p):
            return obj

        # Convert from POINTER(ctypes.c_char)
        elif isinstance(obj, ctypes.POINTER(ctypes.c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(ctypes.cast(obj, ctypes.POINTER(ctypes.c_char)))

        # Convert from ctypes.c_char array
        elif isinstance(obj, ctypes.c_char * len(obj)):
            return obj

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)

    from_param = classmethod(from_param)


def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)


# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to ctypes.c_void_p.
def UNCHECKED(type):
    if hasattr(type, "_type_") and isinstance(type._type_, str) and type._type_ != "P":
        return type
    else:
        return ctypes.c_void_p


# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self, func, restype, argtypes, errcheck):
        self.func = func
        self.func.restype = restype
        self.argtypes = argtypes
        if errcheck:
            self.func.errcheck = errcheck

    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func

    def __call__(self, *args):
        fixed_args = []
        i = 0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.func(*fixed_args + list(args[i:]))


def ord_if_char(value):
    """
    Simple helper used for casts to simple builtin types:  if the argument is a
    string type, it will be converted to it's ordinal value.

    This function will raise an exception if the argument is string with more
    than one characters.
    """
    return ord(value) if (isinstance(value, bytes) or isinstance(value, str)) else value

# End preamble

_libs = {}
_libdirs = []

# Begin loader

"""
Load libraries - appropriately for all our supported platforms
"""
# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import ctypes
import ctypes.util
import glob
import os.path
import platform
import re
import sys


def _environ_path(name):
    """Split an environment variable into a path-like list elements"""
    if name in os.environ:
        return os.environ[name].split(":")
    return []


class LibraryLoader:
    """
    A base class For loading of libraries ;-)
    Subclasses load libraries for specific platforms.
    """

    # library names formatted specifically for platforms
    name_formats = ["%s"]

    class Lookup:
        """Looking up calling conventions for a platform"""

        mode = ctypes.DEFAULT_MODE

        def __init__(self, path):
            super(LibraryLoader.Lookup, self).__init__()
            self.access = dict(cdecl=ctypes.CDLL(path, self.mode))

        def get(self, name, calling_convention="cdecl"):
            """Return the given name according to the selected calling convention"""
            if calling_convention not in self.access:
                raise LookupError(
                    "Unknown calling convention '{}' for function '{}'".format(
                        calling_convention, name
                    )
                )
            return getattr(self.access[calling_convention], name)

        def has(self, name, calling_convention="cdecl"):
            """Return True if this given calling convention finds the given 'name'"""
            if calling_convention not in self.access:
                return False
            return hasattr(self.access[calling_convention], name)

        def __getattr__(self, name):
            return getattr(self.access["cdecl"], name)

    def __init__(self):
        self.other_dirs = []

    def __call__(self, libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            # noinspection PyBroadException
            try:
                return self.Lookup(path)
            except Exception:  # pylint: disable=broad-except
                pass

        raise ImportError("Could not load %s." % libname)

    def getpaths(self, libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # search through a prioritized series of locations for the library

            # we first search any specific directories identified by user
            for dir_i in self.other_dirs:
                for fmt in self.name_formats:
                    # dir_i should be absolute already
                    yield os.path.join(dir_i, fmt % libname)

            # check if this code is even stored in a physical file
            try:
                this_file = __file__
            except NameError:
                this_file = None

            # then we search the directory where the generated python interface is stored
            if this_file is not None:
                for fmt in self.name_formats:
                    yield os.path.abspath(os.path.join(os.path.dirname(__file__), fmt % libname))

            # now, use the ctypes tools to try to find the library
            for fmt in self.name_formats:
                path = ctypes.util.find_library(fmt % libname)
                if path:
                    yield path

            # then we search all paths identified as platform-specific lib paths
            for path in self.getplatformpaths(libname):
                yield path

            # Finally, we'll try the users current working directory
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.curdir, fmt % libname))

    def getplatformpaths(self, _libname):  # pylint: disable=no-self-use
        """Return all the library paths available in this platform"""
        return []


# Darwin (Mac OS X)


class DarwinLibraryLoader(LibraryLoader):
    """Library loader for MacOS"""

    name_formats = [
        "lib%s.dylib",
        "lib%s.so",
        "lib%s.bundle",
        "%s.dylib",
        "%s.so",
        "%s.bundle",
        "%s",
    ]

    class Lookup(LibraryLoader.Lookup):
        """
        Looking up library files for this platform (Darwin aka MacOS)
        """

        # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
        # of the default RTLD_LOCAL.  Without this, you end up with
        # libraries not being loadable, resulting in "Symbol not found"
        # errors
        mode = ctypes.RTLD_GLOBAL

    def getplatformpaths(self, libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [fmt % libname for fmt in self.name_formats]

        for directory in self.getdirs(libname):
            for name in names:
                yield os.path.join(directory, name)

    @staticmethod
    def getdirs(libname):
        """Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        """

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [
                os.path.expanduser("~/lib"),
                "/usr/local/lib",
                "/usr/lib",
            ]

        dirs = []

        if "/" in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
            dirs.extend(_environ_path("LD_RUN_PATH"))

        if hasattr(sys, "frozen") and getattr(sys, "frozen") == "macosx_app":
            dirs.append(os.path.join(os.environ["RESOURCEPATH"], "..", "Frameworks"))

        dirs.extend(dyld_fallback_library_path)

        return dirs


# Posix


class PosixLibraryLoader(LibraryLoader):
    """Library loader for POSIX-like systems (including Linux)"""

    _ld_so_cache = None

    _include = re.compile(r"^\s*include\s+(?P<pattern>.*)")

    name_formats = ["lib%s.so", "%s.so", "%s"]

    class _Directories(dict):
        """Deal with directories"""

        def __init__(self):
            dict.__init__(self)
            self.order = 0

        def add(self, directory):
            """Add a directory to our current set of directories"""
            if len(directory) > 1:
                directory = directory.rstrip(os.path.sep)
            # only adds and updates order if exists and not already in set
            if not os.path.exists(directory):
                return
            order = self.setdefault(directory, self.order)
            if order == self.order:
                self.order += 1

        def extend(self, directories):
            """Add a list of directories to our set"""
            for a_dir in directories:
                self.add(a_dir)

        def ordered(self):
            """Sort the list of directories"""
            return (i[0] for i in sorted(self.items(), key=lambda d: d[1]))

    def _get_ld_so_conf_dirs(self, conf, dirs):
        """
        Recursive function to help parse all ld.so.conf files, including proper
        handling of the `include` directive.
        """

        try:
            with open(conf) as fileobj:
                for dirname in fileobj:
                    dirname = dirname.strip()
                    if not dirname:
                        continue

                    match = self._include.match(dirname)
                    if not match:
                        dirs.add(dirname)
                    else:
                        for dir2 in glob.glob(match.group("pattern")):
                            self._get_ld_so_conf_dirs(dir2, dirs)
        except IOError:
            pass

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = self._Directories()
        for name in (
            "LD_LIBRARY_PATH",
            "SHLIB_PATH",  # HP-UX
            "LIBPATH",  # OS/2, AIX
            "LIBRARY_PATH",  # BE/OS
        ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))

        self._get_ld_so_conf_dirs("/etc/ld.so.conf", directories)

        bitage = platform.architecture()[0]

        unix_lib_dirs_list = []
        if bitage.startswith("64"):
            # prefer 64 bit if that is our arch
            unix_lib_dirs_list += ["/lib64", "/usr/lib64"]

        # must include standard libs, since those paths are also used by 64 bit
        # installs
        unix_lib_dirs_list += ["/lib", "/usr/lib"]
        if sys.platform.startswith("linux"):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            if bitage.startswith("32"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/i386-linux-gnu", "/usr/lib/i386-linux-gnu"]
            elif bitage.startswith("64"):
                # Assume Intel/AMD x86 compatible
                unix_lib_dirs_list += [
                    "/lib/x86_64-linux-gnu",
                    "/usr/lib/x86_64-linux-gnu",
                ]
            else:
                # guess...
                unix_lib_dirs_list += glob.glob("/lib/*linux-gnu")
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r"lib(.*)\.s[ol]")
        # ext_re = re.compile(r"\.s[ol]$")
        for our_dir in directories.ordered():
            try:
                for path in glob.glob("%s/*.s[ol]*" % our_dir):
                    file = os.path.basename(path)

                    # Index by filename
                    cache_i = cache.setdefault(file, set())
                    cache_i.add(path)

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        cache_i = cache.setdefault(library, set())
                        cache_i.add(path)
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname, set())
        for i in result:
            # we iterate through all found paths for library, since we may have
            # actually found multiple architectures or other library types that
            # may not load
            yield i


# Windows


class WindowsLibraryLoader(LibraryLoader):
    """Library loader for Microsoft Windows"""

    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]

    class Lookup(LibraryLoader.Lookup):
        """Lookup class for Windows libraries..."""

        def __init__(self, path):
            super(WindowsLibraryLoader.Lookup, self).__init__(path)
            self.access["stdcall"] = ctypes.windll.LoadLibrary(path)


# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin": DarwinLibraryLoader,
    "cygwin": WindowsLibraryLoader,
    "win32": WindowsLibraryLoader,
    "msys": WindowsLibraryLoader,
}

load_library = loaderclass.get(sys.platform, PosixLibraryLoader)()


def add_library_search_dirs(other_dirs):
    """
    Add libraries to search paths.
    If library paths are relative, convert them to absolute with respect to this
    file's directory
    """
    for path in other_dirs:
        if not os.path.isabs(path):
            path = os.path.abspath(path)
        load_library.other_dirs.append(path)


del loaderclass

# End loader

add_library_search_dirs([])

# Begin libraries
_libs["whisper"] = load_library("whisper")

# 1 libraries
# End libraries

# No modules

__uint32_t = c_uint# /usr/include/x86_64-linux-gnu/bits/types.h: 42

uint32_t = __uint32_t# /usr/include/x86_64-linux-gnu/bits/stdint-uintn.h: 26

enum_ggml_log_level = c_int# /home/dnhkng/Documents/Projects/whisper.cpp/ggml.h: 461

ggml_log_callback = CFUNCTYPE(UNCHECKED(None), enum_ggml_log_level, String, POINTER(None))# /home/dnhkng/Documents/Projects/whisper.cpp/ggml.h: 1826

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 77
class struct_whisper_context(Structure):
    pass

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 78
class struct_whisper_state(Structure):
    pass

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 428
class struct_whisper_full_params(Structure):
    pass

whisper_pos = c_int32# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 81

whisper_token = c_int32# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 82

whisper_seq_id = c_int32# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 83

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 85
class struct_whisper_context_params(Structure):
    pass

struct_whisper_context_params.__slots__ = [
    'use_gpu',
]
struct_whisper_context_params._fields_ = [
    ('use_gpu', c_bool),
]

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 104
class struct_whisper_token_data(Structure):
    pass

struct_whisper_token_data.__slots__ = [
    'id',
    'tid',
    'p',
    'plog',
    'pt',
    'ptsum',
    't0',
    't1',
    'vlen',
]
struct_whisper_token_data._fields_ = [
    ('id', whisper_token),
    ('tid', whisper_token),
    ('p', c_float),
    ('plog', c_float),
    ('pt', c_float),
    ('ptsum', c_float),
    ('t0', c_int64),
    ('t1', c_int64),
    ('vlen', c_float),
]

whisper_token_data = struct_whisper_token_data# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 104

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 112
class struct_whisper_model_loader(Structure):
    pass

struct_whisper_model_loader.__slots__ = [
    'context',
    'read',
    'eof',
    'close',
]
struct_whisper_model_loader._fields_ = [
    ('context', POINTER(None)),
    ('read', CFUNCTYPE(UNCHECKED(c_size_t), POINTER(None), POINTER(None), c_size_t)),
    ('eof', CFUNCTYPE(UNCHECKED(c_bool), POINTER(None))),
    ('close', CFUNCTYPE(UNCHECKED(None), POINTER(None))),
]

whisper_model_loader = struct_whisper_model_loader# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 112

enum_whisper_gretype = c_int# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

WHISPER_GRETYPE_END = 0# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

WHISPER_GRETYPE_ALT = 1# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

WHISPER_GRETYPE_RULE_REF = 2# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

WHISPER_GRETYPE_CHAR = 3# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

WHISPER_GRETYPE_CHAR_NOT = 4# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

WHISPER_GRETYPE_CHAR_RNG_UPPER = 5# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

WHISPER_GRETYPE_CHAR_ALT = 6# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 115

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 143
class struct_whisper_grammar_element(Structure):
    pass

struct_whisper_grammar_element.__slots__ = [
    'type',
    'value',
]
struct_whisper_grammar_element._fields_ = [
    ('type', enum_whisper_gretype),
    ('value', uint32_t),
]

whisper_grammar_element = struct_whisper_grammar_element# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 143

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 148
if _libs["whisper"].has("whisper_init_from_file_with_params", "cdecl"):
    whisper_init_from_file_with_params = _libs["whisper"].get("whisper_init_from_file_with_params", "cdecl")
    whisper_init_from_file_with_params.argtypes = [String, struct_whisper_context_params]
    whisper_init_from_file_with_params.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 149
if _libs["whisper"].has("whisper_init_from_buffer_with_params", "cdecl"):
    whisper_init_from_buffer_with_params = _libs["whisper"].get("whisper_init_from_buffer_with_params", "cdecl")
    whisper_init_from_buffer_with_params.argtypes = [POINTER(None), c_size_t, struct_whisper_context_params]
    whisper_init_from_buffer_with_params.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 150
if _libs["whisper"].has("whisper_init_with_params", "cdecl"):
    whisper_init_with_params = _libs["whisper"].get("whisper_init_with_params", "cdecl")
    whisper_init_with_params.argtypes = [POINTER(struct_whisper_model_loader), struct_whisper_context_params]
    whisper_init_with_params.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 154
if _libs["whisper"].has("whisper_init_from_file_with_params_no_state", "cdecl"):
    whisper_init_from_file_with_params_no_state = _libs["whisper"].get("whisper_init_from_file_with_params_no_state", "cdecl")
    whisper_init_from_file_with_params_no_state.argtypes = [String, struct_whisper_context_params]
    whisper_init_from_file_with_params_no_state.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 155
if _libs["whisper"].has("whisper_init_from_buffer_with_params_no_state", "cdecl"):
    whisper_init_from_buffer_with_params_no_state = _libs["whisper"].get("whisper_init_from_buffer_with_params_no_state", "cdecl")
    whisper_init_from_buffer_with_params_no_state.argtypes = [POINTER(None), c_size_t, struct_whisper_context_params]
    whisper_init_from_buffer_with_params_no_state.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 156
if _libs["whisper"].has("whisper_init_with_params_no_state", "cdecl"):
    whisper_init_with_params_no_state = _libs["whisper"].get("whisper_init_with_params_no_state", "cdecl")
    whisper_init_with_params_no_state.argtypes = [POINTER(struct_whisper_model_loader), struct_whisper_context_params]
    whisper_init_with_params_no_state.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 158
if _libs["whisper"].has("whisper_init_from_file", "cdecl"):
    whisper_init_from_file = _libs["whisper"].get("whisper_init_from_file", "cdecl")
    whisper_init_from_file.argtypes = [String]
    whisper_init_from_file.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 162
if _libs["whisper"].has("whisper_init_from_buffer", "cdecl"):
    whisper_init_from_buffer = _libs["whisper"].get("whisper_init_from_buffer", "cdecl")
    whisper_init_from_buffer.argtypes = [POINTER(None), c_size_t]
    whisper_init_from_buffer.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 166
if _libs["whisper"].has("whisper_init", "cdecl"):
    whisper_init = _libs["whisper"].get("whisper_init", "cdecl")
    whisper_init.argtypes = [POINTER(struct_whisper_model_loader)]
    whisper_init.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 170
if _libs["whisper"].has("whisper_init_from_file_no_state", "cdecl"):
    whisper_init_from_file_no_state = _libs["whisper"].get("whisper_init_from_file_no_state", "cdecl")
    whisper_init_from_file_no_state.argtypes = [String]
    whisper_init_from_file_no_state.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 174
if _libs["whisper"].has("whisper_init_from_buffer_no_state", "cdecl"):
    whisper_init_from_buffer_no_state = _libs["whisper"].get("whisper_init_from_buffer_no_state", "cdecl")
    whisper_init_from_buffer_no_state.argtypes = [POINTER(None), c_size_t]
    whisper_init_from_buffer_no_state.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 178
if _libs["whisper"].has("whisper_init_no_state", "cdecl"):
    whisper_init_no_state = _libs["whisper"].get("whisper_init_no_state", "cdecl")
    whisper_init_no_state.argtypes = [POINTER(struct_whisper_model_loader)]
    whisper_init_no_state.restype = POINTER(struct_whisper_context)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 183
if _libs["whisper"].has("whisper_init_state", "cdecl"):
    whisper_init_state = _libs["whisper"].get("whisper_init_state", "cdecl")
    whisper_init_state.argtypes = [POINTER(struct_whisper_context)]
    whisper_init_state.restype = POINTER(struct_whisper_state)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 196
if _libs["whisper"].has("whisper_ctx_init_openvino_encoder", "cdecl"):
    whisper_ctx_init_openvino_encoder = _libs["whisper"].get("whisper_ctx_init_openvino_encoder", "cdecl")
    whisper_ctx_init_openvino_encoder.argtypes = [POINTER(struct_whisper_context), String, String, String]
    whisper_ctx_init_openvino_encoder.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 203
if _libs["whisper"].has("whisper_free", "cdecl"):
    whisper_free = _libs["whisper"].get("whisper_free", "cdecl")
    whisper_free.argtypes = [POINTER(struct_whisper_context)]
    whisper_free.restype = None

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 204
if _libs["whisper"].has("whisper_free_state", "cdecl"):
    whisper_free_state = _libs["whisper"].get("whisper_free_state", "cdecl")
    whisper_free_state.argtypes = [POINTER(struct_whisper_state)]
    whisper_free_state.restype = None

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 205
if _libs["whisper"].has("whisper_free_params", "cdecl"):
    whisper_free_params = _libs["whisper"].get("whisper_free_params", "cdecl")
    whisper_free_params.argtypes = [POINTER(struct_whisper_full_params)]
    whisper_free_params.restype = None

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 206
if _libs["whisper"].has("whisper_free_context_params", "cdecl"):
    whisper_free_context_params = _libs["whisper"].get("whisper_free_context_params", "cdecl")
    whisper_free_context_params.argtypes = [POINTER(struct_whisper_context_params)]
    whisper_free_context_params.restype = None

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 211
if _libs["whisper"].has("whisper_pcm_to_mel", "cdecl"):
    whisper_pcm_to_mel = _libs["whisper"].get("whisper_pcm_to_mel", "cdecl")
    whisper_pcm_to_mel.argtypes = [POINTER(struct_whisper_context), POINTER(c_float), c_int, c_int]
    whisper_pcm_to_mel.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 217
if _libs["whisper"].has("whisper_pcm_to_mel_with_state", "cdecl"):
    whisper_pcm_to_mel_with_state = _libs["whisper"].get("whisper_pcm_to_mel_with_state", "cdecl")
    whisper_pcm_to_mel_with_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), POINTER(c_float), c_int, c_int]
    whisper_pcm_to_mel_with_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 227
if _libs["whisper"].has("whisper_pcm_to_mel_phase_vocoder", "cdecl"):
    whisper_pcm_to_mel_phase_vocoder = _libs["whisper"].get("whisper_pcm_to_mel_phase_vocoder", "cdecl")
    whisper_pcm_to_mel_phase_vocoder.argtypes = [POINTER(struct_whisper_context), POINTER(c_float), c_int, c_int]
    whisper_pcm_to_mel_phase_vocoder.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 233
if _libs["whisper"].has("whisper_pcm_to_mel_phase_vocoder_with_state", "cdecl"):
    whisper_pcm_to_mel_phase_vocoder_with_state = _libs["whisper"].get("whisper_pcm_to_mel_phase_vocoder_with_state", "cdecl")
    whisper_pcm_to_mel_phase_vocoder_with_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), POINTER(c_float), c_int, c_int]
    whisper_pcm_to_mel_phase_vocoder_with_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 244
if _libs["whisper"].has("whisper_set_mel", "cdecl"):
    whisper_set_mel = _libs["whisper"].get("whisper_set_mel", "cdecl")
    whisper_set_mel.argtypes = [POINTER(struct_whisper_context), POINTER(c_float), c_int, c_int]
    whisper_set_mel.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 250
if _libs["whisper"].has("whisper_set_mel_with_state", "cdecl"):
    whisper_set_mel_with_state = _libs["whisper"].get("whisper_set_mel_with_state", "cdecl")
    whisper_set_mel_with_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), POINTER(c_float), c_int, c_int]
    whisper_set_mel_with_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 261
if _libs["whisper"].has("whisper_encode", "cdecl"):
    whisper_encode = _libs["whisper"].get("whisper_encode", "cdecl")
    whisper_encode.argtypes = [POINTER(struct_whisper_context), c_int, c_int]
    whisper_encode.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 266
if _libs["whisper"].has("whisper_encode_with_state", "cdecl"):
    whisper_encode_with_state = _libs["whisper"].get("whisper_encode_with_state", "cdecl")
    whisper_encode_with_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), c_int, c_int]
    whisper_encode_with_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 278
if _libs["whisper"].has("whisper_decode", "cdecl"):
    whisper_decode = _libs["whisper"].get("whisper_decode", "cdecl")
    whisper_decode.argtypes = [POINTER(struct_whisper_context), POINTER(whisper_token), c_int, c_int, c_int]
    whisper_decode.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 285
if _libs["whisper"].has("whisper_decode_with_state", "cdecl"):
    whisper_decode_with_state = _libs["whisper"].get("whisper_decode_with_state", "cdecl")
    whisper_decode_with_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), POINTER(whisper_token), c_int, c_int, c_int]
    whisper_decode_with_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 298
if _libs["whisper"].has("whisper_tokenize", "cdecl"):
    whisper_tokenize = _libs["whisper"].get("whisper_tokenize", "cdecl")
    whisper_tokenize.argtypes = [POINTER(struct_whisper_context), String, POINTER(whisper_token), c_int]
    whisper_tokenize.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 305
if _libs["whisper"].has("whisper_lang_max_id", "cdecl"):
    whisper_lang_max_id = _libs["whisper"].get("whisper_lang_max_id", "cdecl")
    whisper_lang_max_id.argtypes = []
    whisper_lang_max_id.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 311
if _libs["whisper"].has("whisper_lang_id", "cdecl"):
    whisper_lang_id = _libs["whisper"].get("whisper_lang_id", "cdecl")
    whisper_lang_id.argtypes = [String]
    whisper_lang_id.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 314
if _libs["whisper"].has("whisper_lang_str", "cdecl"):
    whisper_lang_str = _libs["whisper"].get("whisper_lang_str", "cdecl")
    whisper_lang_str.argtypes = [c_int]
    whisper_lang_str.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 322
if _libs["whisper"].has("whisper_lang_auto_detect", "cdecl"):
    whisper_lang_auto_detect = _libs["whisper"].get("whisper_lang_auto_detect", "cdecl")
    whisper_lang_auto_detect.argtypes = [POINTER(struct_whisper_context), c_int, c_int, POINTER(c_float)]
    whisper_lang_auto_detect.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 328
if _libs["whisper"].has("whisper_lang_auto_detect_with_state", "cdecl"):
    whisper_lang_auto_detect_with_state = _libs["whisper"].get("whisper_lang_auto_detect_with_state", "cdecl")
    whisper_lang_auto_detect_with_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), c_int, c_int, POINTER(c_float)]
    whisper_lang_auto_detect_with_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 335
if _libs["whisper"].has("whisper_n_len", "cdecl"):
    whisper_n_len = _libs["whisper"].get("whisper_n_len", "cdecl")
    whisper_n_len.argtypes = [POINTER(struct_whisper_context)]
    whisper_n_len.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 336
if _libs["whisper"].has("whisper_n_len_from_state", "cdecl"):
    whisper_n_len_from_state = _libs["whisper"].get("whisper_n_len_from_state", "cdecl")
    whisper_n_len_from_state.argtypes = [POINTER(struct_whisper_state)]
    whisper_n_len_from_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 337
if _libs["whisper"].has("whisper_n_vocab", "cdecl"):
    whisper_n_vocab = _libs["whisper"].get("whisper_n_vocab", "cdecl")
    whisper_n_vocab.argtypes = [POINTER(struct_whisper_context)]
    whisper_n_vocab.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 338
if _libs["whisper"].has("whisper_n_text_ctx", "cdecl"):
    whisper_n_text_ctx = _libs["whisper"].get("whisper_n_text_ctx", "cdecl")
    whisper_n_text_ctx.argtypes = [POINTER(struct_whisper_context)]
    whisper_n_text_ctx.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 339
if _libs["whisper"].has("whisper_n_audio_ctx", "cdecl"):
    whisper_n_audio_ctx = _libs["whisper"].get("whisper_n_audio_ctx", "cdecl")
    whisper_n_audio_ctx.argtypes = [POINTER(struct_whisper_context)]
    whisper_n_audio_ctx.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 340
if _libs["whisper"].has("whisper_is_multilingual", "cdecl"):
    whisper_is_multilingual = _libs["whisper"].get("whisper_is_multilingual", "cdecl")
    whisper_is_multilingual.argtypes = [POINTER(struct_whisper_context)]
    whisper_is_multilingual.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 342
if _libs["whisper"].has("whisper_model_n_vocab", "cdecl"):
    whisper_model_n_vocab = _libs["whisper"].get("whisper_model_n_vocab", "cdecl")
    whisper_model_n_vocab.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_vocab.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 343
if _libs["whisper"].has("whisper_model_n_audio_ctx", "cdecl"):
    whisper_model_n_audio_ctx = _libs["whisper"].get("whisper_model_n_audio_ctx", "cdecl")
    whisper_model_n_audio_ctx.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_audio_ctx.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 344
if _libs["whisper"].has("whisper_model_n_audio_state", "cdecl"):
    whisper_model_n_audio_state = _libs["whisper"].get("whisper_model_n_audio_state", "cdecl")
    whisper_model_n_audio_state.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_audio_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 345
if _libs["whisper"].has("whisper_model_n_audio_head", "cdecl"):
    whisper_model_n_audio_head = _libs["whisper"].get("whisper_model_n_audio_head", "cdecl")
    whisper_model_n_audio_head.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_audio_head.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 346
if _libs["whisper"].has("whisper_model_n_audio_layer", "cdecl"):
    whisper_model_n_audio_layer = _libs["whisper"].get("whisper_model_n_audio_layer", "cdecl")
    whisper_model_n_audio_layer.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_audio_layer.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 347
if _libs["whisper"].has("whisper_model_n_text_ctx", "cdecl"):
    whisper_model_n_text_ctx = _libs["whisper"].get("whisper_model_n_text_ctx", "cdecl")
    whisper_model_n_text_ctx.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_text_ctx.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 348
if _libs["whisper"].has("whisper_model_n_text_state", "cdecl"):
    whisper_model_n_text_state = _libs["whisper"].get("whisper_model_n_text_state", "cdecl")
    whisper_model_n_text_state.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_text_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 349
if _libs["whisper"].has("whisper_model_n_text_head", "cdecl"):
    whisper_model_n_text_head = _libs["whisper"].get("whisper_model_n_text_head", "cdecl")
    whisper_model_n_text_head.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_text_head.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 350
if _libs["whisper"].has("whisper_model_n_text_layer", "cdecl"):
    whisper_model_n_text_layer = _libs["whisper"].get("whisper_model_n_text_layer", "cdecl")
    whisper_model_n_text_layer.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_text_layer.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 351
if _libs["whisper"].has("whisper_model_n_mels", "cdecl"):
    whisper_model_n_mels = _libs["whisper"].get("whisper_model_n_mels", "cdecl")
    whisper_model_n_mels.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_n_mels.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 352
if _libs["whisper"].has("whisper_model_ftype", "cdecl"):
    whisper_model_ftype = _libs["whisper"].get("whisper_model_ftype", "cdecl")
    whisper_model_ftype.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_ftype.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 353
if _libs["whisper"].has("whisper_model_type", "cdecl"):
    whisper_model_type = _libs["whisper"].get("whisper_model_type", "cdecl")
    whisper_model_type.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_type.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 359
if _libs["whisper"].has("whisper_get_logits", "cdecl"):
    whisper_get_logits = _libs["whisper"].get("whisper_get_logits", "cdecl")
    whisper_get_logits.argtypes = [POINTER(struct_whisper_context)]
    whisper_get_logits.restype = POINTER(c_float)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 360
if _libs["whisper"].has("whisper_get_logits_from_state", "cdecl"):
    whisper_get_logits_from_state = _libs["whisper"].get("whisper_get_logits_from_state", "cdecl")
    whisper_get_logits_from_state.argtypes = [POINTER(struct_whisper_state)]
    whisper_get_logits_from_state.restype = POINTER(c_float)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 363
if _libs["whisper"].has("whisper_token_to_str", "cdecl"):
    whisper_token_to_str = _libs["whisper"].get("whisper_token_to_str", "cdecl")
    whisper_token_to_str.argtypes = [POINTER(struct_whisper_context), whisper_token]
    whisper_token_to_str.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 364
if _libs["whisper"].has("whisper_model_type_readable", "cdecl"):
    whisper_model_type_readable = _libs["whisper"].get("whisper_model_type_readable", "cdecl")
    whisper_model_type_readable.argtypes = [POINTER(struct_whisper_context)]
    whisper_model_type_readable.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 368
if _libs["whisper"].has("whisper_token_eot", "cdecl"):
    whisper_token_eot = _libs["whisper"].get("whisper_token_eot", "cdecl")
    whisper_token_eot.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_eot.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 369
if _libs["whisper"].has("whisper_token_sot", "cdecl"):
    whisper_token_sot = _libs["whisper"].get("whisper_token_sot", "cdecl")
    whisper_token_sot.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_sot.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 370
if _libs["whisper"].has("whisper_token_solm", "cdecl"):
    whisper_token_solm = _libs["whisper"].get("whisper_token_solm", "cdecl")
    whisper_token_solm.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_solm.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 371
if _libs["whisper"].has("whisper_token_prev", "cdecl"):
    whisper_token_prev = _libs["whisper"].get("whisper_token_prev", "cdecl")
    whisper_token_prev.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_prev.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 372
if _libs["whisper"].has("whisper_token_nosp", "cdecl"):
    whisper_token_nosp = _libs["whisper"].get("whisper_token_nosp", "cdecl")
    whisper_token_nosp.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_nosp.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 373
if _libs["whisper"].has("whisper_token_not", "cdecl"):
    whisper_token_not = _libs["whisper"].get("whisper_token_not", "cdecl")
    whisper_token_not.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_not.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 374
if _libs["whisper"].has("whisper_token_beg", "cdecl"):
    whisper_token_beg = _libs["whisper"].get("whisper_token_beg", "cdecl")
    whisper_token_beg.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_beg.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 375
if _libs["whisper"].has("whisper_token_lang", "cdecl"):
    whisper_token_lang = _libs["whisper"].get("whisper_token_lang", "cdecl")
    whisper_token_lang.argtypes = [POINTER(struct_whisper_context), c_int]
    whisper_token_lang.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 378
if _libs["whisper"].has("whisper_token_translate", "cdecl"):
    whisper_token_translate = _libs["whisper"].get("whisper_token_translate", "cdecl")
    whisper_token_translate.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_translate.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 379
if _libs["whisper"].has("whisper_token_transcribe", "cdecl"):
    whisper_token_transcribe = _libs["whisper"].get("whisper_token_transcribe", "cdecl")
    whisper_token_transcribe.argtypes = [POINTER(struct_whisper_context)]
    whisper_token_transcribe.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 382
if _libs["whisper"].has("whisper_print_timings", "cdecl"):
    whisper_print_timings = _libs["whisper"].get("whisper_print_timings", "cdecl")
    whisper_print_timings.argtypes = [POINTER(struct_whisper_context)]
    whisper_print_timings.restype = None

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 383
if _libs["whisper"].has("whisper_reset_timings", "cdecl"):
    whisper_reset_timings = _libs["whisper"].get("whisper_reset_timings", "cdecl")
    whisper_reset_timings.argtypes = [POINTER(struct_whisper_context)]
    whisper_reset_timings.restype = None

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 386
if _libs["whisper"].has("whisper_print_system_info", "cdecl"):
    whisper_print_system_info = _libs["whisper"].get("whisper_print_system_info", "cdecl")
    whisper_print_system_info.argtypes = []
    whisper_print_system_info.restype = c_char_p

enum_whisper_sampling_strategy = c_int# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 391

WHISPER_SAMPLING_GREEDY = 0# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 391

WHISPER_SAMPLING_BEAM_SEARCH = (WHISPER_SAMPLING_GREEDY + 1)# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 391

whisper_new_segment_callback = CFUNCTYPE(UNCHECKED(None), POINTER(struct_whisper_context), POINTER(struct_whisper_state), c_int, POINTER(None))# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 399

whisper_progress_callback = CFUNCTYPE(UNCHECKED(None), POINTER(struct_whisper_context), POINTER(struct_whisper_state), c_int, POINTER(None))# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 402

whisper_encoder_begin_callback = CFUNCTYPE(UNCHECKED(c_bool), POINTER(struct_whisper_context), POINTER(struct_whisper_state), POINTER(None))# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 407

whisper_abort_callback = CFUNCTYPE(UNCHECKED(c_bool), POINTER(None))# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 412

whisper_logits_filter_callback = CFUNCTYPE(UNCHECKED(None), POINTER(struct_whisper_context), POINTER(struct_whisper_state), POINTER(whisper_token_data), c_int, POINTER(c_float), POINTER(None))# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 417

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 487
class struct_anon_8(Structure):
    pass

struct_anon_8.__slots__ = [
    'best_of',
]
struct_anon_8._fields_ = [
    ('best_of', c_int),
]

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 491
class struct_anon_9(Structure):
    pass

struct_anon_9.__slots__ = [
    'beam_size',
    'patience',
]
struct_anon_9._fields_ = [
    ('beam_size', c_int),
    ('patience', c_float),
]

struct_whisper_full_params.__slots__ = [
    'strategy',
    'n_threads',
    'n_max_text_ctx',
    'offset_ms',
    'duration_ms',
    'translate',
    'no_context',
    'no_timestamps',
    'single_segment',
    'print_special',
    'print_progress',
    'print_realtime',
    'print_timestamps',
    'token_timestamps',
    'thold_pt',
    'thold_ptsum',
    'max_len',
    'split_on_word',
    'max_tokens',
    'speed_up',
    'debug_mode',
    'audio_ctx',
    'tdrz_enable',
    'initial_prompt',
    'prompt_tokens',
    'prompt_n_tokens',
    'language',
    'detect_language',
    'suppress_blank',
    'suppress_non_speech_tokens',
    'temperature',
    'max_initial_ts',
    'length_penalty',
    'temperature_inc',
    'entropy_thold',
    'logprob_thold',
    'no_speech_thold',
    'greedy',
    'beam_search',
    'new_segment_callback',
    'new_segment_callback_user_data',
    'progress_callback',
    'progress_callback_user_data',
    'encoder_begin_callback',
    'encoder_begin_callback_user_data',
    'abort_callback',
    'abort_callback_user_data',
    'logits_filter_callback',
    'logits_filter_callback_user_data',
    'grammar_rules',
    'n_grammar_rules',
    'i_start_rule',
    'grammar_penalty',
]
struct_whisper_full_params._fields_ = [
    ('strategy', enum_whisper_sampling_strategy),
    ('n_threads', c_int),
    ('n_max_text_ctx', c_int),
    ('offset_ms', c_int),
    ('duration_ms', c_int),
    ('translate', c_bool),
    ('no_context', c_bool),
    ('no_timestamps', c_bool),
    ('single_segment', c_bool),
    ('print_special', c_bool),
    ('print_progress', c_bool),
    ('print_realtime', c_bool),
    ('print_timestamps', c_bool),
    ('token_timestamps', c_bool),
    ('thold_pt', c_float),
    ('thold_ptsum', c_float),
    ('max_len', c_int),
    ('split_on_word', c_bool),
    ('max_tokens', c_int),
    ('speed_up', c_bool),
    ('debug_mode', c_bool),
    ('audio_ctx', c_int),
    ('tdrz_enable', c_bool),
    ('initial_prompt', String),
    ('prompt_tokens', POINTER(whisper_token)),
    ('prompt_n_tokens', c_int),
    ('language', String),
    ('detect_language', c_bool),
    ('suppress_blank', c_bool),
    ('suppress_non_speech_tokens', c_bool),
    ('temperature', c_float),
    ('max_initial_ts', c_float),
    ('length_penalty', c_float),
    ('temperature_inc', c_float),
    ('entropy_thold', c_float),
    ('logprob_thold', c_float),
    ('no_speech_thold', c_float),
    ('greedy', struct_anon_8),
    ('beam_search', struct_anon_9),
    ('new_segment_callback', whisper_new_segment_callback),
    ('new_segment_callback_user_data', POINTER(None)),
    ('progress_callback', whisper_progress_callback),
    ('progress_callback_user_data', POINTER(None)),
    ('encoder_begin_callback', whisper_encoder_begin_callback),
    ('encoder_begin_callback_user_data', POINTER(None)),
    ('abort_callback', whisper_abort_callback),
    ('abort_callback_user_data', POINTER(None)),
    ('logits_filter_callback', whisper_logits_filter_callback),
    ('logits_filter_callback_user_data', POINTER(None)),
    ('grammar_rules', POINTER(POINTER(whisper_grammar_element))),
    ('n_grammar_rules', c_size_t),
    ('i_start_rule', c_size_t),
    ('grammar_penalty', c_float),
]

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 524
if _libs["whisper"].has("whisper_context_default_params_by_ref", "cdecl"):
    whisper_context_default_params_by_ref = _libs["whisper"].get("whisper_context_default_params_by_ref", "cdecl")
    whisper_context_default_params_by_ref.argtypes = []
    whisper_context_default_params_by_ref.restype = POINTER(struct_whisper_context_params)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 525
if _libs["whisper"].has("whisper_context_default_params", "cdecl"):
    whisper_context_default_params = _libs["whisper"].get("whisper_context_default_params", "cdecl")
    whisper_context_default_params.argtypes = []
    whisper_context_default_params.restype = struct_whisper_context_params

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 526
if _libs["whisper"].has("whisper_full_default_params_by_ref", "cdecl"):
    whisper_full_default_params_by_ref = _libs["whisper"].get("whisper_full_default_params_by_ref", "cdecl")
    whisper_full_default_params_by_ref.argtypes = [enum_whisper_sampling_strategy]
    whisper_full_default_params_by_ref.restype = POINTER(struct_whisper_full_params)

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 527
if _libs["whisper"].has("whisper_full_default_params", "cdecl"):
    whisper_full_default_params = _libs["whisper"].get("whisper_full_default_params", "cdecl")
    whisper_full_default_params.argtypes = [enum_whisper_sampling_strategy]
    whisper_full_default_params.restype = struct_whisper_full_params

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 532
if _libs["whisper"].has("whisper_full", "cdecl"):
    whisper_full = _libs["whisper"].get("whisper_full", "cdecl")
    whisper_full.argtypes = [POINTER(struct_whisper_context), struct_whisper_full_params, POINTER(c_float), c_int]
    whisper_full.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 538
if _libs["whisper"].has("whisper_full_with_state", "cdecl"):
    whisper_full_with_state = _libs["whisper"].get("whisper_full_with_state", "cdecl")
    whisper_full_with_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), struct_whisper_full_params, POINTER(c_float), c_int]
    whisper_full_with_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 550
if _libs["whisper"].has("whisper_full_parallel", "cdecl"):
    whisper_full_parallel = _libs["whisper"].get("whisper_full_parallel", "cdecl")
    whisper_full_parallel.argtypes = [POINTER(struct_whisper_context), struct_whisper_full_params, POINTER(c_float), c_int, c_int]
    whisper_full_parallel.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 559
if _libs["whisper"].has("whisper_full_n_segments", "cdecl"):
    whisper_full_n_segments = _libs["whisper"].get("whisper_full_n_segments", "cdecl")
    whisper_full_n_segments.argtypes = [POINTER(struct_whisper_context)]
    whisper_full_n_segments.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 560
if _libs["whisper"].has("whisper_full_n_segments_from_state", "cdecl"):
    whisper_full_n_segments_from_state = _libs["whisper"].get("whisper_full_n_segments_from_state", "cdecl")
    whisper_full_n_segments_from_state.argtypes = [POINTER(struct_whisper_state)]
    whisper_full_n_segments_from_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 563
if _libs["whisper"].has("whisper_full_lang_id", "cdecl"):
    whisper_full_lang_id = _libs["whisper"].get("whisper_full_lang_id", "cdecl")
    whisper_full_lang_id.argtypes = [POINTER(struct_whisper_context)]
    whisper_full_lang_id.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 566
if _libs["whisper"].has("whisper_full_lang_id_from_state", "cdecl"):
    whisper_full_lang_id_from_state = _libs["whisper"].get("whisper_full_lang_id_from_state", "cdecl")
    whisper_full_lang_id_from_state.argtypes = [POINTER(struct_whisper_state)]
    whisper_full_lang_id_from_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 569
if _libs["whisper"].has("whisper_full_get_segment_t0", "cdecl"):
    whisper_full_get_segment_t0 = _libs["whisper"].get("whisper_full_get_segment_t0", "cdecl")
    whisper_full_get_segment_t0.argtypes = [POINTER(struct_whisper_context), c_int]
    whisper_full_get_segment_t0.restype = c_int64

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 570
if _libs["whisper"].has("whisper_full_get_segment_t0_from_state", "cdecl"):
    whisper_full_get_segment_t0_from_state = _libs["whisper"].get("whisper_full_get_segment_t0_from_state", "cdecl")
    whisper_full_get_segment_t0_from_state.argtypes = [POINTER(struct_whisper_state), c_int]
    whisper_full_get_segment_t0_from_state.restype = c_int64

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 572
if _libs["whisper"].has("whisper_full_get_segment_t1", "cdecl"):
    whisper_full_get_segment_t1 = _libs["whisper"].get("whisper_full_get_segment_t1", "cdecl")
    whisper_full_get_segment_t1.argtypes = [POINTER(struct_whisper_context), c_int]
    whisper_full_get_segment_t1.restype = c_int64

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 573
if _libs["whisper"].has("whisper_full_get_segment_t1_from_state", "cdecl"):
    whisper_full_get_segment_t1_from_state = _libs["whisper"].get("whisper_full_get_segment_t1_from_state", "cdecl")
    whisper_full_get_segment_t1_from_state.argtypes = [POINTER(struct_whisper_state), c_int]
    whisper_full_get_segment_t1_from_state.restype = c_int64

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 576
if _libs["whisper"].has("whisper_full_get_segment_speaker_turn_next", "cdecl"):
    whisper_full_get_segment_speaker_turn_next = _libs["whisper"].get("whisper_full_get_segment_speaker_turn_next", "cdecl")
    whisper_full_get_segment_speaker_turn_next.argtypes = [POINTER(struct_whisper_context), c_int]
    whisper_full_get_segment_speaker_turn_next.restype = c_bool

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 577
if _libs["whisper"].has("whisper_full_get_segment_speaker_turn_next_from_state", "cdecl"):
    whisper_full_get_segment_speaker_turn_next_from_state = _libs["whisper"].get("whisper_full_get_segment_speaker_turn_next_from_state", "cdecl")
    whisper_full_get_segment_speaker_turn_next_from_state.argtypes = [POINTER(struct_whisper_state), c_int]
    whisper_full_get_segment_speaker_turn_next_from_state.restype = c_bool

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 580
if _libs["whisper"].has("whisper_full_get_segment_text", "cdecl"):
    whisper_full_get_segment_text = _libs["whisper"].get("whisper_full_get_segment_text", "cdecl")
    whisper_full_get_segment_text.argtypes = [POINTER(struct_whisper_context), c_int]
    whisper_full_get_segment_text.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 581
if _libs["whisper"].has("whisper_full_get_segment_text_from_state", "cdecl"):
    whisper_full_get_segment_text_from_state = _libs["whisper"].get("whisper_full_get_segment_text_from_state", "cdecl")
    whisper_full_get_segment_text_from_state.argtypes = [POINTER(struct_whisper_state), c_int]
    whisper_full_get_segment_text_from_state.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 584
if _libs["whisper"].has("whisper_full_n_tokens", "cdecl"):
    whisper_full_n_tokens = _libs["whisper"].get("whisper_full_n_tokens", "cdecl")
    whisper_full_n_tokens.argtypes = [POINTER(struct_whisper_context), c_int]
    whisper_full_n_tokens.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 585
if _libs["whisper"].has("whisper_full_n_tokens_from_state", "cdecl"):
    whisper_full_n_tokens_from_state = _libs["whisper"].get("whisper_full_n_tokens_from_state", "cdecl")
    whisper_full_n_tokens_from_state.argtypes = [POINTER(struct_whisper_state), c_int]
    whisper_full_n_tokens_from_state.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 588
if _libs["whisper"].has("whisper_full_get_token_text", "cdecl"):
    whisper_full_get_token_text = _libs["whisper"].get("whisper_full_get_token_text", "cdecl")
    whisper_full_get_token_text.argtypes = [POINTER(struct_whisper_context), c_int, c_int]
    whisper_full_get_token_text.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 589
if _libs["whisper"].has("whisper_full_get_token_text_from_state", "cdecl"):
    whisper_full_get_token_text_from_state = _libs["whisper"].get("whisper_full_get_token_text_from_state", "cdecl")
    whisper_full_get_token_text_from_state.argtypes = [POINTER(struct_whisper_context), POINTER(struct_whisper_state), c_int, c_int]
    whisper_full_get_token_text_from_state.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 591
if _libs["whisper"].has("whisper_full_get_token_id", "cdecl"):
    whisper_full_get_token_id = _libs["whisper"].get("whisper_full_get_token_id", "cdecl")
    whisper_full_get_token_id.argtypes = [POINTER(struct_whisper_context), c_int, c_int]
    whisper_full_get_token_id.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 592
if _libs["whisper"].has("whisper_full_get_token_id_from_state", "cdecl"):
    whisper_full_get_token_id_from_state = _libs["whisper"].get("whisper_full_get_token_id_from_state", "cdecl")
    whisper_full_get_token_id_from_state.argtypes = [POINTER(struct_whisper_state), c_int, c_int]
    whisper_full_get_token_id_from_state.restype = whisper_token

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 596
if _libs["whisper"].has("whisper_full_get_token_data", "cdecl"):
    whisper_full_get_token_data = _libs["whisper"].get("whisper_full_get_token_data", "cdecl")
    whisper_full_get_token_data.argtypes = [POINTER(struct_whisper_context), c_int, c_int]
    whisper_full_get_token_data.restype = whisper_token_data

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 597
if _libs["whisper"].has("whisper_full_get_token_data_from_state", "cdecl"):
    whisper_full_get_token_data_from_state = _libs["whisper"].get("whisper_full_get_token_data_from_state", "cdecl")
    whisper_full_get_token_data_from_state.argtypes = [POINTER(struct_whisper_state), c_int, c_int]
    whisper_full_get_token_data_from_state.restype = whisper_token_data

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 600
if _libs["whisper"].has("whisper_full_get_token_p", "cdecl"):
    whisper_full_get_token_p = _libs["whisper"].get("whisper_full_get_token_p", "cdecl")
    whisper_full_get_token_p.argtypes = [POINTER(struct_whisper_context), c_int, c_int]
    whisper_full_get_token_p.restype = c_float

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 601
if _libs["whisper"].has("whisper_full_get_token_p_from_state", "cdecl"):
    whisper_full_get_token_p_from_state = _libs["whisper"].get("whisper_full_get_token_p_from_state", "cdecl")
    whisper_full_get_token_p_from_state.argtypes = [POINTER(struct_whisper_state), c_int, c_int]
    whisper_full_get_token_p_from_state.restype = c_float

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 607
if _libs["whisper"].has("whisper_bench_memcpy", "cdecl"):
    whisper_bench_memcpy = _libs["whisper"].get("whisper_bench_memcpy", "cdecl")
    whisper_bench_memcpy.argtypes = [c_int]
    whisper_bench_memcpy.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 608
if _libs["whisper"].has("whisper_bench_memcpy_str", "cdecl"):
    whisper_bench_memcpy_str = _libs["whisper"].get("whisper_bench_memcpy_str", "cdecl")
    whisper_bench_memcpy_str.argtypes = [c_int]
    whisper_bench_memcpy_str.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 609
if _libs["whisper"].has("whisper_bench_ggml_mul_mat", "cdecl"):
    whisper_bench_ggml_mul_mat = _libs["whisper"].get("whisper_bench_ggml_mul_mat", "cdecl")
    whisper_bench_ggml_mul_mat.argtypes = [c_int]
    whisper_bench_ggml_mul_mat.restype = c_int

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 610
if _libs["whisper"].has("whisper_bench_ggml_mul_mat_str", "cdecl"):
    whisper_bench_ggml_mul_mat_str = _libs["whisper"].get("whisper_bench_ggml_mul_mat_str", "cdecl")
    whisper_bench_ggml_mul_mat_str.argtypes = [c_int]
    whisper_bench_ggml_mul_mat_str.restype = c_char_p

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 614
if _libs["whisper"].has("whisper_log_set", "cdecl"):
    whisper_log_set = _libs["whisper"].get("whisper_log_set", "cdecl")
    whisper_log_set.argtypes = [ggml_log_callback, POINTER(None)]
    whisper_log_set.restype = None

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 15
def WHISPER_DEPRECATED(func, hint):
    return func

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 32
try:
    WHISPER_SAMPLE_RATE = 16000
except:
    pass

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 33
try:
    WHISPER_N_FFT = 400
except:
    pass

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 34
try:
    WHISPER_HOP_LENGTH = 160
except:
    pass

# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 35
try:
    WHISPER_CHUNK_SIZE = 30
except:
    pass

whisper_context = struct_whisper_context# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 77

whisper_state = struct_whisper_state# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 78

whisper_full_params = struct_whisper_full_params# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 428

whisper_context_params = struct_whisper_context_params# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 85

whisper_token_data = struct_whisper_token_data# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 104

whisper_model_loader = struct_whisper_model_loader# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 112

whisper_grammar_element = struct_whisper_grammar_element# /home/dnhkng/Documents/Projects/whisper.cpp/whisper.h: 143

# No inserted files

# No prefix-stripping

