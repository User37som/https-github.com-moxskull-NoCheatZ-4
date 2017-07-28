#!/usr/bin/env python3
"""
Check if all given binaries work with the given glibc version.
glibc_check.py 2.11 bin [bin ...]
rc = 0 means "yes", rc = 1 means "no".
"""

import re
import subprocess
import sys

verbose = True
objdump = "objdump -p %s"
glibc_re = re.compile(r'GLIBC_([0-9]\.[0-9]+(\.[0-9]+)?)')
glibcxx_re = re.compile(r'GLIBCXX_([0-9]\.[0-9]+(\.[0-9]+)?)')

def parse_version(v):
	k = v.split('.')
	if len(k) > 2:
		return int(k[0]), int(k[1]), int(k[2])
	else:
		return int(k[0]), int(k[1]), 0

def format_version(version):
	return "%d.%d.%d" % version

def main():
	given_glibc = parse_version(sys.argv[1])
	given_glibcxx = parse_version(sys.argv[2])
	filenames = sys.argv[3:]
	
	overall_glibc_versions = set()
	overall_glibcxx_versions = set()
	for filename in filenames:
		try:
			output = subprocess.check_output(objdump % filename, shell=True, stderr=subprocess.STDOUT)
			output = output.decode('utf-8')
			versions_glibc = set(parse_version(match.group(1)) for match in glibc_re.finditer(output))
			versions_glibcxx = set(parse_version(match.group(1)) for match in glibcxx_re.finditer(output))
			requires_glibc = max(versions_glibc)
			requires_glibcxx = max(versions_glibcxx)
			overall_glibc_versions.add(requires_glibc)
			overall_glibcxx_versions.add(requires_glibcxx)
			if verbose:
				print("%s GLIBC_%s" % (filename, format_version(requires_glibc)))
				print("%s GLIBCXX_%s" % (filename, format_version(requires_glibcxx)))
		except subprocess.CalledProcessError as e:
			if verbose:
				print("%s errored." % filename)
	
	wanted_glibc = max(overall_glibc_versions)
	wanted_glibcxx = max(overall_glibcxx_versions)
	ok = given_glibc >= wanted_glibc
	
	if verbose:
		if ok:
			print("The binaries work with the given glibc %s." %
				format_version(given_glibc))
		else:
			print("ERROR : The binaries do not work with the given glibc %s. "
				"Minimum is: %s" % (format_version(given_glibc), format_version(wanted_glibc)))
	
	ok2 = given_glibcxx >= wanted_glibcxx
	
	if verbose:
		if ok2:
			print("The binaries work with the given glibcxx %s." %
				format_version(given_glibcxx))
		else:
			print("ERROR : The binaries do not work with the given glibcxx %s. "
				"Minimum is: %s" % (format_version(given_glibcxx), format_version(wanted_glibcxx)))
				
	return ok2 and ok


if __name__ == '__main__':
	ok = main()
	sys.exit(0 if ok else 1)
