typedef struct {
    int id;
    char msg[16];
} SimpleStruct;


int P(const SimpleStruct* input, const char* msg_buf) {
    printf("id = %d\n", input->id);
    printf("msg = %s\n", msg_buf);
    return input->id;
}
