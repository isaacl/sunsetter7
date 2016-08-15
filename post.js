function send(command) {
  Module.ccall("queue_command", "number", ["string"], [command]);
}

send("xboard");
send("variant crazyhouse");
send("analyze");
send("easy");
send("sd 9");
send("go");

setTimeout(function () {
  send("quit");
}, 10000);
