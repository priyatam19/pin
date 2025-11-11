#include <stdio.h>
#include <string.h>

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

#ifdef MULTI_STRING_PACKET_MAIN
int main(void) {
    struct NotePacket pkt = {
        .id = 42,
        .primary = "heat",
        .secondary = ""
    };
    return inspect_packet(pkt);
}
#endif
