@echo off
set NGAGESDK=%~dp0/..
cmake -P %~dp0/setup.cmake
