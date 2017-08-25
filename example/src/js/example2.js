print('Ex2:SpiderMonkey is the champ. ');
var globalVar2 = 2000;


var func = function () {
	var a2 = 20;
	a = 28;
	a = 30;
	print("Ex2:Test by Manoj");
}


var func5 = function () {
	var b=27;
	print("Ex2:Test by Manoj2");
	func();
}


var func6 = function () {
	print("Ex2:Test by Manoj3");
	var c =28;
	debugger;
	func5()
}
func6();
//debugger;