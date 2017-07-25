#!/usr/bin/python

import os
import subprocess

def get_git_tag():
	return str(subprocess.check_output(['git', 'describe', '--tags']).decode('ascii')).strip()

def get_git_long():
	return str(subprocess.check_output(['git', 'describe', '--long', '--tags']).decode('ascii')).strip()

version_short = get_git_tag()
version_long = get_git_long()

file_content = '#include "version.h"\n\nchar const * const NCZ_VERSION_GIT = "' + version_long.replace('"', '') + '";\nchar const * const NCZ_VERSION_GIT_SHORT = "' + version_short.replace('"', '') + '";\n\n'

print(file_content)

with open("../server-plugin/Code/version.cpp", "w") as outfile:
	outfile.write(file_content)
