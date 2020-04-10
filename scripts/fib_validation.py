#!/usr/bin/env python3
import re
import os
import time
import pathlib
import argparse
import subprocess
import urllib.request

def fetch_fib_number(index):
    resp = urllib.request.urlopen(f"http://www.protocol5.com/Fibonacci/{index}.htm")
    html = [h.strip().decode('utf-8') for h in resp.readlines()]
    represents = []
    for line in html:
        match = re.search(r"<li><h4>.+?</h4><div>(.+?)</div></li>", line)
        if match:
            represents.append(match.group(1))

    return {
        "base2": represents[0],
        "base3": represents[1],
        "base5": represents[2],
        "base6": represents[3],
        "base8": represents[4],
        "base10": represents[5],
        "radix16": represents[6],
        "radix36": represents[7],
        #"radix63404": represents[8]
    }

def main(execute, index, base):
    start_time = time.time()
    ret = subprocess.Popen([execute, str(index)], stdout=subprocess.PIPE)
    out, err = ret.communicate()
    while ret.returncode != 0:
        pass
    else:
        duration = time.time() - start_time
        print(f"It takes {duration} seconds")

    result = out.decode("utf-8").strip()
    expect = fetch_fib_number(args.index)[base]

    if result == expect:
        print("Pass")
        exit(0)
    else:
        print("No pass")
        exit(1)

if __name__ == '__main__':
    #parser = argparse.ArgumentParser()
    #parser.add_argument("executable", type=str, help="Fibonacci executable file")
    #parser.add_argument("index", type=str, help="Fibonacci number index")
    #parser.add_argument("-b", action='store', dest="base", help="Fibonacci number base (default: base10)")
    #args = parser.parse_args()

    #FIB_EXE = str(pathlib.Path(args.executable).resolve())
    #FIB_INDEX = int(args.index)
    #FIB_NUMBER_BASE = args.base

    #if not os.path.exists(FIB_EXE):
    #    print("Fibonacci executable is not exist")
    #    exit(1)

    #if FIB_INDEX < 0:
    #    print("Fibonacci index is less than 0")
    #    exit(1)

    #if not FIB_NUMBER_BASE:
    #    FIB_NUMBER_BASE = "base10"

    #main(FIB_EXE, FIB_INDEX, FIB_NUMBER_BASE)

    # Open output file fibnum.txt and read it to
    # verify the correctness of the Fibonacci number output
    #f = open("fibnum.txt", "r")
    #read = f.readlines()
    f = open("fibnum_crawler.txt", "a")

    for i in range(10000,20001):
        expect = fetch_fib_number(i)["base10"]
        print(i)
        f.write("%s\n" % expect)

