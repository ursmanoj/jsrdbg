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

// SpiderMonkey.
#include <jsapi.h>
#include <jsdbgapi.h>

// JSRDBG.
#include <jsrdbg/jsrdbg.h>

using namespace std;
using namespace JS;
using namespace JSR;

static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

// Example script.
//extern char _binary_example_js_start[];
//extern char _binary_example_js_end[];

// Naive implementation of the printing to the standard output.
JSBool JS_fn_print( JSContext *cx, unsigned int argc, Value *vp ) {

    CallArgs args = CallArgsFromVp(argc, vp);

    char *chars = JS_EncodeStringToUTF8( cx, args.get(0).toString() );
    if( !chars ) {
        JS_ReportError( cx, "Cannot convert JS string into a native UTF8 one." );
        return JS_FALSE;
    }

    cout << chars << endl;

    JS_free( cx, chars );

    return JS_TRUE;
}

static JSFunctionSpec JDB_Funcs[] = {
   { "print", JSOP_WRAPPER ( JS_fn_print ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
   { nullptr },
};

// Component responsible for loading script's source code if the JS engine cannot provide it.
class ScriptLoader : public IJSScriptLoader {
public:
	string mScript;

    ScriptLoader() { }

    ~ScriptLoader() { }

    int load( JSContext *cx, const std::string &path, std::string &script ) {
        /*if( path == "example.js" ) {
            script = string(_binary_example_js_start, _binary_example_js_end - _binary_example_js_start);
            return JSR_ERROR_NO_ERROR;
        }*/
        string fullpath = "./src/js/";
        fullpath += path;

		script = readJSFile(fullpath.c_str());
		mScript = script;
		//cout<<"script file contents:" <<script;
		return JSR_ERROR_NO_ERROR;
		
        //return JSR_ERROR_FILE_NOT_FOUND;
    }
private:
	char * readJSFile(const char * fileName) {
		FILE* fp;
		// Open java script file.
		if (!(fp = fopen(fileName, "r"))) {
			cout << "Failed to open javascript: "<<fileName<<endl;
			return NULL;
		}
		long file_len = 0;
		fseek(fp, 0, SEEK_END);
		file_len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		char * buf = new char[file_len + 1];
		// Read java script file.
		int len = fread(buf, 1, file_len, fp);
		buf[file_len] = 0;
		fclose(fp);
		return buf;
	}

};

ScriptLoader loader;

// Runs example script.
bool RunScript( JSContext *cx, JSRemoteDebugger &dbg ) {

    // New global object gets it's own compartments too.
    CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    JS::RootedObject global(cx, JS_NewGlobalObject( cx, &global_class, nullptr, options ));
    if( !global ) {
        cout << "Cannot create global object." << endl;
        return false;
    }

    JSAutoRequest req( cx );
    JSAutoCompartment ac( cx, global );

    if( !JS_InitStandardClasses( cx, global ) ) {
        cout << "Cannot initialize standard classes." << endl;
        return false;
    }

    if ( !JS_DefineFunctions( cx, global, &JDB_Funcs[0] ) ) {
        cout << "Cannot initialize utility functions." << endl;
        return false;
    }
#if 1 //register only after script is evaluated, and before calling function
    // Register newly created global object into the debugger,
    // in order to make it debuggable.
    if( dbg.addDebuggee( cx, global ) != JSR_ERROR_NO_ERROR ) {
        cout << "Cannot add debuggee." << endl;
        return false;
    }
#endif
    Value rval;
    JSBool result;

    cout << "Use jrdb command in order to connect to the debugger." << endl;
    cout << "Application is suspended." << endl;

    // Run Garbage collector.
    JS_GC( JS_GetRuntime( cx ) );

    // Runs JS script.
    string tmp;
	loader.load(NULL, "example.js",tmp);
    //result = JS_EvaluateScript( cx, global.get(), _binary_example_js_start, _binary_example_js_end  -_binary_example_js_start, "example.js", 0, &rval);
    //result = JS_EvaluateScript( cx, global.get(), loader.mScript.c_str(), loader.mScript.length(), "3630f6c5-e056-4c64-bb07-c4b134bae4d7.js", 0, &rval);
	//cout<<"Script file:(len="<<loader.mScript.length()<<")"<<loader.mScript<<endl;
    JSScript * jss = JS_CompileScript(cx, global.get(), loader.mScript.c_str(), loader.mScript.length(), "3630f6c5-e056-4c64-bb07-c4b134bae4d7.js", 0);
	jsval res;
	if (!jss) {
		cout << "Error compiling" << endl;
		return false;   /* compilation error */
	}
	 if (!JS_ExecuteScript(cx, global.get(), jss, &res)) {
		cout << "Error JS_ExecuteScript" << endl;
		return false;   /* compilation error */
	}
#if 0//2nd script
	loader.load(NULL, "example2.js",tmp);
	result = JS_EvaluateScript( cx, global.get(), loader.mScript.c_str(), loader.mScript.length(), "example2.js", 0, &rval);
	cout<<"Script file:(len="<<loader.mScript.length()<<")"<<loader.mScript<<endl;
#endif
#if 0 
    // Register newly created global object into the debugger,
    // in order to make it debuggable.
    if( dbg.addDebuggee( cx, global ) != JSR_ERROR_NO_ERROR ) {
        cout << "Cannot add debuggee." << endl;
        return false;
    }
#endif
	int input;

	cout<<"Enter any key to run func3"<<endl;
	cin>>input;
	//while(true);
	
	jsval argv[1];
	JS::RootedValue r(cx);	

	if (JS_CallFunctionName(cx, global, "func3", 1, argv, r.address())) {
    	cout<<"Successfull function call"<<endl;    
	}

    cout << "Application has been finished.result: "<<result << endl;

    return static_cast<bool>( result );
}

// Initializes debugger and runs script into its scope.
bool RunDbgScript( JSContext *cx ) {

    // Initialize debugger.

    // Configure remote debugger.
    static bool portOpened = false;
	//static JSRemoteDebuggerCfg cfg;
	JSRemoteDebuggerCfg cfg;

	if(portOpened == false) {
		portOpened = true;
		cout<<"Opening port for debugging"<<endl;
	    
	    cfg.setTcpHost(JSR_DEFAULT_TCP_BINDING_IP);
	    cfg.setTcpPort(JSR_DEFAULT_TCP_PORT);
	}
	JSRemoteDebugger dbg(cfg);
    //cfg.setScriptLoader(&loader);
	//string tmp;
	//loader.load(NULL, "example.js",tmp);

    // Configure debugger engine.
    JSDbgEngineOptions dbgOptions;
    // Suspend script just after starting it.
	dbgOptions.suspended();
	dbgOptions.continueWhenNoConnections();

    //JSRemoteDebugger dbg( cfg );
    

    if( dbg.install( cx, "example-JS", dbgOptions ) != JSR_ERROR_NO_ERROR ) {
        cout << "Cannot install debugger." << endl;
        return false;
    }

	static bool started = false;
	if(started == false) {
		started = true;
		int ret=dbg.start();
	    if( ret!= JSR_ERROR_NO_ERROR ) {
	        dbg.uninstall( cx );
	        cout << "Cannot start debugger: " <<ret<< endl;
	        return false;
	    }
	}

    bool result = RunScript( cx, dbg );

    dbg.stop();
    dbg.uninstall( cx );

    return result;
}

void * context_thread(void *arg) {

	//char * scriptName= arg;
    setlocale(LC_ALL, "");

    // Creates new inner compartment in the runtime.
    JSRuntime *rt = JS_NewRuntime( 8L * 1024 * 1024, JS_NO_HELPER_THREADS );
    if( !rt ) {
        cout << "Cannot initialize runtime." << endl;
        exit(1);
    }

    JS_SetNativeStackQuota(rt, 1024 * 1024);

    JS_SetGCParameter(rt, JSGC_MAX_BYTES, 0xffffffff);

    JSContext *cx = JS_NewContext( rt, 8192 );
    if( !cx ) {
        JS_DestroyRuntime( rt );
        JS_ShutDown();
        cout << "Cannot initialize JS context." << endl;
        exit(1);
    }

    if( !RunDbgScript( cx ) ) {
        cout << "Application failed." << endl;
    }

    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
	return nullptr;
}

static pthread_t thread1;
static pthread_t thread2;

static void startTestThread(pthread_t *ptr_thread) {

	int ret = pthread_create(ptr_thread, NULL, &context_thread, NULL);
	cout <<"Thread create: " <<ret<<endl;
}

void tester() {
	//string path = new string("/home/manoj/Test/out/data/read_write/dth/device_def/3630f6c5-e056-4c64-bb07-c4b134bae4d7/profile.js");
}

int main(int argc, char **argv) { 
	startTestThread(&thread1);
	//startTestThread(&thread2);
	while(true);
}

