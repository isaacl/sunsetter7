function send(command) {
  console.log('>> ' + command);
  Module.ccall("queue_command", "number", ["string"], [command]);
}

onmessage = function (e) {
  send(e.data);
}

send("xboard");
send("variant crazyhouse");
send("analyze");
send("easy");
send("sd 15");
send("go");

setTimeout(function () {
  send("force");
}, 2000);
