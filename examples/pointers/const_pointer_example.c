// Test case: Various const qualifier combinations
// This tests proper handling of const qualifiers

struct Point {
    double x;
    double y;
};

// Const pointer to const data
double calculate_distance(const struct Point *p1, const struct Point *p2) {
    if (!p1 || !p2) {
        return -1.0;
    }

    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;

    // Simple distance calculation (without sqrt for simplicity)
    return dx * dx + dy * dy;
}
