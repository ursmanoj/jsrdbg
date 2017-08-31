/*
 * A Remote Debugger Example for SpiderMonkey Java Script Engine.
 * Copyright (C) 2014-2015 Slawomir Wojtasiak
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * Character encoding support is definitely broken here. It's quite complex
 * problem, so if you are interested how to handle it correctly head over
 * to the common utility code in $(top_srcdir)/utils
 */

#include <iostream>
#include <locale.h>
#include <stdlib.h>

#include "JSThread.h"

using namespace std;

static const int number_of_threads = 4;

int main(int argc, char **argv) { 
	JSThread threads[number_of_threads];

	int threadIndex, scriptIndex;
	string funcName;

	while(true) {
		cout<<"Enter ThreadIndex: ";
		cin>>threadIndex;

		cout<<"Enter scriptIndex: ";
		cin>>scriptIndex;

		cout<<"Enter Function to call: ";
		cin>>funcName;

		threads[threadIndex].ExecuteFunction(scriptIndex, funcName);
	}
}

