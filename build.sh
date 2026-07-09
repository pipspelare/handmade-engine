#!/bin/bash

filename=$1
clearname="${filename%.*}"

if [ ! -d build ]; then
	mkdir build;
fi

if [ -z $DISPLAY ]; then
	echo "Display environment variable was unset. Setting to ':0'";
 	export DISPLAY=:0;
fi       

if [ $XDG_SESSION_TYPE != "x11" ]; then
	echo "If you are on wayland - install X11 compatibility layer with" #  `command -v Xwayland ` "
	# TODO: check for X11/Xwayland packages
	#
	# if command -v dpkg-query >/dev/null 2>&1; then
	#    dpkg -l | grep -q xwayland && echo "✅ XWayland (Debian/Ubuntu)"
	#elif command -v rpm >/dev/null 2>&1; then
	#    rpm -qa | grep -qi xwayland Xorg && echo "✅ XWayland (RHEL/Fedora)"
	#elif command -v pacman >/dev/null 2>&1; then
	#    pacman -Q xorg-xwayland >/dev/null 2>&1 && echo "✅ XWayland (Arch)"
	fi

gcc -Wall -g src/main.cpp -lX11 -o build/engine

if [ $? == 0 ]; then
	./build/engine
fi
