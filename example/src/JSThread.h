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
		JSThead();//Consructor will init JSContext and may be associate with debugger and start a thread as well waiting on semaphore

		void ExecuteFunction(int scriptIndex, string func_name);
		
	private:
		//script and function to execute next
		int mScriptIndex = 0;
		string mFuncName;
		sem_t mSemaphore;
} 
