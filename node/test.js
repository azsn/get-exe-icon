const geticon = require('./build/Release/getexeicon');
const fs = require('fs');

const icoByFile = geticon.getIconFromFile("C:\\Windows\\explorer.exe");
//const icoByPid = geticon.getIconFromPid(8764);
//const icoByPidNoPNG = geticon.getIconFromPid(8764, false);
//const icoDefault = geticon.getDefaultExeIcon();

console.log(icoByFile);
//console.log(icoByPid);
//console.log(icoByPidNoPNG);
//console.log(icoDefault);

fs.writeFile("build\\out_file.ico", icoByFile, function(err) {
	console.log("Saving byFile to build\\out_file.ico. Error:", err);
});

//fs.writeFile("build\\out_pid.ico", icoByPid, function(err) {
//	console.log("Saving byPid to build\\out_pid.ico. Error:", err);
//});
//
//fs.writeFile("build\\out_pid_nopng.ico", icoByPidNoPNG, function(err) {
//	console.log("Saving byPidNoPNG to build\\out_pid_nopng.ico. Error:", err);
//});

//fs.writeFile("build\\default.ico", icoDefault, function(err) {
//	console.log("Saving default to build\\default.ico. Error:", err);
//});
