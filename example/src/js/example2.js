print('Example 2');
var globalVar = 1000;
var func = function () {
	var a = 10;
	a = 18;
	
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
//func3();
//debugger;