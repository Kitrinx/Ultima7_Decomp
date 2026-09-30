@echo off
rem IDESTUB source folder name
rem The BC 2.0 IDE compiled each file under its full upper-case path, which
rem __FILE__ keeps. Puts the source at folder\name and writes a stub, name,
rem in this directory that includes it from there.
if not exist \U7\nul md \U7
if not exist %2\nul md %2
copy %1 %2\%3 > nul
echo #include "%2\%3" > %3
