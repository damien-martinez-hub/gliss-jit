#!/usr/bin/python3

from distutils.core import setup, Extension

module = Extension(
	"$(proc)gliss",
	sources = ["python.c"],
	include_dirs = ["../include"],
	libraries = ["$(proc)"],
	library_dirs = ["../src"]
)

setup(
	name = "$(PROC)Lib",
	version = "1.0",
	description = "$(PROC) emulation",
	ext_modules = [module]
)
