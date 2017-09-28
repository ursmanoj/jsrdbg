print('Example 0');
var globalVar = 1000;
var func = function () {
	var a = 10;
	a = 18;
	
	var car = {type:"Fiat", model:"500", color:"white"};
	
	var nested = {mycar:car, name:"Morning"};
	
	print("Before debugger");
	//debugger;
	print("Test by Manoj");
	a = 20;
	a =28;
}
var func2 = function () {
	var b=17;
	print("Test by Manoj2");
	func();
}
var func3 = function () {
	print("Test by Manoj3");
	var c =18;
	//debugger;
	func2()
}
var dummy = function() {
	print("Dummy func, so pending debug request also executed");
}

var ipFuncCB = function() {
	print("This is Control Box from example.js");
}

var ipCmd = function() {
	print("This is Command to ED");
}

var ipEvent = function() {
	print("This is ED Event parsing");
}

var testFunc = function() {
	print("testFunc");
	noVarFunc();
}

var loopFunc=  function () {
	print("Test loopFunc");
	var a=true;
	
	while(a==true) {
		var c =18;
		print("loop");
		c = 20;
		c = 10;
		func();
	}
}
//func3();
//debugger;