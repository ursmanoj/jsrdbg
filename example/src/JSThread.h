//include thread
//include semaphore
//include jsapi
// SpiderMonkey.
#include <jsapi.h>
#include <jsdbgapi.h>

// JSRDBG.
#include <jsrdbg/jsrdbg.h>

//Semaphore
#include <semaphore.h>


using namespace std;
using namespace JS;
using namespace JSR;

class JSThread {
	public:
		JSThread(string name);//Consructor will init JSContext and may be associate with debugger and start a thread as well waiting on semaphore

		void ExecuteFunction(int scriptIndex, string func_name);
		
	private:
		//script and function to execute next
		int mScriptIndex = 0;
		string mFuncName;
		sem_t mSemaphore;
		pthread_t mThread;
		JSScript ** mJssList;
		string mThreadName;

		static void compileAllScripts(JSContext * cx, JS::RootedObject *p_global, JSThread * jsThread);
		static bool RunScript( JSContext *cx, JSThread * jsThread);
		static bool RunDbgScript( JSContext *cx , JSThread * jsThread);
		static void * context_thread(void *arg);
		void startTestThread() ;
};

class JSRemoteDebugUtil {
public:
	JSRemoteDebugUtil();
	~JSRemoteDebugUtil();
	static void registerContext(JSContext *cx, string name);
	static void startDebugger(JSContext *cx, JS::RootedObject *global);
	static void stopDebugger(JSContext *cx, JS::RootedObject *global);

private:
	static JSRemoteDebuggerCfg cfg;
	static JSRemoteDebugger dbg;
	static bool started;
};

