import os
import platform
import re
import sys


# Number of bits of Python
if sys.maxsize <= 2**32:
    bits = 32
else:
    bits = 64


USING_WINDOWS = platform.system() in ['Windows', 'Microsoft']


def find_dll():
    # Attempt to find the location of a JRE
    if USING_WINDOWS:
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

        java_home = os.getenv('JAVA_HOME')
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
    else:
        # FIXME : Where can I find this on Linux?
        return None
