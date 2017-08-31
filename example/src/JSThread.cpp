#include "JSThread.h"

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
static JSFunctionSpec JDB_Funcs[] = {
   { "print", JSOP_WRAPPER ( JS_fn_print ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
   { nullptr },
};

struct script_info {
	string path;
	JSScript * jss;
};

static script_info scripts[] = {
	{"example.js", NULL},
	{"example1.js", NULL},
	{"example2.js", NULL},
	{"example3.js" NULL}
};
static int num_of_scripts = sizeof(scripts)/sizeof(scripts[0]);

private void JSThread::compileAllScripts(JSContext * cx, JS::RootedObject *p_global) {
	// TODO: compile and save JSS objects of all scripts, make an array like above
	//compile and execute all scripts so their functions are ready to execute anytime
	string tmp;
	loader.load(NULL, "example.js",tmp);
    //result = JS_EvaluateScript( cx, global.get(), _binary_example_js_start, _binary_example_js_end  -_binary_example_js_start, "example.js", 0, &rval);
    //result = JS_EvaluateScript( cx, global.get(), loader.mScript.c_str(), loader.mScript.length(), "3630f6c5-e056-4c64-bb07-c4b134bae4d7.js", 0, &rval);
	//cout<<"Script file:(len="<<loader.mScript.length()<<")"<<loader.mScript<<endl;
    JSScript * jss = JS_CompileScript(cx, *p_global.get(), loader.mScript.c_str(), loader.mScript.length(), "3630f6c5-e056-4c64-bb07-c4b134bae4d7.js", 0);
	jsval res;
	if (!jss) {
		cout << "Error compiling" << endl;
		return false;   /* compilation error */
	}
	 
	
}

private bool JSThread::RunScript( JSContext *cx) {

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
    compileAllScripts(cx);
    //int input;

	//cout<<"Enter any key to run func3"<<endl;
	//cin>>input;
	while(true) {
		sem_wait(&mSemaphore);
		jsval argv[1];
		JS::RootedValue r(cx);
		jsval res;

		if (!JS_ExecuteScript(cx, global.get(), scripts[mScriptIndex].jss, &res)) {
			cout << "Error JS_ExecuteScript" << endl;
			return false;   /* compilation error */
		}

		if (JS_CallFunctionName(cx, global, mFuncName.c_str(), 1, argv, r.address())) {
	    	cout<<"Successfull function call"<<endl;    
		}
	}

    cout << "Application has been finished.result: "<<result << endl;

    return static_cast<bool>( result );
}

// Initializes debugger and runs script into its scope.
private bool JSThread::RunDbgScript( JSContext *cx ) {

    // Initialize debugger.

   JSRemoteDebugUtil::registerContext(cx);

    bool result = RunScript(cx);

    return result;
}

private void * JSThread::context_thread(void *arg) {

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

void JSThread::startTestThread() {
	int ret = pthread_create(&mThread, NULL, &context_thread, NULL);
	cout <<"Thread create: " <<ret<<endl;
}

//Constructor
JSThread::JSThread() {
	if (sem_init(&mSemaphore, 0, 0) == -1)
		cout<<"Semaphore failed"<<endl;

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

void JSRemoteDebugUtil::registerContext(JSContext *cx) {
	// Configure debugger engine.
    JSDbgEngineOptions dbgOptions;
    // Suspend script just after starting it.
    //dbgOptions.suspended();
    dbgOptions.continueWhenNoConnections();

	int ret = dbg.install( cx, "SpmWrapper", dbgOptions );
	
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

