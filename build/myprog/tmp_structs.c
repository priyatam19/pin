# 0 "temp_no_pp.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "temp_no_pp.c"
typedef struct {
    int id;
    float score;
    char name[32];
    double values[3];
    struct {
        int code;
        char desc[16];
    } sub;
} MyStruct;



int P(const MyStruct* input, const char* name_buf, const char* sub_desc_buf) {
    printf("Inside P\n");
    printf("id = %d\n", input->id);
    printf("score = %f\n", input->score);
    printf("name = %s\n", name_buf);
    printf("values = [%f, %f, %f]\n", input->values[0], input->values[1], input->values[2]);
    printf("sub.code = %d\n", input->sub.code);
    printf("sub.desc = %s\n", sub_desc_buf);

    return input->id + input->sub.code;
}
