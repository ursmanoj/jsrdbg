#include "JSThread.h"

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

static JSBool JS_fn_print( JSContext *cx, unsigned int argc, Value *vp ) {

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

/*struct script_info {
	string path;
	JSScript * jss;
};*/

static string scripts[] = {
	"example.js",
	"example1.js",
	"example2.js",
	"example3.js"
};
static const int num_of_scripts = sizeof(scripts)/sizeof(scripts[0]);

void JSThread::compileAllScripts(JSContext * cx, JS::RootedObject *p_global, JSThread * jsThread) {
	// TODO: compile and save JSS objects of all scripts, make an array like above

	for(int i=0;i<num_of_scripts;i++) {
		//compile and execute all scripts so their functions are ready to execute anytime
		string tmp;
		loader.load(NULL, scripts[i],tmp);

		if(i==0) {
			//add dummy script
			//loader.mScript.append("\r\n");
			//TODO: find a way to append newline and then compile
			char * dummyFunc = "\nvar processDebugCommand = function() {" "\n\tprint(\"Dummy func, so pending debug request also executed\");\n}";
			loader.mScript.append(dummyFunc);
		}
		
	    jsThread->mJssList[i] = JS_CompileScript(cx, p_global->get(), loader.mScript.c_str(), loader.mScript.length(), scripts[i].c_str(), 0);

		jsval res;
		if (!jsThread->mJssList[i]) {
			cout << "Error compiling scriptIndex: "<<i << endl;
			return;   /* compilation error */
		}
	}	 
}

// TODO: later rename the private functions to more appropriate names
bool JSThread::RunScript( JSContext *cx, JSThread * jsThread) {

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
    JSRemoteDebugUtil::startDebugger(cx, &global);
#endif
    Value rval;
    JSBool result;

    // Run Garbage collector.
    JS_GC( JS_GetRuntime( cx ) );

    // Runs JS script.
    compileAllScripts(cx, &global, jsThread);
    //int input;

	//cout<<"Enter any key to run func3"<<endl;
	//cin>>input;
	while(true) {
		sem_wait(&jsThread->mSemaphore);
		jsval argv[1];
		JS::RootedValue r(cx);
		jsval res;

		if (!JS_ExecuteScript(cx, global.get(), jsThread->mJssList[jsThread->mScriptIndex], &res)) {
			cout << "Error JS_ExecuteScript" << endl;
			return false;   /* compilation error */
		}

		cout<<"File->FuncName: "<<scripts[jsThread->mScriptIndex]<<"->"<<jsThread->mFuncName<<" ";
		if (JS_CallFunctionName(cx, global, jsThread->mFuncName.c_str(), 1, argv, r.address())) {
	    	cout<<"Successfull function call"<<endl;    
		} else
			cout<<"FAILED!!!!"<<endl;
	}

    cout << "Application has been finished.result: "<<result << endl;

    return static_cast<bool>( result );
}

// Initializes debugger and runs script into its scope.
bool JSThread::RunDbgScript( JSContext *cx, JSThread * jsThread ) {

    // Initialize debugger.

   JSRemoteDebugUtil::registerContext(cx, jsThread->mThreadName);

    bool result = RunScript(cx, jsThread);

    return result;
}

void * JSThread::context_thread(void *arg) {

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

    if( !RunDbgScript( cx, (JSThread *)arg ) ) {
        cout << "Application failed." << endl;
    }

    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
	return nullptr;
}

void JSThread::startTestThread() {
	int ret = pthread_create(&mThread, NULL, &context_thread, this);
	//cout <<"Thread create: " <<ret<<endl;
}

//Constructor
JSThread::JSThread(string name) {
	if (sem_init(&mSemaphore, 0, 0) == -1)
		cout<<"Semaphore failed"<<endl;

	mJssList = new JSScript *[num_of_scripts];
	mThreadName = name;
	startTestThread();
}

void JSThread::ExecuteFunction(int scriptIndex, string func_name) {
	mScriptIndex = scriptIndex;
	mFuncName = func_name;
	sem_post(&mSemaphore);
}

JSRemoteDebuggerCfg JSRemoteDebugUtil::cfg;
JSRemoteDebugger JSRemoteDebugUtil::dbg;
bool JSRemoteDebugUtil::started;

JSRemoteDebugUtil::JSRemoteDebugUtil() {
	// Initialize debugger.

    //JSRemoteDebuggerCfg cfg;
	cout<<"Setting up TCP Port for remote debugging"<<endl;
    cfg.setTcpHost(JSR_DEFAULT_TCP_BINDING_IP);
    cfg.setTcpPort(JSR_DEFAULT_TCP_PORT);
	cout<<"Hopefully TCP Port setup done";
	started = false;

#if 0// TODO: to be moved to deinit (destructor may be)
    dbg.stop();
    dbg.uninstall( cx );
#endif
    //return result;
}

JSRemoteDebugUtil::~JSRemoteDebugUtil() {
	dbg.stop();
    //dbg.uninstall( cx );
}

void JSRemoteDebugUtil::registerContext(JSContext *cx, string name) {
	// Configure debugger engine.
    JSDbgEngineOptions dbgOptions;
    // Suspend script just after starting it.
    //dbgOptions.suspended();
    dbgOptions.continueWhenNoConnections();

	int ret = dbg.install( cx, name.c_str(), dbgOptions );
	
	if( ret != JSR_ERROR_NO_ERROR ) {
		cout << "Cannot install debugger(err="<<ret<<").";
		return;
	}

	if(started)
		return;
	started = true;
	ret = dbg.start() ;
	if( ret != JSR_ERROR_NO_ERROR ) {
		dbg.uninstall( cx );
		cout << "Cannot start debugger(err="<<ret<<").";
		started = false;
		return;
	}	
}

void JSRemoteDebugUtil::startDebugger(JSContext *cx, JS::RootedObject *global) {
	int ret = dbg.addDebuggee( cx, *global );
	
	if( ret != JSR_ERROR_NO_ERROR ) {
	    //dbg.uninstall( cx );
	    cout << "Cannot add debuggee.(err="<<ret << ")";
	}
}

void JSRemoteDebugUtil::stopDebugger(JSContext *cx, JS::RootedObject *global) {
	int ret = dbg.removeDebuggee( cx, *global );
	
	if( ret != JSR_ERROR_NO_ERROR ) {
	    //dbg.uninstall( cx );
	    cout << "Cannot remove debuggee.(err="<<ret << ")";
	}
}

