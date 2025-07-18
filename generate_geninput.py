import sys
import random

PROTO_MODULE = sys.argv[2] + "_pb2"
MESSAGE = sys.argv[3] 
# Always expect the real proto message name as argument (exact case)
# You can parse the .proto file for field names/types.
# For demo, hardcode for MyStruct.
with open("gen_input.py", "w") as f:
    f.write(f"""import {PROTO_MODULE}
import random

m = {PROTO_MODULE}.{MESSAGE}()
m.id = random.randint(1,100)
m.score = random.uniform(0,100)
m.name = "random_name"
m.values.extend([random.random() for _ in range(3)])
m.sub.code = random.randint(1,1000)
m.sub.desc = "random_sub"
with open("input.bin", "wb") as f:
    f.write(m.SerializeToString())
""")