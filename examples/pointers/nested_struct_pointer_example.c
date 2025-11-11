// Test case: Pointer to struct containing nested structs
// This tests handling of complex nested structures

struct Address {
    int zip;
    int street_number;
};

struct Person {
    int age;
    struct Address addr;
};

// Update person's address - pointer to struct with nested struct
int update_person(struct Person *person, int new_zip) {
    if (!person) {
        return -1;
    }

    person->addr.zip = new_zip;
    return 0;
}
