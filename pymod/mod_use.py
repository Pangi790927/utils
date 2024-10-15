#!/usr/bin/python3

import ctypes
import pymod

# TODO: make it so that the python module calls the callbacks with those parameters:

def cbk1(str_val, int_val, ctx):
    print("1st callback")

def cbk2(str_val, int_val, ctx):
    print(f"2nd callback [{str_val}] {int_val} {ctx}")

def cbk3(str_val, int_val, ctx):
    print(f"3rd callback [{str_val}] {int_val} {ctx}")

def cbk4(str_val, int_val, ctx):
    print("4th callback")

pymod.iset_cbk(14, cbk3, None)
pymod.iset_cbk(14, cbk3, None)
pymod.trigger_int(14, "the_string", 521)
pymod.sset_cbk("14", cbk2, None)
pymod.trigger_int(14, "the other string", 1222)
pymod.trigger_str("14", "the third string {}", -1)
pymod.unset_cbk(cbk3)

# my_functions = ctypes.CDLL("./pymod.so")
# print(my_functions.fn_do_stuff())
# print(my_functions.spam_system("ls -l"))