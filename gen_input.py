import input_pb2
import json
import random

def random_json():
    return json.dumps({
        "array": [random.randint(0,100), random.uniform(0,1)],
        "bool": random.choice([True, False]),
        "obj": {"a": random.randint(1,99)}
    })

m = input_pb2.Input()
m.s = random_json()
with open("input.bin", "wb") as f:
    f.write(m.SerializeToString())
print("Generated JSON string:", m.s)
