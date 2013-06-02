import os
import re
import sys


# Number of bits of Python
if sys.maxsize <= 2 ** 32:
    bits = 32
else:
    bits = 64


def find_dll():
    """Attempts to find the location of a JRE.
    """
    java_home = os.getenv('JAVA_HOME')
    if sys.platform == 'win32':
        def find_jre(java_dir):
            JRE_NAME = re.compile(r'^jre([0-9])$')
            JDK_NAME = re.compile(r'^jdk1\.([0-9])\.([0-9])_([0-9]+)$')

            if not os.path.exists(java_dir):
                return None
            # Trying JREs
            choice = None
            for subdir in os.listdir(java_dir):
                m = JRE_NAME.match(subdir)
                if m:
                    version = m.groups()
                    if choice is None or version > choice[1]:
                        choice = subdir, version
            if choice is not None:
                return os.path.join(java_dir, choice[0])

            # Trying JDKs
            choice = None
            for subdir in os.listdir(java_dir):
                m = JDK_NAME.match(subdir)
                if m:
                    version = m.groups()
                    if choice is None or version > choice[1]:
                        choice = subdir, version
            if choice is not None:
                return os.path.join(java_dir, choice[0], 'jre')
            else:
                return None

        if java_home:
            # JAVA_HOME might be set to a JDK; in that case we use the 'jre'
            # subdirectory
            java_home_jre = os.path.join(java_home, 'jre')
            if os.path.exists(java_home_jre):
                java_home = java_home_jre
                sys.stderr.write("Using JRE from JAVA_HOME environment "
                                 "variable (in jre/ subdir)\n")
            else:
                sys.stderr.write("Using JRE from JAVA_HOME environment "
                                 "variable\n")
        elif os.path.exists('C:\\Program Files (x86)'):
            if bits == 32:
                java_home = find_jre('C:\\Program Files (x86)\\Java')
            else:
                java_home = find_jre('C:\\Program Files\\Java')
        else:
            java_home = find_jre('C:\\Program Files\\Java')

        return os.path.join(java_home, 'bin/client/jvm.dll')
    elif sys.platform == 'darwin':
        if java_home:
            if os.path.isdir(java_home):
                dll_file = os.path.join(java_home, 'JavaVM')
                if os.path.isfile(dll_file):
                    return dll_file
        # Apparently the DLL is always at the same place on OSX
        return '/System/Library/Frameworks/JavaVM.framework/JavaVM'
    else:
        # If JAVA_HOME is set, only consider this one
        if java_home:
            java_home_jre = os.path.join(java_home, 'jre')
            # JAVA_HOME might be set to a JDK; in that case we use the 'jre'
            # subdirectory
            if os.path.exists(java_home_jre):
                java_home = [java_home_jre]
                sys.stderr.write("Using JRE from JAVA_HOME environment "
                                 "variable (in jre/ subdir)\n")
            else:
                java_home = [java_home]
                sys.stderr.write("Using JRE from JAVA_HOME environment "
                                 "variable\n")
        # Else, consider all the dirs in /usr/lib/jvm
        else:
            java_home = []
            for directory in os.listdir('/usr/lib/jvm'):
                if directory[0] != '.':
                    directory = os.path.join('/usr/lib/jvm', directory)
                    java_home.append(directory)

            # Sort these possible homes

            if bits == 32:
                bitskey = lambda d: 0 if 'i386' in d else 100
            else:
                bitskey = lambda d: 100 if 'i386' in d else 0
            usual_name = re.compile(r'^.+/java-([0-9]+)-[^/]+$')
            def key(d):
                m = usual_name.match(d)
                if m is None:
                    r = 0
                else:
                    r = -int(m.group(1))
                # If on 32 bits, put 'i386' installs at the start, else put
                # them at the end
                if bits == 32 and 'i386' not in d:
                    return r + 100
                elif bits == 64 and 'i386' in d:
                    return r + 100
                else:
                    return r
            java_home = sorted(java_home, key=key)

        # Find the DLL
        for d in java_home:
            d = os.path.join(d, 'lib')
            if os.path.isdir(d):
                for dd in os.listdir(d):
                    dd = os.path.join(d, dd)
                    if os.path.isdir(dd):
                        l = os.path.join(dd, 'client/libjvm.so')
                        if os.path.isfile(l):
                            return l
                        l = os.path.join(dd, 'server/libjvm.so')
                        if os.path.isfile(l):
                            return l
        return None


if __name__ == '__main__':
    dll = find_dll()
    print dll
    sys.exit(0 if dll else 1)
