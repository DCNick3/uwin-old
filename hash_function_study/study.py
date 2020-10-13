addresses = [ int(x, 16) for x in open('basic_block_addresses.txt').read().split() ] 

# only 24 bits represent addresses
assert(len(addresses) == len(set(map(lambda x: x & 0xffffff, addresses))))

def test_func(name, func):
    r = set(map(func, addresses))
    num = len(addresses) - len(r)
    collisions = num / len(addresses)
    slots = max(r) + 1
    print("%20s: %.8f (%08d/%08d) %7s MiB %f" % (name, collisions, num, len(addresses), "%.2f" % (slots * 8 / 1024 / 1024), len(r) / slots))

test_func("id x", lambda x: x)
test_func("mod 500000", lambda x: x % 500000)
test_func("mod 5000000", lambda x: x % 5000000)
test_func("no lo 2", lambda x: x >> 2)
test_func("no lo 3", lambda x: x >> 3)
test_func("no lo 4", lambda x: x >> 4)
test_func("no lo 5", lambda x: x >> 5)
test_func("no lo 6", lambda x: x >> 6)
test_func("no lo 7", lambda x: x >> 7)
test_func("no lo 8", lambda x: x >> 8)
test_func("only lo 8", lambda x: x & 0xff)
test_func("only lo 16", lambda x: x & 0xffff)
test_func("only lo 17", lambda x: x & 0x1fff)
test_func("only lo 18", lambda x: x & 0x3ffff)
test_func("only lo 19", lambda x: x & 0x7ffff)
test_func("only lo 20", lambda x: x & 0xfffff)
test_func("only lo 21", lambda x: x & 0x1fffff)
test_func("only lo 22", lambda x: x & 0x3fffff)
test_func("only lo 23", lambda x: x & 0x7fffff)
test_func("only lo 24", lambda x: x & 0xffffff)
test_func("lo 8 + hi 8", lambda x: x & 0xff | ((x >> 8) & 0xff00))
test_func("lo 8 + hi 12", lambda x: x & 0xff | ((x >> 4) & 0xfff00))
