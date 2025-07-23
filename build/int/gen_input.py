import input_pb2 as pb2
import random
from google.protobuf.descriptor import FieldDescriptor

def fill_random(msg):
    for field in msg.DESCRIPTOR.fields:
        if field.label == FieldDescriptor.LABEL_REPEATED:
            num_items = random.randint(0, 3)
            for _ in range(num_items):
                if field.type == FieldDescriptor.TYPE_MESSAGE:
                    sub_msg = getattr(msg, field.name).add()
                    fill_random(sub_msg)
                elif field.type == FieldDescriptor.TYPE_INT32:
                    getattr(msg, field.name).append(random.randint(-100, 100))
                elif field.type == FieldDescriptor.TYPE_INT64:
                    getattr(msg, field.name).append(random.randint(-1000, 1000))
                elif field.type == FieldDescriptor.TYPE_FLOAT:
                    getattr(msg, field.name).append(random.uniform(-10.0, 10.0))
                elif field.type == FieldDescriptor.TYPE_DOUBLE:
                    getattr(msg, field.name).append(random.uniform(-100.0, 100.0))
                elif field.type == FieldDescriptor.TYPE_BOOL:
                    getattr(msg, field.name).append(random.choice([True, False]))
                elif field.type == FieldDescriptor.TYPE_STRING:
                    getattr(msg, field.name).append("random_str_" + str(random.randint(0, 100)))
                else:
                    pass  # Add more types as needed
        else:  # Optional or required
            if field.type == FieldDescriptor.TYPE_MESSAGE:
                sub_msg = getattr(msg, field.name)
                fill_random(sub_msg)
            elif field.type == FieldDescriptor.TYPE_INT32:
                setattr(msg, field.name, random.randint(-100, 100))
            elif field.type == FieldDescriptor.TYPE_INT64:
                setattr(msg, field.name, random.randint(-1000, 1000))
            elif field.type == FieldDescriptor.TYPE_FLOAT:
                setattr(msg, field.name, random.uniform(-10.0, 10.0))
            elif field.type == FieldDescriptor.TYPE_DOUBLE:
                setattr(msg, field.name, random.uniform(-100.0, 100.0))
            elif field.type == FieldDescriptor.TYPE_BOOL:
                setattr(msg, field.name, random.choice([True, False]))
            elif field.type == FieldDescriptor.TYPE_STRING:
                setattr(msg, field.name, "random_str_" + str(random.randint(0, 100)))
            else:
                pass  # Add more types as needed

m = pb2.Input()
fill_random(m)
with open("input.bin", "wb") as f:
    f.write(m.SerializeToString())
print("Generated message:", str(m))
