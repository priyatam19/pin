# 0 "tmp_structs.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "tmp_structs.c"

struct NotePacket {
    int id;
    char primary[8];
    char secondary[8];
};

int inspect_packet(struct NotePacket pkt) {
    size_t len_primary = strlen(pkt.primary);
    size_t len_secondary = strlen(pkt.secondary);

    printf("packet id:%d primary[%zu]:%s secondary[%zu]:%s\n",
           pkt.id,
           len_primary,
           pkt.primary,
           len_secondary,
           pkt.secondary);
    return (int)(len_primary + len_secondary);
}

int main(void) {
    struct NotePacket pkt = {
        .id = 42,
        .primary = "heat",
        .secondary = ""
    };
    return inspect_packet(pkt);
}
