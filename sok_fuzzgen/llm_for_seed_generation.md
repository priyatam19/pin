Here are a couple of concrete, non-LLM strategies that stay local, reuse the existing proto metadata, and don’t require shipping full source to an external model:

Proto-Aware Constraint Mining (Static + Symbolic Hints)

When we generate the .proto, we already know field names, pointer metadata, and enum candidates. Extend that pass to scan the target function body (via libclang AST) for simple guards such as if (msg->len < 4) return;, switch(msg->cmd), memcmp(...), etc.
Emit a lightweight JSON “hint” file alongside input.proto that encodes observed constants (cmd must be MQTT_CMD_CONNECT, topic.len <= 256, etc.).
Feed those hints into a corpus synthesizer that enumerates small combinations: pick meaningful enums, set length fields to boundary values (0, threshold-1, threshold+1), and ensure dependent buffers (bytes fields) respect the hinted sizes.
Result: deterministic seeds that cover common branch conditions before the fuzzer even starts mutating.
AST-Guided Trace Replayer (No External Models)

Hook the build to optionally run the target with instrumentation (e.g., compile with -fsanitize-coverage=trace-pc) and execute a curated set of “semi-random” proto instances.
Start with the proto schema, fill each field with structured noise (ASCII strings, small integers, repeated nested messages) and run the wrapper in a loop while collecting coverage via libFuzzer -runs=N.
Anytime a new basic block trips, serialize that proto to disk and add it to seed_corpus/. You can iterate this offline: mutate proto fields with knowledge of their type (flip one enum, grow a repeated field) and keep anything that expands coverage.
Because this uses the proto machinery already in place, it scales to any structured function; and since the process stays local, you don’t share source beyond the instrumentation.
Both approaches keep code inside your environment, leverage the metadata the pipeline already emits, and can run as pre-fuzz steps so that Stage A starts from meaningful seeds rather than all-zero protobufs. Let me know which direction you want to prototype first, and I can start sketching the scripts.