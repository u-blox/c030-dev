::This batch file can be called with command line parameters as in the following examples:
::
::Run the default tests 3 times:
::tests 3
::Run the default tests 100 times and send the output to the file "temp 1.txt":
::tests 100 "temp 1.txt"
::Run the tests using "mbed test -v -n driver-battery-gauge-bq*" 10 times and send the output to the file temp.txt:
::tests 10 temp.txt "mbed test -v -n driver-battery-gauge-bq*"

@echo off
set exe=mbed test -v -n driver*
set count=100
set outfile=test_out.txt

if not 'x%1'=='x' (set count=%1)
if not 'x%2'=='x' (set "outfile=%~2")
if not 'x%3'=='x' (set exe=%~3)

echo Running "%exe%" %count% time(s), or until there is an error, output going to "%outfile%".
for /L %%a IN (1, 1, %count%) DO (
echo Run %%a of %count%: "%exe%"
%exe% > "%outfile%" 2>&1
if errorlevel 1 goto errorEnd
)
echo All runs completed.
goto end

:errorEnd
echo non-zero errorlevel was returned (%errorlevel%), check "%outfile%" for the reason.

:end