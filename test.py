import subprocess
import threading
import unittest
import os
import signal
import time
import math


class SunsetterTest(unittest.TestCase):
    def setUp(self):
        self.p = open_process("./sunsetter")
        xboard(self.p)

    def tearDown(self):
        kill_process(self.p)

    def test_white_mate_in_1_wtm(self):
        r = sunsetter_go(self.p, "2k5/Q7/3K4/8/8/8/8/8/ w - - 4 3", [], 1000)
        print(r)
        self.assertEqual(r["score"]["mate"], 1)

    def test_white_mate_in_1_btm(self):
        # TODO
        pass
        # r = sunsetter_go(self.p, "3k4/Q7/3K4/8/8/8/8/8/ b - - 7 4", [], 1000)
        # print(r)
        # self.assertEqual(r["score"]["mate"], 1)

    def test_white_mate_in_2_wtm(self):
        r = sunsetter_go(self.p, "5k2/2Q5/3K4/8/8/8/8/8/ w - - 10 6", [], 1000)
        print(r)
        self.assertEqual(r["score"]["mate"], 2)


def open_process(command, _popen_lock=threading.Lock()):
    kwargs = {
        "stdout": subprocess.PIPE,
        "stderr": subprocess.STDOUT,
        "stdin": subprocess.PIPE,
        "shell": False,
        "bufsize": 1,
        "universal_newlines": True,
    }

    try:
        # Windows
        kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
    except AttributeError:
        # Unix
        kwargs["preexec_fn"] = os.setpgrp

    with _popen_lock:
        return subprocess.Popen(command, **kwargs)


def kill_process(p):
    try:
        # Windows
        p.send_signal(signal.CTRL_BREAK_EVENT)
    except AttributeError:
        # Unix
        os.killpg(p.pid, signal.SIGKILL)


def recv(p):
    while True:
        line = p.stdout.readline()
        if line == "":
            raise EOFError()

        line = line.rstrip()
        print(">>", line)

        if line:
            return line


def send(p, line):
    print("<<", line)
    p.stdin.write(line + "\n")
    p.stdin.flush()


def xboard(p):
    send(p, "xboard")

    while True:
        line = recv(p)
        if line.startswith(" Sunsetter"):
            pass
        elif line.startswith(" "):
            continue
        elif line.startswith("Created "):
            continue
        elif line == "tellics gameend4":
            return
        elif line.startswith("tellics "):
            continue
        else:
            raise ValueError("unexpected engine output: %s" % (line, ))


def sunsetter_go(p, position, moves, movetime, maxdepth=None):
    if position == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1":
        send(p, "reset")
    else:
        send(p, "setboard %s" % position)

    send(p, "force")
    send(p, "easy")

    for move in moves:
        send(p, move)

    start = time.time()

    if maxdepth is not None:
        send(p, "sd %d" % max(5, maxdepth))
    else:
        send(p, "sd 0")

    send(p, "go")

    info = {}
    info["bestmove"] = None
    info["depth"] = 0
    info["score"] = {}

    cp = None

    done = False

    while True:
        line = recv(p).strip()
        if line == "tellics done":
            if done:
                return info
            else:
                logging.error("Unexpected tellics done")
        elif line.startswith("T-hits:") or line.startswith("Time"):
            continue
        elif line.startswith("set fixed depth to "):
            continue
        elif line.startswith("bookfile "):
            continue
        elif line.startswith("1-0 ") or line.startswith("0-1 "):
            if not done:
                done = True
                send(p, "tellics done")
                info["score"]["mate"] = 0
                info["depth"] = 0
        elif line.startswith("1/2-1/2 "):
            if not done:
                done = True
                send(p, "tellics done")
                info["score"]["cp"] = 0
                info["depth"] = 0
        elif line.startswith("move "):
            if not done:
                done = True
                send(p, "force")
                send(p, "tellics done")
                info["bestmove"] = line.split()[1]

                if cp is not None and abs(cp) >= 20000:
                    info["score"]["mate"] = math.copysign((30000 - abs(cp)) // 10, cp)
                elif cp is not None:
                    info["score"]["cp"] = cp

                if not info.get("pv", None):
                    info["pv"] = info["bestmove"]

                if info.get("time", None) and "nodes" in info:
                    info["nps"] = info["nodes"] * 1000 // info["time"]
        elif line[0].isdigit():
            if not done:
                depth, score, stime, nodes, pv = line.split(None, 4)
                info["depth"] = int(depth)
                cp = int(score)
                info["time"] = int(stime) * 10
                info["nodes"] = int(nodes)
                info["pv"] = " ".join(move for move in pv.split()
                                      if move.replace("@", "").replace("=", "").isalnum())

                if time.time() - start > movetime / 1000 or (maxdepth is not None and info["depth"] >= maxdepth):
                    send(p, "?")
        elif line.startswith("Found move: "):
            if not done:
                # Found move: d7d5 -10 fply: 11  searches: 840015 quiesces: 113140
                _, _, move, score, _, fply, _, searches, _ = line.split(None, 8)
                info["bestmove"] = move
                info["depth"] = int(fply) + 1
                info["nodes"] = int(searches)
                cp = int(score)
        elif line.startswith("tellics whisper "):
            if not done:
                # tellics whisper :-| 12: d7d5 d2d3 b8c6 c1e3 f8b4 b1c3 g8f6 (M: +0 D: +0 C: -10 KW: +0 KB: +0 )
                _, _, _, depth, trail = line.split(None, 4)
                info["depth"] = int(depth.rstrip(":"))
                pv = []
                for move in trail.split():
                    if move.replace("@", "").replace("=", "").isalnum():
                        pv.append(move)
                    else:
                        break
                if pv:
                    info["pv"] = " ".join(pv)
        elif line.startswith("tellics "):
            # tellics kibitz, tellics gameend...
            continue
        else:
            raise ValueError("Unexpected engine output: %s" % (line, ))


if __name__ == "__main__":
    unittest.main()
