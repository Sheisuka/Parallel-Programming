@echo off
set N=10
for /L %%i in (1,1,%N%) do start /B Lab4.exe %%i
pause