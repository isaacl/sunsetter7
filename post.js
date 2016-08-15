onmessage = function (e) {
  if (e.data == 'quit') close();
  Module.ccall("queue_command", "number", ["string"], [e.data]);
}
